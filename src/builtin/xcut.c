/*
 * xcut.c - 提取列
 * 
 * 功能：从文件中提取指定的列（字段）
 * 用法：xcut [选项] [文件...]
 * 
 * 选项：
 *   -d <分隔符>  指定字段分隔符（默认制表符）
 *   -f <字段列表> 指定要提取的字段（如 1,2,3 或 1-3）
 *   -c <字符位置> 指定要提取的字符位置（如 1-10）
 *   --help       显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <errno.h>

#define MAX_LINE_LENGTH 4096

// 选项结构体
typedef struct {
    char delimiter;      // 分隔符
    int use_fields;      // 使用字段模式（-f）
    int use_chars;       // 使用字符模式（-c）
    char *field_spec;    // 字段规格（如 "1,2,3" 或 "1-3"）
    char *char_spec;     // 字符规格（如 "1-10"）
} CutOptions;

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] [文件...]\n", cmd_name);
    printf("功能: 从文件中提取指定的列（字段）\n");
    printf("选项:\n");
    printf("  -d <分隔符>    指定字段分隔符（默认制表符）\n");
    printf("  -f <字段列表>  指定要提取的字段（如 1,2,3 或 1-3）\n");
    printf("  -c <字符位置>  指定要提取的字符位置（如 1-10）\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s -d: -f1 /etc/passwd\n", cmd_name);
    printf("  %s -c1-10 file.txt\n", cmd_name);
}

// 解析字段规格（如 "1,2,3" 或 "1-3"）
static int parse_field_spec(const char *spec, int *fields, int max_fields) {
    int count = 0;
    const char *p = spec;
    
    while (*p != '\0' && count < max_fields) {
        // 跳过空白
        while (*p == ' ' || *p == '\t') p++;
        if (*p == '\0') break;
        
        // 解析数字或范围
        if (isdigit(*p)) {
            int start = 0;
            while (isdigit(*p)) {
                start = start * 10 + (*p - '0');
                p++;
            }
            
            if (*p == '-') {
                // 范围
                p++;
                int end = 0;
                while (isdigit(*p)) {
                    end = end * 10 + (*p - '0');
                    p++;
                }
                
                // 添加范围内的所有字段
                for (int i = start; i <= end && count < max_fields; i++) {
                    fields[count++] = i;
                }
            } else {
                // 单个字段
                fields[count++] = start;
            }
            
            // 跳过逗号
            if (*p == ',') p++;
        } else {
            p++;
        }
    }
    
    return count;
}

// 处理文件（字段模式）
static int process_file_fields(FILE *file, const CutOptions *opts, const char *filename, ShellContext *ctx) {
    (void)filename;
    char line[MAX_LINE_LENGTH];
    int fields[100];
    int field_count = 0;
    
    // 解析字段规格
    if (opts->field_spec != NULL) {
        field_count = parse_field_spec(opts->field_spec, fields, 100);
    }
    
    if (field_count == 0) {
        XSHELL_LOG_ERROR(ctx, "xcut: 错误: 无效的字段规格\n");
        return -1;
    }
    
    // 读取并处理每一行
    while (fgets(line, sizeof(line), file) != NULL) {
        // 移除换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
        }
        
        // 分割字段
        char *token = line;
        int field_num = 1;
        int output_count = 0;
        
        for (size_t i = 0; i <= len; i++) {
            if (line[i] == opts->delimiter || line[i] == '\0') {
                // 检查是否需要输出此字段
                for (int j = 0; j < field_count; j++) {
                    if (fields[j] == field_num) {
                        if (output_count > 0) {
                            putchar(opts->delimiter);
                        }
                        // 输出字段
                        for (size_t k = 0; k < (size_t)(line + i - token); k++) {
                            putchar(token[k]);
                        }
                        output_count++;
                        break;
                    }
                }
                
                token = line + i + 1;
                field_num++;
            }
        }
        
        putchar('\n');
    }
    
    return 0;
}

// 处理文件（字符模式）
static int process_file_chars(FILE *file, const CutOptions *opts, const char *filename) {
    (void)filename;
    char line[MAX_LINE_LENGTH];
    int start = 1, end = 1;
    
    // 解析字符规格（简化：只支持 "start-end" 格式）
    if (opts->char_spec != NULL) {
        sscanf(opts->char_spec, "%d-%d", &start, &end);
        if (start < 1) start = 1;
        if (end < start) end = start;
    }
    
    // 读取并处理每一行
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t len = strlen(line);
        
        // 移除换行符（但最后会重新添加）
        int has_newline = 0;
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
            len--;
            has_newline = 1;
        }
        
        // 输出指定范围的字符（转换为0-based索引）
        int start_idx = start - 1;
        int end_idx = end - 1;
        
        if (start_idx < (int)len) {
            if (end_idx >= (int)len) {
                end_idx = len - 1;
            }
            
            for (int i = start_idx; i <= end_idx && i < (int)len; i++) {
                putchar(line[i]);
            }
        }
        
        if (has_newline) {
            putchar('\n');
        }
    }
    
    return 0;
}

// xcut 命令实现
int cmd_xcut(Command *cmd, ShellContext *ctx) {
    CutOptions opts = {0};
    opts.delimiter = '\t';  // 默认制表符
    
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
                XSHELL_LOG_ERROR(ctx, "xcut: 错误: -d 选项需要参数\n");
                return -1;
            }
            opts.delimiter = cmd->args[i + 1][0];
            i += 2;
        } else if (strcmp(cmd->args[i], "-f") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xcut: 错误: -f 选项需要参数\n");
                return -1;
            }
            opts.use_fields = 1;
            opts.field_spec = cmd->args[i + 1];
            i += 2;
        } else if (strcmp(cmd->args[i], "-c") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xcut: 错误: -c 选项需要参数\n");
                return -1;
            }
            opts.use_chars = 1;
            opts.char_spec = cmd->args[i + 1];
            i += 2;
        } else if (cmd->args[i][0] == '-' && strlen(cmd->args[i]) > 1) {
            // 可能是组合选项，如 -d: 或 -f1
            if (cmd->args[i][1] == 'd' && cmd->args[i][2] != '\0') {
                opts.delimiter = cmd->args[i][2];
                i++;
            } else if (cmd->args[i][1] == 'f' && cmd->args[i][2] != '\0') {
                opts.use_fields = 1;
                opts.field_spec = cmd->args[i] + 2;
                i++;
            } else if (cmd->args[i][1] == 'c' && cmd->args[i][2] != '\0') {
                opts.use_chars = 1;
                opts.char_spec = cmd->args[i] + 2;
                i++;
            } else {
                break;
            }
        } else {
            break;
        }
    }
    
    // 检查是否指定了模式
    if (!opts.use_fields && !opts.use_chars) {
        XSHELL_LOG_ERROR(ctx, "xcut: 错误: 必须指定 -f 或 -c 选项\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 处理文件
    int has_files = 0;
    for (; i < cmd->arg_count; i++) {
        const char *filename = cmd->args[i];
        FILE *file;
        
        if (strcmp(filename, "-") == 0) {
            file = stdin;
            filename = "(standard input)";
        } else {
            file = fopen(filename, "r");
            if (file == NULL) {
                XSHELL_LOG_ERROR(ctx, "xcut: %s: %s\n", filename, strerror(errno));
                continue;
            }
        }
        
        has_files = 1;
        
        if (opts.use_fields) {
            process_file_fields(file, &opts, filename, ctx);
        } else {
            process_file_chars(file, &opts, filename);
        }
        
        if (file != stdin) {
            fclose(file);
        }
    }
    
    // 如果没有文件，从标准输入读取
    if (!has_files) {
        if (opts.use_fields) {
            process_file_fields(stdin, &opts, "(standard input)", ctx);
        } else {
            process_file_chars(stdin, &opts, "(standard input)");
        }
    }
    
    return 0;
}

