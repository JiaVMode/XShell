/*
 * xbasename.c - 提取文件名
 * 
 * 功能：从路径中提取文件名（去除目录部分）
 * 用法：xbasename <路径> [后缀]
 * 
 * 选项：
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s <路径> [后缀]\n", cmd_name);
    printf("功能: 从路径中提取文件名（去除目录部分）\n");
    printf("选项:\n");
    printf("  -h, --help       显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s /path/to/file.txt\n", cmd_name);
    printf("  %s /path/to/file.txt .txt\n", cmd_name);
    printf("  %s file.txt\n", cmd_name);
}

// xbasename 命令实现
int cmd_xbasename(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    // 检查参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    // 检查帮助选项
    if (cmd->arg_count >= 2) {
        if (strcmp(cmd->args[1], "--help") == 0 || strcmp(cmd->args[1], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        }
    }
    
    // 获取路径
    const char *path = cmd->args[1];
    const char *suffix = NULL;
    
    if (cmd->arg_count >= 3) {
        suffix = cmd->args[2];
    }
    
    // 查找最后一个 '/'
    const char *filename = strrchr(path, '/');
    if (filename == NULL) {
        // 没有目录分隔符，整个路径就是文件名
        filename = path;
    } else {
        // 跳过 '/'
        filename++;
    }
    
    // 如果指定了后缀，去除后缀
    if (suffix != NULL && *suffix != '\0') {
        size_t filename_len = strlen(filename);
        size_t suffix_len = strlen(suffix);
        
        // 检查是否以指定后缀结尾
        if (filename_len >= suffix_len && 
            strcmp(filename + filename_len - suffix_len, suffix) == 0) {
            // 输出文件名（去除后缀）
            for (size_t i = 0; i < filename_len - suffix_len; i++) {
                putchar(filename[i]);
            }
            putchar('\n');
            return 0;
        }
    }
    
    // 输出文件名
    printf("%s\n", filename);
    return 0;
}



