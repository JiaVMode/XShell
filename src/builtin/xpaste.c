/*
 * xpaste.c - 合并文件行
 * 
 * 功能：将多个文件的行按列合并
 * 用法：xpaste [选项] [文件...]
 * 
 * 选项：
 *   -d <分隔符>  指定分隔符（默认制表符）
 *   --help       显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_FILES 100
#define MAX_LINE_LENGTH 4096

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] [文件...]\n", cmd_name);
    printf("功能: 将多个文件的行按列合并\n");
    printf("选项:\n");
    printf("  -d <分隔符>    指定分隔符（默认制表符）\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s file1.txt file2.txt\n", cmd_name);
    printf("  %s -d: file1.txt file2.txt\n", cmd_name);
}

// xpaste 命令实现
int cmd_xpaste(Command *cmd, ShellContext *ctx) {
    char delimiter = '\t';  // 默认制表符
    FILE *files[MAX_FILES];
    int file_count = 0;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-d") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xpaste: 错误: -d 选项需要参数\n");
                return -1;
            }
            delimiter = cmd->args[i + 1][0];
            i += 2;
        } else {
            break;
        }
    }
    
    // 打开所有文件
    for (; i < cmd->arg_count && file_count < MAX_FILES; i++) {
        const char *filename = cmd->args[i];
        FILE *file;
        
        if (strcmp(filename, "-") == 0) {
            file = stdin;
        } else {
            file = fopen(filename, "r");
            if (file == NULL) {
                XSHELL_LOG_ERROR(ctx, "xpaste: %s: %s\n", filename, strerror(errno));
                // 关闭已打开的文件
                for (int j = 0; j < file_count; j++) {
                    if (files[j] != stdin) {
                        fclose(files[j]);
                    }
                }
                return -1;
            }
        }
        
        files[file_count++] = file;
    }
    
    // 如果没有文件，使用标准输入
    if (file_count == 0) {
        files[0] = stdin;
        file_count = 1;
    }
    
    // 逐行合并
    char lines[MAX_FILES][MAX_LINE_LENGTH];
    int eof_flags[MAX_FILES] = {0};
    int all_eof = 0;
    
    while (!all_eof) {
        all_eof = 1;
        int has_data = 0;
        
        // 读取每个文件的下一行
        for (int j = 0; j < file_count; j++) {
            if (!eof_flags[j]) {
                if (fgets(lines[j], sizeof(lines[j]), files[j]) != NULL) {
                    // 移除换行符
                    size_t len = strlen(lines[j]);
                    if (len > 0 && lines[j][len - 1] == '\n') {
                        lines[j][len - 1] = '\0';
                    }
                    all_eof = 0;
                    has_data = 1;
                } else {
                    // 文件结束
                    lines[j][0] = '\0';
                    eof_flags[j] = 1;
                }
            }
        }
        
        // 如果所有文件都结束了，退出
        if (all_eof) {
            break;
        }
        
        // 输出合并的行
        if (has_data) {
            for (int j = 0; j < file_count; j++) {
                if (j > 0) {
                    putchar(delimiter);
                }
                printf("%s", lines[j]);
            }
            putchar('\n');
        }
    }
    
    // 关闭文件
    for (int j = 0; j < file_count; j++) {
        if (files[j] != stdin) {
            fclose(files[j]);
        }
    }
    
    return 0;
}



