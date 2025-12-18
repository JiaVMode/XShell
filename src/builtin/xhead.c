/*
 * xhead.c - 显示文件的前 N 行
 * 
 * 功能：类似于 head 命令，显示文件开头部分
 * 用法：xhead [选项] [file]...
 * 
 * 选项：
 *   -n N  显示前 N 行（默认 10 行）
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// 显示文件的前 N 行
static int head_file(const char* filename, int num_lines, int show_header, ShellContext *ctx) {
    FILE* file;
    char line[4096];
    int line_count = 0;
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
        filename = "(standard input)";
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xhead: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }
    
    // 显示文件名头部（多个文件时）
    if (show_header) {
        printf("==> %s <==\n", filename);
    }
    
    // 读取并显示前 N 行
    while (line_count < num_lines && fgets(line, sizeof(line), file)) {
        printf("%s", line);
        line_count++;
    }
    
    // 关闭文件
    if (file != stdin) {
        fclose(file);
    }
    
    return 0;
}

int cmd_xhead(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xhead - 显示文件的前 N 行\n\n");
        printf("用法:\n");
        printf("  xhead [选项] [file]...\n");
        printf("  xhead [选项]               # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  显示文件的开头部分（默认前 10 行）。\n");
        printf("  Head - 头部。\n\n");
        printf("参数:\n");
        printf("  file      要显示的文件（可以多个）\n");
        printf("            不指定文件则从标准输入读取\n");
        printf("            使用 - 表示标准输入\n\n");
        printf("选项:\n");
        printf("  -n N      显示前 N 行（默认 10）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xhead file.txt             # 显示前 10 行\n");
        printf("  xhead -n 5 file.txt        # 显示前 5 行\n");
        printf("  xhead -n 20 *.txt          # 显示多个文件的前 20 行\n");
        printf("  xcat file.txt | xhead      # 从管道读取\n");
        printf("  xcat file.txt | xhead -n 3 # 显示管道输入的前 3 行\n\n");
        printf("多个文件:\n");
        printf("  当指定多个文件时，会在每个文件内容前显示文件名：\n");
        printf("  ==> file1.txt <==\n");
        printf("  （文件内容）\n\n");
        printf("  ==> file2.txt <==\n");
        printf("  （文件内容）\n\n");
        printf("对应系统命令: head\n");
        return 0;
    }
    
    // 默认显示行数
    int num_lines = 10;
    int start_index = 1;
    
    // 解析 -n 选项
    if (start_index < cmd->arg_count && strcmp(cmd->args[start_index], "-n") == 0) {
        start_index++;
        if (start_index >= cmd->arg_count) {
            XSHELL_LOG_ERROR(ctx, "xhead: option requires an argument -- 'n'\n");
            XSHELL_LOG_ERROR(ctx, "Try 'xhead --help' for more information.\n");
            return -1;
        }
        
        num_lines = atoi(cmd->args[start_index]);
        if (num_lines <= 0) {
            XSHELL_LOG_ERROR(ctx, "xhead: invalid number of lines: '%s'\n",
                             cmd->args[start_index]);
            return -1;
        }
        start_index++;
    }
    
    // 如果没有指定文件，从标准输入读取
    if (start_index >= cmd->arg_count) {
        return head_file("-", num_lines, 0, ctx);
    }
    
    // 处理多个文件
    int has_error = 0;
    int file_count = cmd->arg_count - start_index;
    
    for (int i = start_index; i < cmd->arg_count; i++) {
        // 多个文件时显示文件名，文件之间空一行
        int show_header = (file_count > 1);
        if (i > start_index && show_header) {
            printf("\n");
        }
        
        if (head_file(cmd->args[i], num_lines, show_header, ctx) != 0) {
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}




