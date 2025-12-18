/*
 * xcomm.c - 比较排序文件
 * 
 * 功能：比较两个已排序的文件，显示共同行和独有行
 * 用法：xcomm [选项] <文件1> <文件2>
 * 
 * 选项：
 *   -1    隐藏文件1独有的行
 *   -2    隐藏文件2独有的行
 *   -3    隐藏共同行
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件1> <文件2>\n", cmd_name);
    printf("功能: 比较两个已排序的文件，显示共同行和独有行\n");
    printf("选项:\n");
    printf("  -1              隐藏文件1独有的行\n");
    printf("  -2              隐藏文件2独有的行\n");
    printf("  -3              隐藏共同行\n");
    printf("  -h, --help      显示此帮助信息\n");
    printf("输出格式: 三列输出（文件1独有、文件2独有、共同行）\n");
    printf("示例:\n");
    printf("  %s file1.txt file2.txt\n", cmd_name);
    printf("  %s -12 file1.txt file2.txt  # 只显示共同行\n", cmd_name);
}

// 读取一行（移除换行符）
static int read_line(FILE *file, char *line, size_t size) {
    if (fgets(line, size, file) == NULL) {
        return 0;  // EOF
    }
    
    // 移除换行符
    size_t len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') {
        line[len - 1] = '\0';
    }
    
    return 1;
}

// xcomm 命令实现
int cmd_xcomm(Command *cmd, ShellContext *ctx) {
    int hide_col1 = 0;  // -1
    int hide_col2 = 0;  // -2
    int hide_col3 = 0;  // -3
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
            hide_col1 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-2") == 0) {
            hide_col2 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-3") == 0) {
            hide_col3 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-12") == 0) {
            hide_col1 = 1;
            hide_col2 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-13") == 0) {
            hide_col1 = 1;
            hide_col3 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-23") == 0) {
            hide_col2 = 1;
            hide_col3 = 1;
            i++;
        } else if (strcmp(cmd->args[i], "-123") == 0) {
            hide_col1 = 1;
            hide_col2 = 1;
            hide_col3 = 1;
            i++;
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
        XSHELL_LOG_ERROR(ctx, "xcomm: 错误: 需要指定两个文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 打开文件
    FILE *f1, *f2;
    
    if (strcmp(file1, "-") == 0) {
        f1 = stdin;
    } else {
        f1 = fopen(file1, "r");
        if (f1 == NULL) {
            XSHELL_LOG_ERROR(ctx, "xcomm: %s: %s\n", file1, strerror(errno));
            return -1;
        }
    }
    
    if (strcmp(file2, "-") == 0) {
        f2 = stdin;
    } else {
        f2 = fopen(file2, "r");
        if (f2 == NULL) {
            XSHELL_LOG_ERROR(ctx, "xcomm: %s: %s\n", file2, strerror(errno));
            if (f1 != stdin) fclose(f1);
            return -1;
        }
    }
    
    // 比较文件
    char line1[MAX_LINE_LENGTH];
    char line2[MAX_LINE_LENGTH];
    int has_line1 = read_line(f1, line1, sizeof(line1));
    int has_line2 = read_line(f2, line2, sizeof(line2));
    
    while (has_line1 || has_line2) {
        if (!has_line1) {
            // 文件1结束，文件2还有剩余
            if (!hide_col2) {
                printf("\t%s\n", line2);
            }
            has_line2 = read_line(f2, line2, sizeof(line2));
        } else if (!has_line2) {
            // 文件2结束，文件1还有剩余
            if (!hide_col1) {
                printf("%s\n", line1);
            }
            has_line1 = read_line(f1, line1, sizeof(line1));
        } else {
            int cmp = strcmp(line1, line2);
            if (cmp < 0) {
                // 文件1独有
                if (!hide_col1) {
                    printf("%s\n", line1);
                }
                has_line1 = read_line(f1, line1, sizeof(line1));
            } else if (cmp > 0) {
                // 文件2独有
                if (!hide_col2) {
                    printf("\t%s\n", line2);
                }
                has_line2 = read_line(f2, line2, sizeof(line2));
            } else {
                // 共同行
                if (!hide_col3) {
                    printf("\t\t%s\n", line1);
                }
                has_line1 = read_line(f1, line1, sizeof(line1));
                has_line2 = read_line(f2, line2, sizeof(line2));
            }
        }
    }
    
    // 关闭文件
    if (f1 != stdin) fclose(f1);
    if (f2 != stdin) fclose(f2);
    
    return 0;
}



