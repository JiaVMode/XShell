/*
 * xjoin.c - 连接文件
 * 
 * 功能：基于共同字段连接两个文件（类似SQL JOIN）
 * 用法：xjoin [选项] <文件1> <文件2>
 * 
 * 选项：
 *   -1 <字段>  指定文件1的连接字段（默认1）
 *   -2 <字段>  指定文件2的连接字段（默认1）
 *   -t <分隔符> 指定字段分隔符（默认空白）
 *   --help     显示帮助信息
 * 
 * 注意：简化实现，文件需要已排序
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096
#define MAX_FIELDS 100

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件1> <文件2>\n", cmd_name);
    printf("功能: 基于共同字段连接两个文件（类似SQL JOIN）\n");
    printf("选项:\n");
    printf("  -1 <字段>      指定文件1的连接字段（默认1）\n");
    printf("  -2 <字段>      指定文件2的连接字段（默认1）\n");
    printf("  -t <分隔符>    指定字段分隔符（默认空白）\n");
    printf("  -h, --help    显示此帮助信息\n");
    printf("注意: 文件需要已按连接字段排序\n");
    printf("示例:\n");
    printf("  %s file1.txt file2.txt\n", cmd_name);
    printf("  %s -1 2 -2 1 file1.txt file2.txt\n", cmd_name);
}

// 分割行到字段
static int split_fields(const char *line, char delimiter, char **fields, int max_fields) {
    int count = 0;
    const char *p = line;
    char *field_start = (char*)line;
    
    // 跳过前导空白（如果分隔符是空白）
    if (delimiter == ' ') {
        while (*p == ' ' || *p == '\t') p++;
        field_start = (char*)p;
    }
    
    while (*p != '\0' && count < max_fields - 1) {
        if (*p == delimiter || (delimiter == ' ' && (*p == ' ' || *p == '\t'))) {
            size_t len = p - field_start;
            fields[count] = (char*)malloc(len + 1);
            if (fields[count] == NULL) {
                // 释放已分配的内存
                for (int i = 0; i < count; i++) {
                    free(fields[i]);
                }
                return -1;
            }
            strncpy(fields[count], field_start, len);
            fields[count][len] = '\0';
            count++;
            
            // 跳过分隔符和后续空白
            p++;
            if (delimiter == ' ') {
                while (*p == ' ' || *p == '\t') p++;
            }
            field_start = (char*)p;
        } else {
            p++;
        }
    }
    
    // 最后一个字段
    if (field_start < p) {
        size_t len = p - field_start;
        fields[count] = (char*)malloc(len + 1);
        if (fields[count] == NULL) {
            for (int i = 0; i < count; i++) {
                free(fields[i]);
            }
            return -1;
        }
        strncpy(fields[count], field_start, len);
        fields[count][len] = '\0';
        count++;
    }
    
    fields[count] = NULL;
    return count;
}

// 释放字段数组
static void free_fields(char **fields) {
    if (fields == NULL) return;
    for (int i = 0; fields[i] != NULL; i++) {
        free(fields[i]);
    }
}

// 读取并解析一行
static int read_and_parse(FILE *file, char delimiter, int join_field __attribute__((unused)), 
                          char *line_buf, char **fields, int *field_count) {
    if (fgets(line_buf, MAX_LINE_LENGTH, file) == NULL) {
        return 0;  // EOF
    }
    
    // 移除换行符
    size_t len = strlen(line_buf);
    if (len > 0 && line_buf[len - 1] == '\n') {
        line_buf[len - 1] = '\0';
    }
    
    *field_count = split_fields(line_buf, delimiter, fields, MAX_FIELDS);
    return 1;
}

// xjoin 命令实现
int cmd_xjoin(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    int field1 = 1;  // 文件1的连接字段（1-based）
    int field2 = 1;  // 文件2的连接字段（1-based）
    char delimiter = ' ';  // 默认空白分隔符
    const char *file1 = NULL;
    const char *file2 = NULL;
    
    // 解析参数
    if (cmd->arg_count < 3) {
        show_help(cmd->name);
        return 0;
    }
    
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-1") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xjoin: 错误: -1 选项需要参数\n");
                return -1;
            }
            field1 = atoi(cmd->args[i + 1]);
            if (field1 < 1) {
                XSHELL_LOG_ERROR(ctx, "xjoin: 错误: 无效的字段号\n");
                return -1;
            }
            i += 2;
        } else if (strcmp(cmd->args[i], "-2") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xjoin: 错误: -2 选项需要参数\n");
                return -1;
            }
            field2 = atoi(cmd->args[i + 1]);
            if (field2 < 1) {
                XSHELL_LOG_ERROR(ctx, "xjoin: 错误: 无效的字段号\n");
                return -1;
            }
            i += 2;
        } else if (strcmp(cmd->args[i], "-t") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xjoin: 错误: -t 选项需要参数\n");
                return -1;
            }
            delimiter = cmd->args[i + 1][0];
            i += 2;
        } else if (file1 == NULL) {
            file1 = cmd->args[i];
            i++;
        } else if (file2 == NULL) {
            file2 = cmd->args[i];
            i++;
        } else {
            i++;
        }
    }
    
    if (file1 == NULL || file2 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xjoin: 错误: 需要指定两个文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 打开文件
    FILE *f1 = fopen(file1, "r");
    if (f1 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xjoin: %s: %s\n", file1, strerror(errno));
        return -1;
    }
    
    FILE *f2 = fopen(file2, "r");
    if (f2 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xjoin: %s: %s\n", file2, strerror(errno));
        fclose(f1);
        return -1;
    }
    
    // 连接文件
    char line1[MAX_LINE_LENGTH], line2[MAX_LINE_LENGTH];
    char *fields1[MAX_FIELDS], *fields2[MAX_FIELDS];
    int count1 = 0, count2 = 0;
    int has_line1 = 0, has_line2 = 0;
    
    // 读取第一行
    has_line1 = read_and_parse(f1, delimiter, field1, line1, fields1, &count1);
    has_line2 = read_and_parse(f2, delimiter, field2, line2, fields2, &count2);
    
    while (has_line1 && has_line2) {
        if (count1 < field1 || count2 < field2) {
            // 字段不足，跳过
            if (count1 < field1) {
                free_fields(fields1);
                has_line1 = read_and_parse(f1, delimiter, field1, line1, fields1, &count1);
            }
            if (count2 < field2) {
                free_fields(fields2);
                has_line2 = read_and_parse(f2, delimiter, field2, line2, fields2, &count2);
            }
            continue;
        }
        
        const char *key1 = fields1[field1 - 1];
        const char *key2 = fields2[field2 - 1];
        int cmp = strcmp(key1, key2);
        
        if (cmp < 0) {
            // 文件1的键较小，读取下一行
            free_fields(fields1);
            fields1[0] = NULL;  // 重置指针
            has_line1 = read_and_parse(f1, delimiter, field1, line1, fields1, &count1);
        } else if (cmp > 0) {
            // 文件2的键较小，读取下一行
            free_fields(fields2);
            fields2[0] = NULL;  // 重置指针
            has_line2 = read_and_parse(f2, delimiter, field2, line2, fields2, &count2);
        } else {
            // 匹配，输出连接结果
            printf("%s", line1);
            if (delimiter != ' ') {
                putchar(delimiter);
            } else {
                putchar(' ');
            }
            printf("%s\n", line2);
            
            // 读取下一行（简化：只读取文件1的下一行）
            free_fields(fields1);
            fields1[0] = NULL;  // 重置指针
            has_line1 = read_and_parse(f1, delimiter, field1, line1, fields1, &count1);
        }
    }
    
    // 清理
    if (has_line1) {
        free_fields(fields1);
    }
    if (has_line2) {
        free_fields(fields2);
    }
    fclose(f1);
    fclose(f2);
    
    return 0;
}

