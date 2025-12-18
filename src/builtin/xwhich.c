/*
 * xwhich.c - 显示命令路径
 * 
 * 功能：在PATH中搜索命令的完整路径
 * 用法：xwhich <command>
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// 在PATH中搜索命令
static int find_command(const char *command) {
    char *path_env = getenv("PATH");
    if (!path_env) {
        return -1;
    }
    
    // 复制PATH环境变量（因为strtok会修改字符串）
    char *path_copy = strdup(path_env);
    if (!path_copy) {
        return -1;
    }
    
    char full_path[1024];
    char *dir = strtok(path_copy, ":");
    int found = 0;
    
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            printf("%s\n", full_path);
            found = 1;
            break;
        }
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return found ? 0 : -1;
}

int cmd_xwhich(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xwhich - 显示命令路径\n\n");
        printf("用法:\n");
        printf("  xwhich <command>...\n\n");
        printf("说明:\n");
        printf("  在PATH环境变量中搜索命令的完整路径。\n");
        printf("  Which - 哪个。\n\n");
        printf("参数:\n");
        printf("  command   要搜索的命令名（可以多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xwhich ls                  # 查找ls命令路径\n");
        printf("  xwhich ls cat grep         # 查找多个命令\n");
        printf("  xwhich python3             # 查找python3路径\n\n");
        printf("注意:\n");
        printf("  • 只搜索PATH环境变量中的目录\n");
        printf("  • 只返回第一个找到的路径\n");
        printf("  • 命令必须有可执行权限\n");
        printf("  • 不会搜索内置命令和别名\n\n");
        printf("对应系统命令: which\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xwhich: missing command name\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xwhich --help' for more information.\n");
        return -1;
    }
    
    // 搜索每个命令
    int has_error = 0;
    for (int i = 1; i < cmd->arg_count; i++) {
        if (find_command(cmd->args[i]) != 0) {
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}

