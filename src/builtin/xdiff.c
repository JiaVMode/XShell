/*
 * xdiff.c - 比较两个文件的差异
 * 
 * 功能：类似于 diff 命令，比较两个文件并显示差异
 * 用法：xdiff [选项] <file1> <file2>
 * 
 * 选项：
 *   -u    统一格式输出（unified format）
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096
#define MAX_LINES 10000

// 选项结构体
typedef struct {
    int unified;    // -u 统一格式输出
} DiffOptions;

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件1> <文件2>\n", cmd_name);
    printf("功能: 比较两个文件的差异\n");
    printf("选项:\n");
    printf("  -u, --unified    统一格式输出（显示上下文）\n");
    printf("  -h, --help       显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s file1.txt file2.txt\n", cmd_name);
    printf("  %s -u file1.txt file2.txt\n", cmd_name);
}

// 读取文件到内存
static int read_file_lines(const char *filename, char ***lines, int *line_count, ShellContext *ctx) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    char **line_array = NULL;
    int count = 0;
    int capacity = 100;
    
    // 打开文件
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (file == NULL) {
            XSHELL_LOG_ERROR(ctx, "xdiff: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }
    
    // 分配初始内存
    line_array = (char**)malloc(capacity * sizeof(char*));
    if (line_array == NULL) {
        if (file != stdin) fclose(file);
        XSHELL_LOG_PERROR(ctx, "xdiff");
        return -1;
    }
    
    // 读取所有行
    while (fgets(line, sizeof(line), file) != NULL) {
        // 如果容量不足，扩展
        if (count >= capacity) {
            capacity *= 2;
            char **new_array = (char**)realloc(line_array, capacity * sizeof(char*));
            if (new_array == NULL) {
                // 释放已分配的内存
                for (int i = 0; i < count; i++) {
                    free(line_array[i]);
                }
                free(line_array);
                if (file != stdin) fclose(file);
                XSHELL_LOG_PERROR(ctx, "xdiff");
                return -1;
            }
            line_array = new_array;
        }
        
        // 分配并复制行
        size_t len = strlen(line);
        line_array[count] = (char*)malloc(len + 1);
        if (line_array[count] == NULL) {
            // 释放已分配的内存
            for (int i = 0; i < count; i++) {
                free(line_array[i]);
            }
            free(line_array);
            if (file != stdin) fclose(file);
            XSHELL_LOG_PERROR(ctx, "xdiff");
            return -1;
        }
        strcpy(line_array[count], line);
        count++;
        
        // 限制最大行数
        if (count >= MAX_LINES) {
            XSHELL_LOG_ERROR(ctx, "xdiff: 警告: 文件行数超过 %d 行，只比较前 %d 行\n", MAX_LINES, MAX_LINES);
            break;
        }
    }
    
    if (file != stdin) {
        fclose(file);
    }
    
    *lines = line_array;
    *line_count = count;
    return 0;
}

// 释放行数组内存
static void free_lines(char **lines, int count) {
    if (lines == NULL) return;
    for (int i = 0; i < count; i++) {
        free(lines[i]);
    }
    free(lines);
}

// 移除行尾换行符
static void remove_newline(char *line) {
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
}

// 简单格式输出差异
static void print_simple_diff(char **lines1, int count1, char **lines2, int count2,
                              const char *file1, const char *file2) {
    int i = 0, j = 0;
    int diff_count = 0;
    
    printf("--- %s\n", file1);
    printf("+++ %s\n", file2);
    
    while (i < count1 || j < count2) {
        if (i >= count1) {
            // 文件1已结束，文件2还有剩余
            printf("+%d: %s", j + 1, lines2[j]);
            if (lines2[j][strlen(lines2[j]) - 1] != '\n') {
                printf("\n");
            }
            j++;
            diff_count++;
        } else if (j >= count2) {
            // 文件2已结束，文件1还有剩余
            printf("-%d: %s", i + 1, lines1[i]);
            if (lines1[i][strlen(lines1[i]) - 1] != '\n') {
                printf("\n");
            }
            i++;
            diff_count++;
        } else {
            // 比较当前行
            char line1[MAX_LINE_LENGTH];
            char line2[MAX_LINE_LENGTH];
            strcpy(line1, lines1[i]);
            strcpy(line2, lines2[j]);
            remove_newline(line1);
            remove_newline(line2);
            
            if (strcmp(line1, line2) == 0) {
                // 行相同，都前进
                i++;
                j++;
            } else {
                // 行不同，显示差异
                printf("-%d: %s\n", i + 1, line1);
                printf("+%d: %s\n", j + 1, line2);
                i++;
                j++;
                diff_count++;
            }
        }
    }
    
    if (diff_count == 0) {
        printf("文件相同，无差异\n");
    }
}

// 统一格式输出差异
static void print_unified_diff(char **lines1, int count1, char **lines2, int count2,
                               const char *file1, const char *file2) {
    int i = 0, j = 0;
    int in_hunk = 0;
    
    printf("--- %s\n", file1);
    printf("+++ %s\n", file2);
    
    // 简化的统一格式：逐行比较
    while (i < count1 || j < count2) {
        if (i >= count1) {
            // 文件1已结束
            if (!in_hunk) {
                printf("@@ -%d,0 +%d,1 @@\n", i + 1, j + 1);
                in_hunk = 1;
            }
            printf("+%s", lines2[j]);
            if (lines2[j][strlen(lines2[j]) - 1] != '\n') {
                printf("\n");
            }
            j++;
        } else if (j >= count2) {
            // 文件2已结束
            if (!in_hunk) {
                printf("@@ -%d,1 +%d,0 @@\n", i + 1, j + 1);
                in_hunk = 1;
            }
            printf("-%s", lines1[i]);
            if (lines1[i][strlen(lines1[i]) - 1] != '\n') {
                printf("\n");
            }
            i++;
        } else {
            char line1[MAX_LINE_LENGTH];
            char line2[MAX_LINE_LENGTH];
            strcpy(line1, lines1[i]);
            strcpy(line2, lines2[j]);
            remove_newline(line1);
            remove_newline(line2);
            
            if (strcmp(line1, line2) == 0) {
                // 行相同
                if (in_hunk) {
                    // 结束当前 hunk
                    in_hunk = 0;
                }
                i++;
                j++;
            } else {
                // 行不同
                if (!in_hunk) {
                    printf("@@ -%d,1 +%d,1 @@\n", i + 1, j + 1);
                    in_hunk = 1;
                }
                printf("-%s\n", line1);
                printf("+%s\n", line2);
                i++;
                j++;
            }
        }
    }
    
    // 检查是否所有行都相同
    if (i == count1 && j == count2 && !in_hunk) {
        printf("文件相同，无差异\n");
    }
}

// xdiff 命令实现
int cmd_xdiff(Command *cmd, ShellContext *ctx) {
    DiffOptions opts = {0};
    const char *file1 = NULL;
    const char *file2 = NULL;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    // 解析选项和文件名
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-u") == 0 || strcmp(cmd->args[i], "--unified") == 0) {
            opts.unified = 1;
        } else if (file1 == NULL) {
            file1 = cmd->args[i];
        } else if (file2 == NULL) {
            file2 = cmd->args[i];
        } else {
            XSHELL_LOG_ERROR(ctx, "xdiff: 错误: 只能比较两个文件\n");
            return -1;
        }
    }
    
    // 检查是否提供了两个文件
    if (file1 == NULL || file2 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xdiff: 错误: 需要指定两个文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 读取两个文件
    char **lines1 = NULL, **lines2 = NULL;
    int count1 = 0, count2 = 0;
    
    if (read_file_lines(file1, &lines1, &count1, ctx) != 0) {
        return -1;
    }
    
    if (read_file_lines(file2, &lines2, &count2, ctx) != 0) {
        free_lines(lines1, count1);
        return -1;
    }
    
    // 输出差异
    if (opts.unified) {
        print_unified_diff(lines1, count1, lines2, count2, file1, file2);
    } else {
        print_simple_diff(lines1, count1, lines2, count2, file1, file2);
    }
    
    // 释放内存
    free_lines(lines1, count1);
    free_lines(lines2, count2);
    
    return 0;
}

