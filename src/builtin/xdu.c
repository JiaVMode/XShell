/*
 * xdu.c - 显示目录大小
 * 
 * 功能：显示目录及其子目录的磁盘使用量
 * 用法：xdu [选项] [目录]...
 * 
 * 选项：
 *   -h    人类可读格式
 *   -s    只显示总计
 *   --help 显示帮助信息
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] [目录]...\n", cmd_name);
    printf("功能: 显示目录及其子目录的磁盘使用量\n");
    printf("选项:\n");
    printf("  -h, --human-readable  人类可读格式（KB, MB, GB）\n");
    printf("  -s, --summarize        只显示总计\n");
    printf("  -h, --help            显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s /path/to/dir\n", cmd_name);
    printf("  %s -h /path/to/dir\n", cmd_name);
    printf("  %s -s /path/to/dir\n", cmd_name);
}

// 格式化大小（人类可读）
static void format_size(long long size, int human_readable, char *buf, size_t buf_size) {
    if (human_readable) {
        if (size < 1024) {
            snprintf(buf, buf_size, "%lldB", size);
        } else if (size < 1024 * 1024) {
            snprintf(buf, buf_size, "%.1fK", size / 1024.0);
        } else if (size < 1024LL * 1024 * 1024) {
            snprintf(buf, buf_size, "%.1fM", size / (1024.0 * 1024));
        } else {
            snprintf(buf, buf_size, "%.1fG", size / (1024.0 * 1024.0 * 1024));
        }
    } else {
        snprintf(buf, buf_size, "%lld", size);
    }
}

// 计算目录大小（递归）
static long long calculate_dir_size(const char *dir_path, int summarize) {
    long long total_size = 0;
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    char path[4096];
    
    dir = opendir(dir_path);
    if (dir == NULL) {
        return 0;
    }
    
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 构建完整路径
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        
        if (lstat(path, &st) != 0) {
            continue;
        }
        
        if (S_ISDIR(st.st_mode)) {
            // 递归计算子目录
            if (!summarize) {
                long long sub_size = calculate_dir_size(path, 0);
                total_size += sub_size;
            }
            total_size += st.st_size;  // 目录本身的大小
        } else {
            // 文件大小
            total_size += st.st_size;
        }
    }
    
    closedir(dir);
    return total_size;
}

// 显示目录大小
static int show_dir_size(const char *dir_path, int human_readable, int summarize, ShellContext *ctx) {
    struct stat st;
    
    if (lstat(dir_path, &st) != 0) {
        XSHELL_LOG_ERROR(ctx, "xdu: %s: %s\n", dir_path, strerror(errno));
        return -1;
    }
    
    long long size;
    
    if (S_ISDIR(st.st_mode)) {
        size = calculate_dir_size(dir_path, summarize);
    } else {
        size = st.st_size;
    }
    
    char size_str[64];
    format_size(size, human_readable, size_str, sizeof(size_str));
    
    printf("%s\t%s\n", size_str, dir_path);
    
    return 0;
}

// xdu 命令实现
int cmd_xdu(Command *cmd, ShellContext *ctx) {
    int human_readable = 0;
    int summarize = 0;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-h") == 0) {
            // -h 可能是帮助或 human-readable，检查是否有后续参数
            if (i + 1 < cmd->arg_count && cmd->args[i + 1][0] != '-') {
                // 后面有非选项参数，-h 是 human-readable
                human_readable = 1;
                i++;
            } else {
                // 后面没有参数或是选项，-h 是帮助
                show_help(cmd->name);
                return 0;
            }
        } else if (strcmp(cmd->args[i], "--human-readable") == 0) {
            human_readable = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-s") == 0 || strcmp(cmd->args[i], "--summarize") == 0) {
            summarize = 1;
            i++;
        } else {
            break;
        }
    }
    
    // 处理目录
    if (i >= cmd->arg_count) {
        // 没有指定目录，使用当前目录
        char cwd[4096];
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            show_dir_size(cwd, human_readable, summarize, ctx);
        }
    } else {
        for (; i < cmd->arg_count; i++) {
            show_dir_size(cmd->args[i], human_readable, summarize, ctx);
        }
    }
    
    return 0;
}

