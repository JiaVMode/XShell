/*
 * xunset.c - 删除环境变量
 * 
 * 功能：删除指定的环境变量
 * 用法：xunset VAR [VAR2 ...]
 */

#define _POSIX_C_SOURCE 200809L  // 启用 unsetenv

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int cmd_xunset(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xunset - 删除环境变量\n\n");
        printf("用法:\n");
        printf("  xunset VAR [VAR2 ...]\n\n");
        printf("说明:\n");
        printf("  从环境中删除指定的变量。\n");
        printf("  Unset - 取消设置。\n\n");
        printf("参数:\n");
        printf("  VAR       要删除的环境变量名（可以多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xunset MYVAR               # 删除MYVAR变量\n");
        printf("  xunset VAR1 VAR2 VAR3      # 删除多个变量\n\n");
        printf("注意:\n");
        printf("  • 删除不存在的变量不会报错\n");
        printf("  • 无法删除某些系统保护的变量\n");
        printf("  • 变量名区分大小写\n\n");
        printf("相关命令:\n");
        printf("  xenv      - 显示所有环境变量\n");
        printf("  xexport   - 设置环境变量\n\n");
        printf("对应系统命令: unset\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xunset: missing variable name\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xunset --help' for more information.\n");
        return -1;
    }
    
    // 删除每个指定的变量
    int has_error = 0;
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *var_name = cmd->args[i];
        int invalid = 0;
        
        // 验证变量名
        size_t len = strlen(var_name);
        if (len == 0) {
            XSHELL_LOG_ERROR(ctx, "xunset: invalid variable name: empty\n");
            has_error = 1;
            continue;
        }
        
        for (size_t j = 0; j < len; j++) {
            char c = var_name[j];
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_')) {
                XSHELL_LOG_ERROR(ctx, "xunset: invalid variable name: '%s'\n", var_name);
                has_error = 1;
                invalid = 1;
                break;
            }
        }
        
        if (invalid) {
            continue;
        }
        
        // 删除环境变量
        if (unsetenv(var_name) != 0) {
            XSHELL_LOG_PERROR(ctx, "xunset");
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}




