/*
 * xsplit.c - 分割文件
 * 
 * 功能：将大文件分割成多个小文件
 * 用法：xsplit [选项] <文件> [前缀]
 * 
 * 选项：
 *   -l <行数>  按行数分割（每N行一个文件）
 *   -b <大小>  按大小分割（每N字节一个文件，支持K/M后缀）
 *   --help     显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096
#define DEFAULT_PREFIX "x"

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件> [前缀]\n", cmd_name);
    printf("功能: 将大文件分割成多个小文件\n");
    printf("选项:\n");
    printf("  -l <行数>      按行数分割（每N行一个文件）\n");
    printf("  -b <大小>      按大小分割（每N字节一个文件，支持K/M后缀）\n");
    printf("  -h, --help    显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s -l 1000 large.txt\n", cmd_name);
    printf("  %s -b 1M large.txt\n", cmd_name);
    printf("  %s -l 100 file.txt part\n", cmd_name);
}

// 解析大小（支持K/M后缀）
static long long parse_size(const char *str) {
    long long size = 0;
    const char *p = str;
    
    while (isdigit(*p)) {
        size = size * 10 + (*p - '0');
        p++;
    }
    
    if (*p == 'K' || *p == 'k') {
        size *= 1024;
    } else if (*p == 'M' || *p == 'm') {
        size *= 1024 * 1024;
    } else if (*p == 'G' || *p == 'g') {
        size *= 1024LL * 1024 * 1024;
    }
    
    return size;
}

// 生成输出文件名
static void generate_filename(const char *prefix, int index, char *filename, size_t size) {
    // 生成类似 xaa, xab, xac 或 part01, part02 的文件名
    if (index < 26) {
        snprintf(filename, size, "%s%c%c", prefix, 'a' + (index / 26), 'a' + (index % 26));
    } else {
        snprintf(filename, size, "%s%02d", prefix, index);
    }
}

// 按行数分割
static int split_by_lines(const char *input_file, const char *prefix, int lines_per_file, ShellContext *ctx) {
    FILE *input = fopen(input_file, "r");
    if (input == NULL) {
        XSHELL_LOG_ERROR(ctx, "xsplit: %s: %s\n", input_file, strerror(errno));
        return -1;
    }
    
    char line[MAX_LINE_LENGTH];
    int file_index = 0;
    int line_count = 0;
    FILE *output = NULL;
    char output_filename[256];
    
    while (fgets(line, sizeof(line), input) != NULL) {
        if (line_count == 0) {
            // 打开新文件
            if (output != NULL) {
                fclose(output);
            }
            generate_filename(prefix, file_index, output_filename, sizeof(output_filename));
            output = fopen(output_filename, "w");
            if (output == NULL) {
                XSHELL_LOG_ERROR(ctx, "xsplit: %s: %s\n", output_filename, strerror(errno));
                fclose(input);
                return -1;
            }
            file_index++;
        }
        
        fputs(line, output);
        line_count++;
        
        if (line_count >= lines_per_file) {
            line_count = 0;
        }
    }
    
    if (output != NULL) {
        fclose(output);
    }
    fclose(input);
    
    return 0;
}

// 按大小分割
static int split_by_size(const char *input_file, const char *prefix, long long bytes_per_file, ShellContext *ctx) {
    FILE *input = fopen(input_file, "r");
    if (input == NULL) {
        XSHELL_LOG_ERROR(ctx, "xsplit: %s: %s\n", input_file, strerror(errno));
        return -1;
    }
    
    int file_index = 0;
    long long byte_count = 0;
    FILE *output = NULL;
    char output_filename[256];
    int ch;
    
    while ((ch = fgetc(input)) != EOF) {
        if (byte_count == 0) {
            // 打开新文件
            if (output != NULL) {
                fclose(output);
            }
            generate_filename(prefix, file_index, output_filename, sizeof(output_filename));
            output = fopen(output_filename, "w");
            if (output == NULL) {
                XSHELL_LOG_ERROR(ctx, "xsplit: %s: %s\n", output_filename, strerror(errno));
                fclose(input);
                return -1;
            }
            file_index++;
        }
        
        fputc(ch, output);
        byte_count++;
        
        if (byte_count >= bytes_per_file) {
            byte_count = 0;
        }
    }
    
    if (output != NULL) {
        fclose(output);
    }
    fclose(input);
    
    return 0;
}

// xsplit 命令实现
int cmd_xsplit(Command *cmd, ShellContext *ctx) {
    int lines_per_file = 0;
    long long bytes_per_file = 0;
    const char *input_file = NULL;
    const char *prefix = DEFAULT_PREFIX;
    
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
        } else if (strcmp(cmd->args[i], "-l") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xsplit: 错误: -l 选项需要参数\n");
                return -1;
            }
            lines_per_file = atoi(cmd->args[i + 1]);
            if (lines_per_file <= 0) {
                XSHELL_LOG_ERROR(ctx, "xsplit: 错误: 无效的行数\n");
                return -1;
            }
            i += 2;
        } else if (strcmp(cmd->args[i], "-b") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xsplit: 错误: -b 选项需要参数\n");
                return -1;
            }
            bytes_per_file = parse_size(cmd->args[i + 1]);
            if (bytes_per_file <= 0) {
                XSHELL_LOG_ERROR(ctx, "xsplit: 错误: 无效的大小\n");
                return -1;
            }
            i += 2;
        } else if (input_file == NULL) {
            input_file = cmd->args[i];
            i++;
        } else if (strcmp(prefix, DEFAULT_PREFIX) == 0) {
            prefix = cmd->args[i];
            i++;
        } else {
            i++;
        }
    }
    
    if (input_file == NULL) {
        XSHELL_LOG_ERROR(ctx, "xsplit: 错误: 需要指定输入文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    if (lines_per_file == 0 && bytes_per_file == 0) {
        XSHELL_LOG_ERROR(ctx, "xsplit: 错误: 必须指定 -l 或 -b 选项\n");
        show_help(cmd->name);
        return -1;
    }
    
    if (lines_per_file > 0) {
        return split_by_lines(input_file, prefix, lines_per_file, ctx);
    } else {
        return split_by_size(input_file, prefix, bytes_per_file, ctx);
    }
}



