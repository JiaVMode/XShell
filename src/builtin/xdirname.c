/*
 * xdirname.c - 提取目录名
 * 
 * 功能：从路径中提取目录部分（去除文件名）
 * 用法：xdirname <路径>
 * 
 * 选项：
 *   --help 显示帮助信息
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s <路径>\n", cmd_name);
    printf("功能: 从路径中提取目录部分（去除文件名）\n");
    printf("选项:\n");
    printf("  -h, --help       显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s /path/to/file.txt    # 输出: /path/to\n", cmd_name);
    printf("  %s file.txt             # 输出: .\n", cmd_name);
    printf("  %s /path/to/            # 输出: /path/to\n", cmd_name);
    printf("  %s /                   # 输出: /\n", cmd_name);
}

// xdirname 命令实现
int cmd_xdirname(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    // 检查参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    // 检查帮助选项
    if (strcmp(cmd->args[1], "--help") == 0 || strcmp(cmd->args[1], "-h") == 0) {
        show_help(cmd->name);
        return 0;
    }
    
    // 获取路径
    const char *path = cmd->args[1];
    size_t len = strlen(path);
    
    // 处理特殊情况
    if (len == 0) {
        printf(".\n");
        return 0;
    }
    
    // 查找最后一个 '/'
    const char *last_slash = strrchr(path, '/');
    
    if (last_slash == NULL) {
        // 没有目录分隔符，输出 '.'
        printf(".\n");
    } else if (last_slash == path) {
        // 第一个字符就是 '/'，可能是根目录或只有 '/'
        if (len == 1) {
            printf("/\n");
        } else {
            // 类似 "/path" 的情况
            printf("/\n");
        }
    } else {
        // 有目录部分，输出目录部分（不包括最后的 '/'）
        // 跳过末尾的 '/'（如果有多个）
        size_t dir_len = last_slash - path;
        while (dir_len > 1 && path[dir_len - 1] == '/') {
            dir_len--;
        }
        
        // 输出目录部分
        for (size_t i = 0; i < dir_len; i++) {
            putchar(path[i]);
        }
        putchar('\n');
    }
    
    return 0;
}



