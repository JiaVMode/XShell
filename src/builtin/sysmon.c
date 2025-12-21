/**
 * @file sysmon.c
 * @brief 系统监控模块 - 美观的系统信息显示
 */

#define _POSIX_C_SOURCE 200809L
#include "xui.h"
#include "xshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <sys/utsname.h>
#include <time.h>
#include <sys/select.h>

// 颜色定义
#define COLOR_TITLE    220  // 金色
#define COLOR_LABEL    75   // 蓝色
#define COLOR_VALUE    255  // 白色
#define COLOR_BAR_LOW  46   // 绿色
#define COLOR_BAR_MED  226  // 黄色
#define COLOR_BAR_HIGH 196  // 红色
#define COLOR_BORDER   39   // 青色

// 绘制进度条
static void draw_progress_bar(int percent, int width) {
    int filled = (percent * width) / 100;
    
    // 根据百分比选择颜色
    int color;
    if (percent < 50) color = COLOR_BAR_LOW;
    else if (percent < 80) color = COLOR_BAR_MED;
    else color = COLOR_BAR_HIGH;
    
    xui_term_set_fg256(color);
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("]");
    xui_term_reset_style();
    printf(" %3d%%", percent);
}

// 格式化内存大小
static void format_size(unsigned long kb, char *buf, size_t size) {
    if (kb >= 1024 * 1024) {
        snprintf(buf, size, "%.1f GB", (double)kb / (1024 * 1024));
    } else if (kb >= 1024) {
        snprintf(buf, size, "%.1f MB", (double)kb / 1024);
    } else {
        snprintf(buf, size, "%lu KB", kb);
    }
}

// 获取 CPU 使用率
static int get_cpu_usage(void) {
    static long prev_idle = 0, prev_total = 0;
    long idle = 0, total = 0;
    
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) return 0;
    
    long user, nice, system, idle_time, iowait, irq, softirq;
    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld",
               &user, &nice, &system, &idle_time, &iowait, &irq, &softirq) == 7) {
        idle = idle_time + iowait;
        total = user + nice + system + idle_time + iowait + irq + softirq;
    }
    fclose(fp);
    
    long diff_idle = idle - prev_idle;
    long diff_total = total - prev_total;
    
    int usage = 0;
    if (diff_total > 0 && prev_total > 0) {
        usage = (int)(100 * (diff_total - diff_idle) / diff_total);
    }
    
    prev_idle = idle;
    prev_total = total;
    
    return usage < 0 ? 0 : (usage > 100 ? 100 : usage);
}

// 获取内存信息
static void get_memory_info(unsigned long *total, unsigned long *used, int *percent) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        *total = si.totalram / 1024;
        unsigned long free = si.freeram / 1024;
        unsigned long buffers = si.bufferram / 1024;
        *used = *total - free - buffers;
        *percent = (int)(100 * (*used) / (*total));
    } else {
        *total = *used = 0;
        *percent = 0;
    }
}

// 获取磁盘信息
static void get_disk_info(unsigned long *total, unsigned long *used, int *percent) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        *total = (vfs.f_blocks * vfs.f_frsize) / 1024;
        unsigned long free = (vfs.f_bfree * vfs.f_frsize) / 1024;
        *used = *total - free;
        *percent = (int)(100 * (*used) / (*total));
    } else {
        *total = *used = 0;
        *percent = 0;
    }
}

// 获取运行时间
static void get_uptime_str(char *buf, size_t size) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        long uptime = si.uptime;
        int days = uptime / 86400;
        int hours = (uptime % 86400) / 3600;
        int mins = (uptime % 3600) / 60;
        
        if (days > 0) {
            snprintf(buf, size, "%d天 %d小时 %d分钟", days, hours, mins);
        } else if (hours > 0) {
            snprintf(buf, size, "%d小时 %d分钟", hours, mins);
        } else {
            snprintf(buf, size, "%d分钟", mins);
        }
    } else {
        snprintf(buf, size, "未知");
    }
}

// 获取负载
static void get_load_avg(double *load1, double *load5, double *load15) {
    FILE *fp = fopen("/proc/loadavg", "r");
    if (fp) {
        fscanf(fp, "%lf %lf %lf", load1, load5, load15);
        fclose(fp);
    } else {
        *load1 = *load5 = *load15 = 0;
    }
}

// 等待按键
static int sysmon_wait_key(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        return xui_term_read_key();
    }
    return 0;
}

// 系统监控主函数
void xsysmon(void) {
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    xui_term_alt_screen_enter();
    xui_term_init();
    xui_term_hide_cursor();
    
    // 初始化 CPU 统计
    get_cpu_usage();
    usleep(100000);
    
    while (1) {
        xui_term_clear();
        
        // 获取终端尺寸
        xui_term_get_size(&term_h, &term_w);
        int box_w = 50;
        int ox = (term_w - box_w) / 2;
        int oy = 2;
        
        // 获取系统信息
        struct utsname uts;
        uname(&uts);
        
        unsigned long mem_total, mem_used, disk_total, disk_used;
        int mem_percent, disk_percent;
        get_memory_info(&mem_total, &mem_used, &mem_percent);
        get_disk_info(&disk_total, &disk_used, &disk_percent);
        int cpu_percent = get_cpu_usage();
        
        char uptime_str[64];
        get_uptime_str(uptime_str, sizeof(uptime_str));
        
        double load1, load5, load15;
        get_load_avg(&load1, &load5, &load15);
        
        char mem_total_str[32], mem_used_str[32];
        char disk_total_str[32], disk_used_str[32];
        format_size(mem_total, mem_total_str, sizeof(mem_total_str));
        format_size(mem_used, mem_used_str, sizeof(mem_used_str));
        format_size(disk_total, disk_total_str, sizeof(disk_total_str));
        format_size(disk_used, disk_used_str, sizeof(disk_used_str));
        
        // ===== 绘制界面 =====
        
        // 标题
        xui_term_move_to(oy, ox + box_w/2 - 8);
        xui_term_set_fg256(COLOR_TITLE);
        xui_term_set_bold();
        printf("[ 系统监控 ]");
        xui_term_reset_style();
        
        // 边框
        xui_term_set_fg256(COLOR_BORDER);
        xui_term_move_to(oy + 1, ox);
        printf("╔");
        for (int i = 0; i < box_w - 2; i++) printf("═");
        printf("╗");
        
        for (int y = 2; y <= 14; y++) {
            xui_term_move_to(oy + y, ox);
            printf("║");
            xui_term_move_to(oy + y, ox + box_w - 1);
            printf("║");
        }
        
        xui_term_move_to(oy + 15, ox);
        printf("╚");
        for (int i = 0; i < box_w - 2; i++) printf("═");
        printf("╝");
        xui_term_reset_style();
        
        // 系统信息
        xui_term_move_to(oy + 2, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        printf("主机名: ");
        xui_term_set_fg256(COLOR_VALUE);
        printf("%s", uts.nodename);
        
        xui_term_move_to(oy + 3, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        printf("系统:   ");
        xui_term_set_fg256(COLOR_VALUE);
        printf("%s %s", uts.sysname, uts.release);
        
        xui_term_move_to(oy + 4, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        printf("运行:   ");
        xui_term_set_fg256(COLOR_VALUE);
        printf("%s", uptime_str);
        
        xui_term_move_to(oy + 5, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        printf("负载:   ");
        xui_term_set_fg256(COLOR_VALUE);
        printf("%.2f  %.2f  %.2f", load1, load5, load15);
        xui_term_reset_style();
        
        // 分隔线
        xui_term_set_fg256(COLOR_BORDER);
        xui_term_move_to(oy + 6, ox);
        printf("╟");
        for (int i = 0; i < box_w - 2; i++) printf("─");
        printf("╢");
        xui_term_reset_style();
        
        // CPU
        xui_term_move_to(oy + 7, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        xui_term_set_bold();
        printf("CPU ");
        xui_term_reset_style();
        draw_progress_bar(cpu_percent, 30);
        
        // 内存
        xui_term_move_to(oy + 9, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        xui_term_set_bold();
        printf("内存");
        xui_term_reset_style();
        draw_progress_bar(mem_percent, 30);
        
        xui_term_move_to(oy + 10, ox + 6);
        xui_term_set_fg256(244);
        printf("%s / %s", mem_used_str, mem_total_str);
        xui_term_reset_style();
        
        // 磁盘
        xui_term_move_to(oy + 11, ox + 2);
        xui_term_set_fg256(COLOR_LABEL);
        xui_term_set_bold();
        printf("磁盘");
        xui_term_reset_style();
        draw_progress_bar(disk_percent, 30);
        
        xui_term_move_to(oy + 12, ox + 6);
        xui_term_set_fg256(244);
        printf("%s / %s", disk_used_str, disk_total_str);
        xui_term_reset_style();
        
        // 帮助
        xui_term_move_to(oy + 14, ox + 2);
        xui_term_set_fg256(244);
        printf("按 Q 退出 | 自动刷新中...");
        xui_term_reset_style();
        
        // 时间戳
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%H:%M:%S", tm_info);
        
        xui_term_move_to(oy + 14, ox + box_w - 12);
        xui_term_set_fg256(75);
        printf("%s", time_str);
        xui_term_reset_style();
        
        fflush(stdout);
        
        // 等待按键或刷新
        int key = sysmon_wait_key(1000);  // 每秒刷新
        if (key == 'q' || key == 'Q' || key == 27) {
            break;
        }
    }
    
    xui_term_alt_screen_leave();
    xui_term_restore();
    system("stty sane");
    fputs("\033[?25h", stdout);
    printf("\n");
}

// 命令行入口
int cmd_xsysmon(struct Command *cmd, struct ShellContext *ctx) {
    (void)cmd;
    (void)ctx;
    
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xsysmon - 系统监控\n\n");
        printf("用法:\n");
        printf("  xsysmon          启动系统监控\n");
        printf("  xsysmon --help   显示帮助信息\n\n");
        printf("功能:\n");
        printf("  显示 CPU、内存、磁盘使用情况\n");
        printf("  显示系统负载和运行时间\n");
        printf("  每秒自动刷新\n\n");
        printf("控制:\n");
        printf("  Q / ESC          退出\n\n");
        return 0;
    }
    
    xsysmon();
    return 0;
}
