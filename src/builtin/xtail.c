/*
 * xtail.c - 显示文件的后 N 行
 * 
 * 功能：类似于 tail 命令，显示文件末尾部分
 * 用法：xtail [选项] [file]...
 * 
 * 选项：
 *   -n N  显示后 N 行（默认 10 行）
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096

// 循环缓冲区来存储最后 N 行
typedef struct {
    char** lines;
    int capacity;
    int count;
    int start;
} CircularBuffer;

// 创建循环缓冲区
static CircularBuffer* create_buffer(int capacity) {
    CircularBuffer* buf = malloc(sizeof(CircularBuffer));
    if (!buf) return NULL;
    
    buf->lines = calloc(capacity, sizeof(char*));
    if (!buf->lines) {
        free(buf);
        return NULL;
    }
    
    for (int i = 0; i < capacity; i++) {
        buf->lines[i] = malloc(MAX_LINE_LENGTH);
        if (!buf->lines[i]) {
            for (int j = 0; j < i; j++) {
                free(buf->lines[j]);
            }
            free(buf->lines);
            free(buf);
            return NULL;
        }
    }
    
    buf->capacity = capacity;
    buf->count = 0;
    buf->start = 0;
    
    return buf;
}

// 释放循环缓冲区
static void free_buffer(CircularBuffer* buf) {
    if (!buf) return;
    
    for (int i = 0; i < buf->capacity; i++) {
        free(buf->lines[i]);
    }
    free(buf->lines);
    free(buf);
}

// 添加行到循环缓冲区
static void add_line(CircularBuffer* buf, const char* line) {
    int index = (buf->start + buf->count) % buf->capacity;
    strncpy(buf->lines[index], line, MAX_LINE_LENGTH - 1);
    buf->lines[index][MAX_LINE_LENGTH - 1] = '\0';
    
    if (buf->count < buf->capacity) {
        buf->count++;
    } else {
        buf->start = (buf->start + 1) % buf->capacity;
    }
}

// 打印缓冲区内容
static void print_buffer(const CircularBuffer* buf) {
    for (int i = 0; i < buf->count; i++) {
        int index = (buf->start + i) % buf->capacity;
        printf("%s", buf->lines[index]);
    }
}

// 显示文件的后 N 行
static int tail_file(const char* filename, int num_lines, int show_header, ShellContext *ctx) {
    FILE* file;
    CircularBuffer* buf;
    char line[MAX_LINE_LENGTH];
    
    // 创建循环缓冲区
    buf = create_buffer(num_lines);
    if (!buf) {
        XSHELL_LOG_ERROR(ctx, "xtail: memory allocation failed\n");
        return -1;
    }
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
        filename = "(standard input)";
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xtail: %s: %s\n", filename, strerror(errno));
            free_buffer(buf);
            return -1;
        }
    }
    
    // 读取所有行，保持最后 N 行在缓冲区中
    while (fgets(line, sizeof(line), file)) {
        add_line(buf, line);
    }
    
    // 显示文件名头部（多个文件时）
    if (show_header) {
        printf("==> %s <==\n", filename);
    }
    
    // 打印最后 N 行
    print_buffer(buf);
    
    // 关闭文件并释放缓冲区
    if (file != stdin) {
        fclose(file);
    }
    free_buffer(buf);
    
    return 0;
}

int cmd_xtail(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtail - 显示文件的后 N 行\n\n");
        printf("用法:\n");
        printf("  xtail [选项] [file]...\n");
        printf("  xtail [选项]               # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  显示文件的末尾部分（默认后 10 行）。\n");
        printf("  Tail - 尾部。\n\n");
        printf("参数:\n");
        printf("  file      要显示的文件（可以多个）\n");
        printf("            不指定文件则从标准输入读取\n");
        printf("            使用 - 表示标准输入\n\n");
        printf("选项:\n");
        printf("  -n N      显示后 N 行（默认 10）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xtail file.txt             # 显示后 10 行\n");
        printf("  xtail -n 5 file.txt        # 显示后 5 行\n");
        printf("  xtail -n 20 log.txt        # 显示日志文件的后 20 行\n");
        printf("  xtail -n 20 *.log          # 显示多个日志文件的后 20 行\n");
        printf("  xcat file.txt | xtail      # 从管道读取\n");
        printf("  xcat file.txt | xtail -n 3 # 显示管道输入的后 3 行\n\n");
        printf("多个文件:\n");
        printf("  当指定多个文件时，会在每个文件内容前显示文件名：\n");
        printf("  ==> file1.txt <==\n");
        printf("  （文件内容）\n\n");
        printf("  ==> file2.txt <==\n");
        printf("  （文件内容）\n\n");
        printf("常见用途:\n");
        printf("  • 查看日志文件的最新内容\n");
        printf("  • 检查大文件的末尾部分\n");
        printf("  • 与其他命令配合使用\n\n");
        printf("对应系统命令: tail\n");
        return 0;
    }
    
    // 默认显示行数
    int num_lines = 10;
    int start_index = 1;
    
    // 解析 -n 选项
    if (start_index < cmd->arg_count && strcmp(cmd->args[start_index], "-n") == 0) {
        start_index++;
        if (start_index >= cmd->arg_count) {
            XSHELL_LOG_ERROR(ctx, "xtail: option requires an argument -- 'n'\n");
            XSHELL_LOG_ERROR(ctx, "Try 'xtail --help' for more information.\n");
            return -1;
        }
        
        num_lines = atoi(cmd->args[start_index]);
        if (num_lines <= 0) {
            XSHELL_LOG_ERROR(ctx, "xtail: invalid number of lines: '%s'\n",
                             cmd->args[start_index]);
            return -1;
        }
        start_index++;
    }
    
    // 如果没有指定文件，从标准输入读取
    if (start_index >= cmd->arg_count) {
        return tail_file("-", num_lines, 0, ctx);
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
        
        if (tail_file(cmd->args[i], num_lines, show_header, ctx) != 0) {
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}




