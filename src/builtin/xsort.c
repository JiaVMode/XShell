/*
 * xsort.c - 排序文件内容
 * 
 * 功能：类似于 sort 命令，对文件行进行排序
 * 用法：xsort [选项] [file]...
 * 
 * 选项：
 *   -r    逆序排序
 *   -n    按数值排序
 *   -u    去除重复行（unique）
 *   --help 显示帮助信息
 */

#define _POSIX_C_SOURCE 200809L  // 启用 strdup 函数

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_LINES 100000
#define MAX_LINE_LENGTH 4096

// 选项结构体
typedef struct {
    int reverse;    // -r 逆序排序
    int numeric;    // -n 数值排序
    int unique;     // -u 去除重复
} SortOptions;

// 字符串比较函数（正序）
static int str_compare(const void* a, const void* b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// 字符串比较函数（逆序）
static int str_compare_reverse(const void* a, const void* b) {
    return strcmp(*(const char**)b, *(const char**)a);
}

// 提取字符串中的数字（跳过引号和空白字符）
static double extract_number(const char* str) {
    // 跳过开头的空白字符和引号
    while (*str && (*str == ' ' || *str == '\t' || *str == '"' || *str == '\'')) {
        str++;
    }
    return atof(str);
}

// 数值比较函数（正序）
static int num_compare(const void* a, const void* b) {
    double num_a = extract_number(*(const char**)a);
    double num_b = extract_number(*(const char**)b);
    
    if (num_a < num_b) return -1;
    if (num_a > num_b) return 1;
    return 0;
}

// 数值比较函数（逆序）
static int num_compare_reverse(const void* a, const void* b) {
    return num_compare(b, a);
}

// 排序文件
static int sort_file(const char* filename, const SortOptions* opts, ShellContext *ctx) {
    FILE* file;
    char** lines = NULL;
    int line_count = 0;
    int capacity = 1000;
    
    // 分配初始行数组
    lines = malloc(capacity * sizeof(char*));
    if (!lines) {
        XSHELL_LOG_ERROR(ctx, "xsort: memory allocation failed\n");
        return -1;
    }
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xsort: %s: %s\n", filename, strerror(errno));
            free(lines);
            return -1;
        }
    }
    
    // 读取所有行
    char line[MAX_LINE_LENGTH];
    while (fgets(line, sizeof(line), file)) {
        // 扩展数组容量
        if (line_count >= capacity) {
            capacity *= 2;
            if (capacity > MAX_LINES) {
                XSHELL_LOG_ERROR(ctx, "xsort: too many lines (max %d)\n", MAX_LINES);
                break;
            }
            char** new_lines = realloc(lines, capacity * sizeof(char*));
            if (!new_lines) {
                XSHELL_LOG_ERROR(ctx, "xsort: memory allocation failed\n");
                for (int i = 0; i < line_count; i++) {
                    free(lines[i]);
                }
                free(lines);
                if (file != stdin) fclose(file);
                return -1;
            }
            lines = new_lines;
        }
        
        // 复制行内容
        lines[line_count] = strdup(line);
        if (!lines[line_count]) {
            XSHELL_LOG_ERROR(ctx, "xsort: memory allocation failed\n");
            for (int i = 0; i < line_count; i++) {
                free(lines[i]);
            }
            free(lines);
            if (file != stdin) fclose(file);
            return -1;
        }
        line_count++;
    }
    
    // 关闭文件
    if (file != stdin) {
        fclose(file);
    }
    
    // 排序
    if (opts->numeric) {
        if (opts->reverse) {
            qsort(lines, line_count, sizeof(char*), num_compare_reverse);
        } else {
            qsort(lines, line_count, sizeof(char*), num_compare);
        }
    } else {
        if (opts->reverse) {
            qsort(lines, line_count, sizeof(char*), str_compare_reverse);
        } else {
            qsort(lines, line_count, sizeof(char*), str_compare);
        }
    }
    
    // 输出排序结果
    for (int i = 0; i < line_count; i++) {
        // 如果启用 unique，跳过与上一行相同的行
        if (opts->unique && i > 0 && strcmp(lines[i], lines[i-1]) == 0) {
            continue;
        }
        printf("%s", lines[i]);
    }
    
    // 释放内存
    for (int i = 0; i < line_count; i++) {
        free(lines[i]);
    }
    free(lines);
    
    return 0;
}

int cmd_xsort(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xsort - 排序文件内容\n\n");
        printf("用法:\n");
        printf("  xsort [选项] [file]...\n");
        printf("  xsort [选项]               # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  对文件的行进行排序。\n");
        printf("  Sort - 排序。\n\n");
        printf("参数:\n");
        printf("  file      要排序的文件\n");
        printf("            不指定文件则从标准输入读取\n");
        printf("            多个文件会被合并后排序\n\n");
        printf("选项:\n");
        printf("  -r        逆序排序（从大到小）\n");
        printf("  -n        按数值排序\n");
        printf("  -u        去除重复行（unique）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("排序规则:\n");
        printf("  默认排序：  按字典顺序（ASCII码）\n");
        printf("  数值排序：  将每行开头解析为数字\n");
        printf("  逆序排序：  从大到小排序\n");
        printf("  去重排序：  输出时跳过连续重复的行\n\n");
        printf("示例:\n");
        printf("  xsort file.txt             # 正序排序\n");
        printf("  xsort -r file.txt          # 逆序排序\n");
        printf("  xsort -n numbers.txt       # 数值排序\n");
        printf("  xsort -u file.txt          # 排序并去重\n");
        printf("  xsort -rn numbers.txt      # 数值逆序排序\n");
        printf("  xsort -un file.txt         # 数值排序并去重\n");
        printf("  xecho -e \"3\\n1\\n2\" | xsort  # 从管道读取\n");
        printf("  xcat *.txt | xsort -u      # 合并多个文件并去重\n\n");
        printf("性能限制:\n");
        printf("  最大行数：%d 行\n", MAX_LINES);
        printf("  最大行长：%d 字节\n\n", MAX_LINE_LENGTH);
        printf("对应系统命令: sort\n");
        return 0;
    }
    
    // 解析选项
    SortOptions opts = {0};
    int start_index = 1;
    
    while (start_index < cmd->arg_count && cmd->args[start_index][0] == '-'
           && strcmp(cmd->args[start_index], "-") != 0) {
        const char* arg = cmd->args[start_index];
        
        if (strcmp(arg, "--help") == 0) {
            start_index++;
            continue;
        }
        
        // 解析单字符选项（支持组合如 -rn）
        for (int i = 1; arg[i]; i++) {
            switch (arg[i]) {
                case 'r': opts.reverse = 1; break;
                case 'n': opts.numeric = 1; break;
                case 'u': opts.unique = 1; break;
                default:
                    XSHELL_LOG_ERROR(ctx, "xsort: invalid option: '-%c'\n", arg[i]);
                    XSHELL_LOG_ERROR(ctx, "Try 'xsort --help' for more information.\n");
                    return -1;
            }
        }
        start_index++;
    }
    
    // 如果没有指定文件，从标准输入读取
    if (start_index >= cmd->arg_count) {
        return sort_file("-", &opts, ctx);
    }
    
    // 如果只有一个文件，直接排序
    if (start_index == cmd->arg_count - 1) {
        return sort_file(cmd->args[start_index], &opts, ctx);
    }
    
    // 多个文件：合并所有内容后排序
    // 创建临时缓冲区存储所有行
    char** all_lines = malloc(MAX_LINES * sizeof(char*));
    if (!all_lines) {
        XSHELL_LOG_ERROR(ctx, "xsort: memory allocation failed\n");
        return -1;
    }
    
    int total_lines = 0;
    int has_error = 0;
    
    // 读取所有文件
    for (int i = start_index; i < cmd->arg_count; i++) {
        FILE* file = fopen(cmd->args[i], "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xsort: %s: %s\n", cmd->args[i], strerror(errno));
            has_error = 1;
            continue;
        }
        
        char line[MAX_LINE_LENGTH];
        while (fgets(line, sizeof(line), file) && total_lines < MAX_LINES) {
            all_lines[total_lines] = strdup(line);
            if (!all_lines[total_lines]) {
                XSHELL_LOG_ERROR(ctx, "xsort: memory allocation failed\n");
                fclose(file);
                has_error = 1;
                break;
            }
            total_lines++;
        }
        fclose(file);
    }
    
    // 排序
    if (opts.numeric) {
        if (opts.reverse) {
            qsort(all_lines, total_lines, sizeof(char*), num_compare_reverse);
        } else {
            qsort(all_lines, total_lines, sizeof(char*), num_compare);
        }
    } else {
        if (opts.reverse) {
            qsort(all_lines, total_lines, sizeof(char*), str_compare_reverse);
        } else {
            qsort(all_lines, total_lines, sizeof(char*), str_compare);
        }
    }
    
    // 输出
    for (int i = 0; i < total_lines; i++) {
        if (opts.unique && i > 0 && strcmp(all_lines[i], all_lines[i-1]) == 0) {
            continue;
        }
        printf("%s", all_lines[i]);
    }
    
    // 释放内存
    for (int i = 0; i < total_lines; i++) {
        free(all_lines[i]);
    }
    free(all_lines);
    
    return has_error ? -1 : 0;
}

