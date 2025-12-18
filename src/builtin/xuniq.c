/*
 * xuniq.c - 去除文件中的重复行
 * 
 * 功能：类似于 uniq 命令，过滤相邻的重复行
 * 用法：xuniq [选项] [file]
 * 
 * 选项：
 *   -c    在每行前显示重复次数
 *   -d    只显示重复的行
 *   -u    只显示不重复的行
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

// 选项结构体
typedef struct {
    int count;       // -c 显示重复次数
    int duplicates;  // -d 只显示重复的行
    int unique;      // -u 只显示不重复的行
} UniqOptions;

// 去重处理
static int uniq_file(const char* filename, const UniqOptions* opts, ShellContext *ctx) {
    FILE* file;
    char prev_line[4096] = {0};
    char curr_line[4096];
    int line_count = 0;
    int is_first_line = 1;
    
    // 打开文件（"-" 表示标准输入）
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");
        if (!file) {
            XSHELL_LOG_ERROR(ctx, "xuniq: %s: %s\n", filename, strerror(errno));
            return -1;
        }
    }
    
    // 逐行读取
    while (fgets(curr_line, sizeof(curr_line), file)) {
        if (is_first_line) {
            // 第一行，直接保存
            strcpy(prev_line, curr_line);
            line_count = 1;
            is_first_line = 0;
        } else if (strcmp(curr_line, prev_line) == 0) {
            // 相同行，增加计数
            line_count++;
        } else {
            // 不同行，输出前一行
            int should_print = 0;
            
            if (opts->duplicates) {
                // -d: 只显示重复的行（出现次数 > 1）
                should_print = (line_count > 1);
            } else if (opts->unique) {
                // -u: 只显示不重复的行（出现次数 = 1）
                should_print = (line_count == 1);
            } else {
                // 默认：显示所有不重复的行
                should_print = 1;
            }
            
            if (should_print) {
                if (opts->count) {
                    // -c: 显示重复次数
                    printf("%7d %s", line_count, prev_line);
                } else {
                    printf("%s", prev_line);
                }
            }
            
            // 保存当前行作为新的前一行
            strcpy(prev_line, curr_line);
            line_count = 1;
        }
    }
    
    // 处理最后一行
    if (!is_first_line) {
        int should_print = 0;
        
        if (opts->duplicates) {
            should_print = (line_count > 1);
        } else if (opts->unique) {
            should_print = (line_count == 1);
        } else {
            should_print = 1;
        }
        
        if (should_print) {
            if (opts->count) {
                printf("%7d %s", line_count, prev_line);
            } else {
                printf("%s", prev_line);
            }
        }
    }
    
    // 关闭文件
    if (file != stdin) {
        fclose(file);
    }
    
    return 0;
}

int cmd_xuniq(Command* cmd, ShellContext* ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xuniq - 去除文件中的重复行\n\n");
        printf("用法:\n");
        printf("  xuniq [选项] [file]\n");
        printf("  xuniq [选项]               # 从标准输入读取\n\n");
        printf("说明:\n");
        printf("  过滤相邻的重复行。\n");
        printf("  Unique - 唯一。\n\n");
        printf("重要提示:\n");
        printf("  xuniq 只会去除**相邻**的重复行。\n");
        printf("  如果要去除所有重复行，需要先排序：\n");
        printf("    xsort file.txt | xuniq\n\n");
        printf("参数:\n");
        printf("  file      要处理的文件\n");
        printf("            不指定文件则从标准输入读取\n\n");
        printf("选项:\n");
        printf("  -c        在每行前显示该行出现的次数\n");
        printf("  -d        只显示重复的行（出现 > 1 次）\n");
        printf("  -u        只显示不重复的行（出现 = 1 次）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xuniq file.txt             # 去除相邻重复行\n");
        printf("  xuniq -c file.txt          # 显示每行出现次数\n");
        printf("  xuniq -d file.txt          # 只显示重复行\n");
        printf("  xuniq -u file.txt          # 只显示唯一行\n");
        printf("  xsort file.txt | xuniq     # 排序后去重（完全去重）\n");
        printf("  xsort file.txt | xuniq -c  # 统计每行出现次数\n");
        printf("  xcat *.txt | xsort | xuniq # 合并文件并去重\n\n");
        printf("工作原理:\n");
        printf("  输入：    输出（默认）：\n");
        printf("  aaa       aaa\n");
        printf("  aaa       bbb\n");
        printf("  bbb       aaa\n");
        printf("  aaa       \n");
        printf("  \n");
        printf("  注意：第三个 aaa 与前面不相邻，所以会输出。\n\n");
        printf("  输入：    排序后：  去重后：\n");
        printf("  aaa       aaa       aaa\n");
        printf("  aaa       aaa       bbb\n");
        printf("  bbb       aaa       \n");
        printf("  aaa       bbb       \n\n");
        printf("常见用法:\n");
        printf("  • 统计文件中不同行的数量：\n");
        printf("    xsort file.txt | xuniq | xwc -l\n");
        printf("  • 找出重复的行：\n");
        printf("    xsort file.txt | xuniq -d\n");
        printf("  • 统计每行出现的次数：\n");
        printf("    xsort file.txt | xuniq -c | xsort -rn\n\n");
        printf("对应系统命令: uniq\n");
        return 0;
    }
    
    // 解析选项
    UniqOptions opts = {0};
    int start_index = 1;
    
    while (start_index < cmd->arg_count && cmd->args[start_index][0] == '-'
           && strcmp(cmd->args[start_index], "-") != 0) {
        const char* arg = cmd->args[start_index];
        
        if (strcmp(arg, "--help") == 0) {
            start_index++;
            continue;
        }
        
        // 解析单字符选项
        for (int i = 1; arg[i]; i++) {
            switch (arg[i]) {
                case 'c': opts.count = 1; break;
                case 'd': opts.duplicates = 1; break;
                case 'u': opts.unique = 1; break;
                default:
                    XSHELL_LOG_ERROR(ctx, "xuniq: invalid option: '-%c'\n", arg[i]);
                    XSHELL_LOG_ERROR(ctx, "Try 'xuniq --help' for more information.\n");
                    return -1;
            }
        }
        start_index++;
    }
    
    // -d 和 -u 互斥
    if (opts.duplicates && opts.unique) {
        XSHELL_LOG_ERROR(ctx, "xuniq: options -d and -u are mutually exclusive\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xuniq --help' for more information.\n");
        return -1;
    }
    
    // 获取文件名
    const char* filename = "-";
    if (start_index < cmd->arg_count) {
        filename = cmd->args[start_index];
    }
    
    return uniq_file(filename, &opts, ctx);
}




