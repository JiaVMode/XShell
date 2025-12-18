/*
 * xwc.c - 统计文件的行数、字数和字节数
 * 
 * 功能：类似于 wc 命令，统计文件信息
 * 用法：xwc [选项] [file]...
 * 
 * 选项：
 *   -l    只显示行数
 *   -w    只显示字数
 *   -c    只显示字节数
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// 选项结构体
typedef struct {
    int lines_only;   // -l 只显示行数
    int words_only;   // -w 只显示字数
    int bytes_only;   // -c 只显示字节数
} WcOptions;

// 统计结果结构体
typedef struct {
    long lines;
    long words;
    long bytes;
} WcStats;

// 统计文件
static int wc_file(const char* filename, const WcOptions* opts, WcStats* stats, ShellContext *ctx) {
    FILE* file;
    int ch;
    int in_word = 0;
    (void)opts;
    
    stats->lines = 0;
    stats->words = 0;
    stats->bytes = 0;
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xwc: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }
    
    // 逐字符读取并统计
    while ((ch = fgetc(file)) != EOF) {
        stats->bytes++;
        
        if (ch == '\n') {
            stats->lines++;
        }
        
        // 统计字数：空白字符分隔
        if (isspace(ch)) {
            in_word = 0;
        } else {
            if (!in_word) {
                stats->words++;
                in_word = 1;
            }
        }
    }
    
    // 关闭文件
    if (file != stdin) {
        fclose(file);
    }
    
    return 0;
}

// 打印统计结果
static void print_stats(const WcStats* stats, const WcOptions* opts, 
                       const char* filename) {
    // 如果没有指定选项，显示所有统计信息
    int show_all = !opts->lines_only && !opts->words_only && !opts->bytes_only;
    
    if (show_all || opts->lines_only) {
        printf("%7ld", stats->lines);
    }
    
    if (show_all || opts->words_only) {
        printf("%7ld", stats->words);
    }
    
    if (show_all || opts->bytes_only) {
        printf("%7ld", stats->bytes);
    }
    
    if (filename) {
        printf(" %s", filename);
    }
    
    printf("\n");
}

int cmd_xwc(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xwc - 统计文件的行数、字数和字节数\n\n");
        printf("用法:\n");
        printf("  xwc [选项] [file]...\n");
        printf("  xwc [选项]                 # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  统计文件的行数、字数和字节数。\n");
        printf("  Word Count - 字数统计。\n\n");
        printf("参数:\n");
        printf("  file      要统计的文件（可以多个）\n");
        printf("            不指定文件则从标准输入读取\n\n");
        printf("选项:\n");
        printf("  -l        只显示行数\n");
        printf("  -w        只显示字数\n");
        printf("  -c        只显示字节数\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("输出格式:\n");
        printf("  默认格式：行数  字数  字节数  文件名\n");
        printf("  例如：    100   500   3000  file.txt\n\n");
        printf("字数定义:\n");
        printf("  字数是指由空白字符（空格、制表符、换行）分隔的连续字符序列。\n\n");
        printf("示例:\n");
        printf("  xwc file.txt               # 统计所有信息\n");
        printf("  xwc -l file.txt            # 只统计行数\n");
        printf("  xwc -w file.txt            # 只统计字数\n");
        printf("  xwc -c file.txt            # 只统计字节数\n");
        printf("  xwc *.txt                  # 统计多个文件\n");
        printf("  xwc -l *.c                 # 统计所有C文件的行数\n");
        printf("  xcat file.txt | xwc        # 从管道读取\n");
        printf("  xcat file.txt | xwc -l     # 统计管道输入的行数\n\n");
        printf("对应系统命令: wc\n");
        return 0;
    }
    
    // 解析选项
    WcOptions opts = {0};
    int start_index = 1;
    
    while (start_index < cmd->arg_count && cmd->args[start_index][0] == '-'
           && strcmp(cmd->args[start_index], "-") != 0) {
        const char* arg = cmd->args[start_index];
        
        if (strcmp(arg, "--help") == 0) {
            start_index++;
            continue;
        }
        
        // 解析单字符选项（支持组合如 -lw）
        for (int i = 1; arg[i]; i++) {
            switch (arg[i]) {
                case 'l': opts.lines_only = 1; break;
                case 'w': opts.words_only = 1; break;
                case 'c': opts.bytes_only = 1; break;
                default:
                    XSHELL_LOG_ERROR(ctx, "xwc: invalid option: '-%c'\n", arg[i]);
                    XSHELL_LOG_ERROR(ctx, "Try 'xwc --help' for more information.\n");
                    return -1;
            }
        }
        start_index++;
    }
    
    // 如果没有指定文件，从标准输入读取
    if (start_index >= cmd->arg_count) {
        WcStats stats;
        if (wc_file("-", &opts, &stats, ctx) == 0) {
            print_stats(&stats, &opts, NULL);
            return 0;
        }
        return -1;
    }
    
    // 统计多个文件
    int has_error = 0;
    WcStats total = {0};
    int file_count = 0;
    
    for (int i = start_index; i < cmd->arg_count; i++) {
        WcStats stats;
        if (wc_file(cmd->args[i], &opts, &stats, ctx) == 0) {
            print_stats(&stats, &opts, cmd->args[i]);
            total.lines += stats.lines;
            total.words += stats.words;
            total.bytes += stats.bytes;
            file_count++;
        } else {
            has_error = 1;
        }
    }
    
    // 如果有多个文件，显示总计
    if (file_count > 1) {
        print_stats(&total, &opts, "total");
    }
    
    return has_error ? -1 : 0;
}




