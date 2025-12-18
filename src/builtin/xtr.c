/*
 * xtr.c - 字符转换
 * 
 * 功能：转换或删除字符
 * 用法：xtr [选项] <字符集1> [字符集2]
 * 
 * 选项：
 *   -d    删除字符
 *   --help 显示帮助信息
 * 
 * 注意：简化实现，支持基本功能
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#define MAX_LINE_LENGTH 4096

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <字符集1> [字符集2]\n", cmd_name);
    printf("功能: 转换或删除字符\n");
    printf("选项:\n");
    printf("  -d              删除字符集中的字符\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s 'a-z' 'A-Z' < file.txt    # 小写转大写\n", cmd_name);
    printf("  %s -d '0-9' < file.txt       # 删除数字\n", cmd_name);
    printf("注意: 简化实现，支持基本字符范围（a-z, A-Z, 0-9）\n");
}

// 检查字符是否在范围内
static int in_range(char c, const char *range) {
    if (range == NULL || *range == '\0') {
        return 0;
    }
    
    // 简化实现：支持 "a-z", "A-Z", "0-9" 格式
    size_t len = strlen(range);
    if (len == 3 && range[1] == '-') {
        char start = range[0];
        char end = range[2];
        
        if (c >= start && c <= end) {
            return 1;
        }
    } else if (len == 1) {
        // 单个字符
        return (c == range[0]);
    }
    
    return 0;
}

// 转换字符
static char translate_char(char c, const char *from, const char *to) {
    if (from == NULL || to == NULL) {
        return c;
    }
    
    // 简化实现：支持 "a-z" -> "A-Z" 这样的转换
    size_t from_len = strlen(from);
    size_t to_len = strlen(to);
    
    if (from_len == 3 && from[1] == '-' && to_len == 3 && to[1] == '-') {
        char from_start = from[0];
        char from_end = from[2];
        char to_start = to[0];
        char to_end = to[2];
        
        if (c >= from_start && c <= from_end) {
            // 计算偏移量
            int offset = c - from_start;
            int range_size = from_end - from_start;
            int to_range_size = to_end - to_start;
            
            if (range_size > 0 && to_range_size > 0) {
                // 按比例映射
                int mapped_offset = (offset * to_range_size) / range_size;
                return to_start + mapped_offset;
            }
        }
    }
    
    return c;
}

// 处理文件
static int process_file(FILE *file, int delete_mode, const char *set1, const char *set2) {
    char line[MAX_LINE_LENGTH];
    
    while (fgets(line, sizeof(line), file) != NULL) {
        if (delete_mode) {
            // 删除模式：删除 set1 中的字符
            for (size_t i = 0; line[i] != '\0'; i++) {
                if (!in_range(line[i], set1)) {
                    putchar(line[i]);
                }
            }
        } else {
            // 转换模式：将 set1 转换为 set2
            for (size_t i = 0; line[i] != '\0'; i++) {
                char c = translate_char(line[i], set1, set2);
                putchar(c);
            }
        }
    }
    
    return 0;
}

// xtr 命令实现
int cmd_xtr(Command *cmd, ShellContext *ctx) {
    int delete_mode = 0;
    const char *set1 = NULL;
    const char *set2 = NULL;
    
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
            delete_mode = 1;
            i++;
        } else if (set1 == NULL) {
            set1 = cmd->args[i];
            i++;
        } else if (set2 == NULL) {
            set2 = cmd->args[i];
            i++;
        } else {
            i++;
        }
    }
    
    if (set1 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xtr: 错误: 需要指定字符集\n");
        show_help(cmd->name);
        return -1;
    }
    
    if (!delete_mode && set2 == NULL) {
        XSHELL_LOG_ERROR(ctx, "xtr: 错误: 转换模式需要两个字符集\n");
        show_help(cmd->name);
        return -1;
    }
    
    // 处理标准输入
    process_file(stdin, delete_mode, set1, set2);
    
    return 0;
}



