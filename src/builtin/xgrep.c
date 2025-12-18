/*
 * xgrep.c - 在文件中搜索匹配的文本行
 * 
 * 功能：类似于 grep 命令，搜索文件中包含指定模式的行
 * 用法：xgrep [选项] <pattern> <file>...
 * 
 * 选项：
 *   -i    忽略大小写
 *   -n    显示行号
 *   -v    反向匹配（显示不匹配的行）
 *   -c    只显示匹配行的计数
 *   -w    整词匹配
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

// 选项结构体
typedef struct {
    int ignore_case;    // -i 忽略大小写
    int show_line_num;  // -n 显示行号
    int invert_match;   // -v 反向匹配
    int count_only;     // -c 只显示计数
    int whole_word;     // -w 整词匹配
} GrepOptions;

// 忽略大小写的字符串查找
static const char* stristr(const char* haystack, const char* needle) {
    if (!*needle) return haystack;
    
    size_t needle_len = strlen(needle);
    for (; *haystack; haystack++) {
        if (tolower(*haystack) == tolower(*needle)) {
            size_t i;
            for (i = 1; i < needle_len; i++) {
                if (tolower(haystack[i]) != tolower(needle[i])) {
                    break;
                }
            }
            if (i == needle_len) {
                return haystack;
            }
        }
    }
    return NULL;
}

// 检查字符是否为单词边界字符
static int is_word_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || 
           (c >= '0' && c <= '9') || c == '_';
}

// 整词匹配检查
static int is_whole_word_match(const char* text, const char* match_pos, const char* pattern) {
    size_t pattern_len = strlen(pattern);
    
    // 检查前一个字符
    if (match_pos > text) {
        char prev_char = *(match_pos - 1);
        if (is_word_char(prev_char)) {
            return 0;  // 前一个字符是单词字符，不是整词匹配
        }
    }
    
    // 检查后一个字符
    char next_char = *(match_pos + pattern_len);
    if (next_char != '\0' && is_word_char(next_char)) {
        return 0;  // 后一个字符是单词字符，不是整词匹配
    }
    
    return 1;  // 是整词匹配
}

// 在文件中搜索模式
static int grep_file(const char* filename, const char* pattern, 
                     const GrepOptions* opts, int show_filename,
                     ShellContext *ctx) {
    FILE* file;
    char line[4096];
    int line_num = 0;
    int match_count = 0;
    int found_match = 0;
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
        filename = "(standard input)";
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xgrep: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }
    
    // 逐行读取
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // 移除行尾的换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // 检查是否匹配
        int is_match = 0;
        
        if (opts->whole_word) {
            // 整词匹配模式
            const char* match_pos;
            if (opts->ignore_case) {
                match_pos = stristr(line, pattern);
            } else {
                match_pos = strstr(line, pattern);
            }
            
            // 检查所有可能的匹配位置
            while (match_pos != NULL) {
                if (is_whole_word_match(line, match_pos, pattern)) {
                    is_match = 1;
                    break;
                }
                // 继续查找下一个匹配
                if (opts->ignore_case) {
                    match_pos = stristr(match_pos + 1, pattern);
                } else {
                    match_pos = strstr(match_pos + 1, pattern);
                }
            }
        } else {
            // 普通匹配模式
            if (opts->ignore_case) {
                is_match = (stristr(line, pattern) != NULL);
            } else {
                is_match = (strstr(line, pattern) != NULL);
            }
        }
        
        // 反向匹配
        if (opts->invert_match) {
            is_match = !is_match;
        }
        
        if (is_match) {
            match_count++;
            found_match = 1;
            
            // 如果只显示计数，不输出行内容
            if (opts->count_only) {
                continue;
            }
            
            // 输出文件名（如果有多个文件）
            if (show_filename) {
                printf("%s:", filename);
            }
            
            // 输出行号
            if (opts->show_line_num) {
                printf("%d:", line_num);
            }
            
            // 输出行内容
            printf("%s\n", line);
        }
    }
    
    // 如果只显示计数，输出计数结果
    if (opts->count_only) {
        if (show_filename) {
            printf("%s:", filename);
        }
        printf("%d\n", match_count);
    }
    
    // 关闭文件
    if (file != stdin) {
        fclose(file);
    }
    
    return found_match ? 0 : 1;
}

int cmd_xgrep(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xgrep - 在文件中搜索文本\n\n");
        printf("用法:\n");
        printf("  xgrep [选项] <pattern> <file>...\n");
        printf("  xgrep [选项] <pattern>            # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  在文件中搜索包含指定模式的行。\n");
        printf("  Global Regular Expression Print - 全局正则表达式打印。\n\n");
        printf("参数:\n");
        printf("  pattern   要搜索的文本模式\n");
        printf("  file      要搜索的文件（可以多个）\n");
        printf("            使用 - 表示从标准输入读取\n\n");
        printf("选项:\n");
        printf("  -i        忽略大小写\n");
        printf("  -n        显示行号\n");
        printf("  -v        反向匹配（显示不匹配的行）\n");
        printf("  -c        只显示匹配行的计数\n");
        printf("  -w        整词匹配\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xgrep hello file.txt           # 搜索包含 hello 的行\n");
        printf("  xgrep -i hello file.txt        # 忽略大小写搜索\n");
        printf("  xgrep -n error log.txt         # 显示行号\n");
        printf("  xgrep -v comment file.c        # 显示不包含 comment 的行\n");
        printf("  xgrep -c TODO *.txt            # 统计匹配行数\n");
        printf("  xgrep -w apple file.txt        # 整词匹配\n");
        printf("  xgrep -in error *.log          # 组合选项\n");
        printf("  xcat file.txt | xgrep pattern  # 从管道读取\n\n");
        printf("对应系统命令: grep\n");
        return 0;
    }
    
    // 解析选项
    GrepOptions opts = {0};
    int start_index = 1;
    
    while (start_index < cmd->arg_count && cmd->args[start_index][0] == '-' 
           && strcmp(cmd->args[start_index], "-") != 0) {
        const char* arg = cmd->args[start_index];
        
        if (strcmp(arg, "--help") == 0) {
            start_index++;
            continue;
        }
        
        // 解析单字符选项（支持组合如 -in）
        for (int i = 1; arg[i]; i++) {
            switch (arg[i]) {
                case 'i': opts.ignore_case = 1; break;
                case 'n': opts.show_line_num = 1; break;
                case 'v': opts.invert_match = 1; break;
                case 'c': opts.count_only = 1; break;
                case 'w': opts.whole_word = 1; break;
                default:
                    XSHELL_LOG_ERROR(ctx, "xgrep: invalid option: '-%c'\n", arg[i]);
                    XSHELL_LOG_ERROR(ctx, "Try 'xgrep --help' for more information.\n");
                    return -1;
            }
        }
        start_index++;
    }
    
    // 检查参数
    if (start_index >= cmd->arg_count) {
        XSHELL_LOG_ERROR(ctx, "xgrep: missing pattern\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xgrep --help' for more information.\n");
        return -1;
    }
    
    const char* pattern = cmd->args[start_index];
    start_index++;
    
    // 如果没有指定文件，从标准输入读取
    if (start_index >= cmd->arg_count) {
        return grep_file("-", pattern, &opts, 0, ctx);
    }
    
    // 搜索多个文件
    int has_error = 0;
    int all_not_found = 1;
    int show_filename = (cmd->arg_count - start_index > 1);  // 多个文件时显示文件名
    
    for (int i = start_index; i < cmd->arg_count; i++) {
        int result = grep_file(cmd->args[i], pattern, &opts, show_filename, ctx);
        if (result == -1) {
            has_error = 1;
        } else if (result == 0) {
            all_not_found = 0;
        }
    }
    
    // 返回值：找不到匹配返回1，出错返回-1，找到匹配返回0
    if (has_error) return -1;
    return all_not_found ? 1 : 0;
}




