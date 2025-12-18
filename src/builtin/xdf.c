/*
 * xdf.c - 显示磁盘空间
 * 
 * 功能：显示文件系统的磁盘空间使用情况
 * 用法：xdf [选项] [文件系统]...
 * 
 * 选项：
 *   -h    人类可读格式
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/statvfs.h>
#include <errno.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] [文件系统]...\n", cmd_name);
    printf("功能: 显示文件系统的磁盘空间使用情况\n");
    printf("选项:\n");
    printf("  -h, --human-readable  人类可读格式（KB, MB, GB）\n");
    printf("  -h, --help            显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s\n", cmd_name);
    printf("  %s -h\n", cmd_name);
    printf("  %s /\n", cmd_name);
}

// 格式化大小
static void format_size(unsigned long long size, int human_readable, char *buf, size_t buf_size) {
    if (human_readable) {
        if (size < 1024) {
            snprintf(buf, buf_size, "%lluB", size);
        } else if (size < 1024ULL * 1024) {
            snprintf(buf, buf_size, "%.1fK", size / 1024.0);
        } else if (size < 1024ULL * 1024 * 1024) {
            snprintf(buf, buf_size, "%.1fM", size / (1024.0 * 1024));
        } else {
            snprintf(buf, buf_size, "%.1fG", size / (1024.0 * 1024.0 * 1024));
        }
    } else {
        snprintf(buf, buf_size, "%llu", size);
    }
}

// 显示文件系统信息
static int show_filesystem(const char *path, int human_readable, ShellContext *ctx) {
    struct statvfs vfs;
    
    if (statvfs(path, &vfs) != 0) {
        XSHELL_LOG_ERROR(ctx, "xdf: %s: %s\n", path, strerror(errno));
        return -1;
    }
    
    unsigned long long block_size = vfs.f_frsize;
    unsigned long long total_blocks = vfs.f_blocks;
    unsigned long long free_blocks = vfs.f_bfree;
    unsigned long long available_blocks = vfs.f_bavail;
    unsigned long long used_blocks = total_blocks - free_blocks;
    
    unsigned long long total = total_blocks * block_size;
    unsigned long long used = used_blocks * block_size;
    unsigned long long available = available_blocks * block_size;
    unsigned long long free = free_blocks * block_size;
    
    char total_str[64], used_str[64], available_str[64], free_str[64];
    format_size(total, human_readable, total_str, sizeof(total_str));
    format_size(used, human_readable, used_str, sizeof(used_str));
    format_size(available, human_readable, available_str, sizeof(available_str));
    format_size(free, human_readable, free_str, sizeof(free_str));
    
    unsigned long long percent_used = total > 0 ? (used * 100) / total : 0;
    
    printf("%-20s %10s %10s %10s %5llu%% %s\n",
           path, total_str, used_str, available_str, percent_used, path);
    
    return 0;
}

// xdf 命令实现
int cmd_xdf(Command *cmd, ShellContext *ctx) {
    int human_readable = 0;
    
    // 解析参数
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-h") == 0) {
            // -h 可能是帮助或 human-readable
            if (i + 1 < cmd->arg_count && cmd->args[i + 1][0] != '-') {
                human_readable = 1;
                i++;
            } else {
                show_help(cmd->name);
                return 0;
            }
        } else if (strcmp(cmd->args[i], "--human-readable") == 0) {
            human_readable = 1;
            i++;
        } else {
            break;
        }
    }
    
    // 打印表头
    printf("%-20s %10s %10s %10s %6s %s\n",
           "文件系统", "总大小", "已用", "可用", "使用%", "挂载点");
    
    // 处理文件系统
    if (i >= cmd->arg_count) {
        // 没有指定，显示根文件系统
        show_filesystem("/", human_readable, ctx);
    } else {
        for (; i < cmd->arg_count; i++) {
            show_filesystem(cmd->args[i], human_readable, ctx);
        }
    }
    
    return 0;
}

