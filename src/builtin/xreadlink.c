/*
 * xreadlink.c - 读取符号链接
 * 
 * 功能：显示符号链接指向的目标路径
 * 用法：xreadlink [选项] <链接文件>
 * 
 * 选项：
 *   -f    显示绝对路径（解析到最终目标）
 *   --help 显示帮助信息
 */

#define _GNU_SOURCE
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <链接文件>\n", cmd_name);
    printf("功能: 显示符号链接指向的目标路径\n");
    printf("选项:\n");
    printf("  -f, --canonicalize  显示绝对路径（解析到最终目标）\n");
    printf("  -h, --help          显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s symlink.txt\n", cmd_name);
    printf("  %s -f symlink.txt\n", cmd_name);
}

// xreadlink 命令实现
int cmd_xreadlink(Command *cmd, ShellContext *ctx) {
    int canonicalize = 0;
    const char *link_path = NULL;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-f") == 0 || strcmp(cmd->args[i], "--canonicalize") == 0) {
            canonicalize = 1;
        } else if (link_path == NULL) {
            link_path = cmd->args[i];
        } else {
            XSHELL_LOG_ERROR(ctx, "xreadlink: 错误: 只能指定一个链接文件\n");
            return -1;
        }
    }
    
    if (link_path == NULL) {
        XSHELL_LOG_ERROR(ctx, "xreadlink: 错误: 需要指定链接文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 读取符号链接
    if (canonicalize) {
        // 使用 realpath 解析到最终目标
        char resolved_path[PATH_MAX];
        if (realpath(link_path, resolved_path) != NULL) {
            printf("%s\n", resolved_path);
            return 0;
        } else {
            XSHELL_LOG_ERROR(ctx, "xreadlink: %s: %s\n", link_path, strerror(errno));
            return -1;
        }
    } else {
        // 使用 readlink 读取链接目标
        char target[PATH_MAX];
        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        
        if (len == -1) {
            XSHELL_LOG_ERROR(ctx, "xreadlink: %s: %s\n", link_path, strerror(errno));
            return -1;
        }
        
        target[len] = '\0';
        printf("%s\n", target);
        return 0;
    }
}



