/*
 * xtree.c - 树形显示目录结构
 * 
 * 功能：以树形结构显示目录内容
 * 用法：xtree [path]
 */

#define _XOPEN_SOURCE 700
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static int dir_count = 0;
static int file_count = 0;

// 打印缩进和树形字符
static void print_tree_prefix(int depth, int is_last[], int is_last_entry) {
    for (int i = 0; i < depth - 1; i++) {
        if (is_last[i]) {
            printf("    ");
        } else {
            printf("│   ");
        }
    }
    
    if (depth > 0) {
        if (is_last_entry) {
            printf("└── ");
        } else {
            printf("├── ");
        }
    }
}

// 递归显示目录树
static void show_tree(const char *path, int depth, int is_last[], int max_depth, ShellContext *ctx) {
    // 限制最大深度，避免无限递归
    if (max_depth > 0 && depth >= max_depth) {
        return;
    }
    
    DIR *dir = opendir(path);
    if (!dir) {
        XSHELL_LOG_PERROR(ctx, "xtree");
        return;
    }
    
    // 统计条目数量
    struct dirent *entry;
    int entry_count = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        entry_count++;
    }
    rewinddir(dir);
    
    // 遍历目录
    int current = 0;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        current++;
        int is_last_entry = (current == entry_count);
        
        // 打印树形前缀
        print_tree_prefix(depth, is_last, is_last_entry);
        
        // 构造完整路径
        char full_path[1024];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // 获取文件信息
        struct stat st;
        if (lstat(full_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                printf("%s/\n", entry->d_name);
                dir_count++;
                
                // 递归显示子目录
                is_last[depth] = is_last_entry;
                show_tree(full_path, depth + 1, is_last, max_depth, ctx);
            } else if (S_ISLNK(st.st_mode)) {
                char link_target[1024];
                ssize_t len = readlink(full_path, link_target, sizeof(link_target) - 1);
                if (len != -1) {
                    link_target[len] = '\0';
                    printf("%s -> %s\n", entry->d_name, link_target);
                } else {
                    printf("%s\n", entry->d_name);
                }
                file_count++;
            } else {
                printf("%s\n", entry->d_name);
                file_count++;
            }
        } else {
            XSHELL_LOG_ERROR(ctx, "xtree: %s: %s\n", full_path, strerror(errno));
            file_count++;
        }
    }
    
    closedir(dir);
}

int cmd_xtree(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtree - 树形显示目录结构\n\n");
        printf("用法:\n");
        printf("  xtree [path] [-L level]\n\n");
        printf("说明:\n");
        printf("  以树形结构递归显示目录内容。\n");
        printf("  Tree - 树。\n\n");
        printf("参数:\n");
        printf("  path      要显示的目录路径（默认为当前目录）\n\n");
        printf("选项:\n");
        printf("  -L level  限制最大显示深度\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("输出格式:\n");
        printf("  ├── file1       普通文件\n");
        printf("  ├── dir1/       目录（以/结尾）\n");
        printf("  │   └── file2   子目录中的文件\n");
        printf("  └── link -> target  符号链接\n\n");
        printf("示例:\n");
        printf("  xtree                      # 显示当前目录树\n");
        printf("  xtree /home                # 显示/home目录树\n");
        printf("  xtree -L 2                 # 只显示2层深度\n");
        printf("  xtree /usr/local -L 3      # 显示3层深度\n\n");
        printf("注意:\n");
        printf("  • 目录名以 / 结尾\n");
        printf("  • 符号链接显示目标\n");
        printf("  • 隐藏文件（.开头）不显示\n");
        printf("  • 会显示目录和文件总数\n\n");
        printf("对应系统命令: tree\n");
        return 0;
    }
    
    // 解析参数
    const char *path = ".";
    int max_depth = -1; // -1表示无限制
    
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "-L") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xtree: option requires an argument -- 'L'\n");
                return -1;
            }
            max_depth = atoi(cmd->args[++i]);
            if (max_depth < 0) {
                XSHELL_LOG_ERROR(ctx, "xtree: invalid level '%s'\n", cmd->args[i]);
                return -1;
            }
        } else if (cmd->args[i][0] == '-') {
            XSHELL_LOG_ERROR(ctx, "xtree: invalid option '%s'\n", cmd->args[i]);
            XSHELL_LOG_ERROR(ctx, "Try 'xtree --help' for more information.\n");
            return -1;
        } else {
            path = cmd->args[i];
        }
    }
    
    // 检查路径是否存在
    struct stat st;
    if (stat(path, &st) != 0) {
        XSHELL_LOG_PERROR(ctx, "xtree");
        return -1;
    }
    
    if (!S_ISDIR(st.st_mode)) {
        XSHELL_LOG_ERROR(ctx, "xtree: %s: Not a directory\n", path);
        return -1;
    }
    
    // 重置计数器
    dir_count = 0;
    file_count = 0;
    
    // 显示根目录
    printf("%s\n", path);
    
    // 显示树形结构
    int is_last[100] = {0}; // 支持最多100层深度
    show_tree(path, 0, is_last, max_depth, ctx);
    
    // 显示统计信息
    printf("\n%d directories, %d files\n", dir_count, file_count);
    
    return 0;
}




