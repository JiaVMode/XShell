/*
 * xtype.c - 显示命令类型
 * 
 * 功能：显示命令是内置命令还是外部命令
 * 用法：xtype <command>
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include "executor.h"
#include "alias.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

// 在PATH中搜索命令
static char* search_in_path(const char *command) {
    char *path_env = getenv("PATH");
    if (!path_env) {
        return NULL;
    }
    
    // 复制PATH环境变量（因为strtok会修改字符串）
    char *path_copy = strdup(path_env);
    if (!path_copy) {
        return NULL;
    }
    
    static char full_path[1024];
    char *dir = strtok(path_copy, ":");
    
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            free(path_copy);
            return full_path;
        }
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return NULL;
}

int cmd_xtype(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtype - 显示命令类型\n\n");
        printf("用法:\n");
        printf("  xtype <command>...\n\n");
        printf("说明:\n");
        printf("  显示命令是内置命令、别名还是外部命令。\n");
        printf("  Type - 类型。\n\n");
        printf("参数:\n");
        printf("  command   要检查的命令名（可以多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("命令类型:\n");
        printf("  • 内置命令 (builtin) - XShell内部实现的命令\n");
        printf("  • 别名 (alias)       - 用户定义的命令别名\n");
        printf("  • 外部命令 (file)    - 系统中的可执行文件\n");
        printf("  • 未找到 (not found) - 不存在的命令\n\n");
        printf("示例:\n");
        printf("  xtype xls                  # 检查xls命令类型\n");
        printf("  xtype xls ls pwd           # 检查多个命令\n");
        printf("  xtype quit                 # 检查quit命令类型\n\n");
        printf("对应系统命令: type\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xtype: missing command name\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xtype --help' for more information.\n");
        return -1;
    }
    
    // 检查每个命令
    int has_error = 0;
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *command = cmd->args[i];
        
        // 检查是否是内置命令
        if (is_builtin(command)) {
            printf("%s is a shell builtin\n", command);
            continue;
        }
        
        // 检查是否是别名
        const char *alias_value = alias_get(command);
        if (alias_value) {
            printf("%s is aliased to `%s'\n", command, alias_value);
            continue;
        }
        
        // 在PATH中搜索
        char *path = search_in_path(command);
        if (path) {
            printf("%s is %s\n", command, path);
        } else {
            printf("%s: not found\n", command);
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}

