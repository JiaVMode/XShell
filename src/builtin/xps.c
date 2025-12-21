// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出
#include <stdlib.h>             // 标准库函数
#include <string.h>             // 字符串处理
#include <unistd.h>             // UNIX 标准函数
#include <dirent.h>             // 目录操作
#include <ctype.h>              // 字符类型判断
#include <pwd.h>                // 用户信息

// ============================================
// xps 命令实现函数 - 增强版
// ============================================
// 命令名称：xps
// 功能：美观地显示进程信息
// ============================================

// 颜色定义
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_HEADER  "\033[1;36m"  // 青色粗体
#define C_PID     "\033[33m"    // 黄色
#define C_USER    "\033[32m"    // 绿色
#define C_CMD     "\033[37m"    // 白色
#define C_RUNNING "\033[1;32m"  // 绿色粗体
#define C_SLEEP   "\033[34m"    // 蓝色
#define C_ZOMBIE  "\033[1;31m"  // 红色粗体
#define C_BORDER  "\033[36m"    // 青色

// 进程状态描述
static const char* get_state_desc(char state) {
    switch (state) {
        case 'R': return "运行";
        case 'S': return "睡眠";
        case 'D': return "等待";
        case 'Z': return "僵尸";
        case 'T': return "停止";
        case 'I': return "空闲";
        default:  return "未知";
    }
}

// 获取状态颜色
static const char* get_state_color(char state) {
    switch (state) {
        case 'R': return C_RUNNING;
        case 'S': return C_SLEEP;
        case 'Z': return C_ZOMBIE;
        default:  return C_RESET;
    }
}

// 格式化内存大小
static void format_mem(unsigned long kb, char *buf, size_t size) {
    if (kb >= 1024 * 1024) {
        snprintf(buf, size, "%.1fG", (double)kb / (1024 * 1024));
    } else if (kb >= 1024) {
        snprintf(buf, size, "%.1fM", (double)kb / 1024);
    } else {
        snprintf(buf, size, "%luK", kb);
    }
}

// 进程信息结构
typedef struct {
    int pid;
    int ppid;
    char user[32];
    char state;
    unsigned long vsize;      // 虚拟内存
    unsigned long rss;        // 物理内存
    float cpu_percent;
    char cmd[256];
} ProcessInfo;

// 读取进程信息
static int read_process_info(int pid, ProcessInfo *info) {
    char path[256];
    char buffer[1024];
    
    info->pid = pid;
    info->cpu_percent = 0;
    
    // 读取 /proc/[pid]/stat
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    FILE *fp = fopen(path, "r");
    if (!fp) return 0;
    
    int dummy_int;
    unsigned long dummy_ul;
    long dummy_l;
    
    // 读取基本信息
    if (fscanf(fp, "%d (%255[^)]) %c %d",
               &info->pid, info->cmd, &info->state, &info->ppid) != 4) {
        fclose(fp);
        return 0;
    }
    
    // 跳过一些字段，读取内存信息
    for (int i = 0; i < 18; i++) {
        fscanf(fp, "%ld", &dummy_l);
    }
    fscanf(fp, "%lu %ld", &info->vsize, &dummy_l);  // vsize
    info->rss = dummy_l * 4;  // rss in pages -> KB
    
    fclose(fp);
    
    // 读取用户
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    fp = fopen(path, "r");
    if (fp) {
        while (fgets(buffer, sizeof(buffer), fp)) {
            if (strncmp(buffer, "Uid:", 4) == 0) {
                int uid;
                sscanf(buffer + 4, "%d", &uid);
                struct passwd *pw = getpwuid(uid);
                if (pw) {
                    strncpy(info->user, pw->pw_name, sizeof(info->user) - 1);
                } else {
                    snprintf(info->user, sizeof(info->user), "%d", uid);
                }
                break;
            }
        }
        fclose(fp);
    }
    
    return 1;
}

// 比较函数（按 PID 排序）
static int compare_proc(const void *a, const void *b) {
    return ((ProcessInfo*)a)->pid - ((ProcessInfo*)b)->pid;
}

int cmd_xps(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    int show_all = 0;
    
    // 解析参数
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            printf("xps - 显示进程信息（增强版）\n\n");
            printf("用法:\n");
            printf("  xps              显示当前用户的进程\n");
            printf("  xps -a           显示所有进程\n");
            printf("  xps --help       显示帮助信息\n\n");
            printf("显示信息:\n");
            printf("  PID    - 进程ID\n");
            printf("  PPID   - 父进程ID\n");
            printf("  USER   - 用户名\n");
            printf("  STATE  - 进程状态\n");
            printf("  MEM    - 内存使用\n");
            printf("  CMD    - 命令名称\n\n");
            printf("进程状态:\n");
            printf("  运行(R) - 正在执行\n");
            printf("  睡眠(S) - 可中断睡眠\n");
            printf("  等待(D) - 不可中断睡眠\n");
            printf("  僵尸(Z) - 已终止等待回收\n");
            printf("  停止(T) - 已停止\n\n");
            return 0;
        }
        if (strcmp(cmd->args[i], "-a") == 0 || strcmp(cmd->args[i], "--all") == 0) {
            show_all = 1;
        }
    }
    
    // 收集进程信息
    ProcessInfo procs[1024];
    int count = 0;
    
    DIR *dir = opendir("/proc");
    if (!dir) {
        fprintf(stderr, "无法访问 /proc\n");
        return -1;
    }
    
    uid_t my_uid = getuid();
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < 1024) {
        // 只处理数字目录（进程目录）
        if (!isdigit(entry->d_name[0])) continue;
        
        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        ProcessInfo info = {0};
        if (!read_process_info(pid, &info)) continue;
        
        // 如果不是显示全部，只显示当前用户的进程
        if (!show_all) {
            char path[64];
            snprintf(path, sizeof(path), "/proc/%d/status", pid);
            FILE *fp = fopen(path, "r");
            if (fp) {
                char buffer[256];
                int proc_uid = -1;
                while (fgets(buffer, sizeof(buffer), fp)) {
                    if (strncmp(buffer, "Uid:", 4) == 0) {
                        sscanf(buffer + 4, "%d", &proc_uid);
                        break;
                    }
                }
                fclose(fp);
                if (proc_uid != (int)my_uid) continue;
            }
        }
        
        procs[count++] = info;
    }
    closedir(dir);
    
    // 排序
    qsort(procs, count, sizeof(ProcessInfo), compare_proc);
    
    // 输出表头
    printf("\n");
    printf(C_BORDER "╔═══════╤═══════╤══════════╤════════╤═════════╤══════════════════════════╗\n" C_RESET);
    printf(C_BORDER "║" C_HEADER " %5s " C_BORDER "│" C_HEADER " %5s " C_BORDER "│" 
           C_HEADER " %-8s " C_BORDER "│" C_HEADER " %-6s " C_BORDER "│" 
           C_HEADER " %7s " C_BORDER "│" C_HEADER " %-24s " C_BORDER "║\n" C_RESET,
           "PID", "PPID", "USER", "状态", "内存", "命令");
    printf(C_BORDER "╟───────┼───────┼──────────┼────────┼─────────┼──────────────────────────╢\n" C_RESET);
    
    // 输出进程信息
    for (int i = 0; i < count; i++) {
        ProcessInfo *p = &procs[i];
        
        char mem_str[16];
        format_mem(p->rss, mem_str, sizeof(mem_str));
        
        // 截断命令名
        char cmd_short[25];
        strncpy(cmd_short, p->cmd, 24);
        cmd_short[24] = '\0';
        
        printf(C_BORDER "║" C_PID " %5d " C_BORDER "│" C_RESET " %5d " C_BORDER "│" 
               C_USER " %-8.8s " C_BORDER "│" " %s%-6s" C_RESET " " C_BORDER "│" 
               C_RESET " %7s " C_BORDER "│" C_CMD " %-24s " C_BORDER "║\n" C_RESET,
               p->pid, p->ppid, p->user, 
               get_state_color(p->state), get_state_desc(p->state),
               mem_str, cmd_short);
    }
    
    // 输出表尾
    printf(C_BORDER "╚═══════╧═══════╧══════════╧════════╧═════════╧══════════════════════════╝\n" C_RESET);
    printf(C_RESET "共 " C_BOLD "%d" C_RESET " 个进程\n\n", count);
    
    return 0;
}
