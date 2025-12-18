/*
 * xexport.c - 设置环境变量
 * 
 * 功能：设置或导出环境变量
 * 用法：xexport VAR=value
 *       xexport VAR (显示指定变量)
 * 
 * 选项：
 *   -p    显示所有导出的变量（格式：export VAR="value"）
 *   --help 显示帮助信息
 */

#define _POSIX_C_SOURCE 200809L  // 启用 setenv

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

extern char **environ;

// 显示单个环境变量
static void print_export_var(const char *name) {
    char *value = getenv(name);
    if (value) {
        printf("export %s=\"%s\"\n", name, value);
    }
}

// 显示所有环境变量（export格式）
static void print_all_exports(void) {
    if (environ == NULL) {
        return;
    }
    
    for (char **env = environ; *env != NULL; env++) {
        // 找到等号位置
        char *eq = strchr(*env, '=');
        if (eq) {
            // 提取变量名
            size_t name_len = eq - *env;
            char name[256];
            if (name_len < sizeof(name)) {
                strncpy(name, *env, name_len);
                name[name_len] = '\0';
                printf("export %s=\"%s\"\n", name, eq + 1);
            }
        }
    }
}

int cmd_xexport(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xexport - 设置环境变量\n\n");
        printf("用法:\n");
        printf("  xexport VAR=value          # 设置环境变量\n");
        printf("  xexport VAR                # 显示指定变量\n");
        printf("  xexport -p                 # 显示所有导出的变量\n");
        printf("  xexport                    # 显示所有导出的变量\n\n");
        printf("说明:\n");
        printf("  设置或导出环境变量到当前Shell环境。\n");
        printf("  Export - 导出。\n\n");
        printf("参数:\n");
        printf("  VAR=value 变量名和值，用等号连接\n");
        printf("  VAR       只显示指定变量的值\n\n");
        printf("选项:\n");
        printf("  -p        以 export 格式显示所有变量\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xexport PATH=/usr/bin      # 设置PATH\n");
        printf("  xexport MYVAR=hello        # 设置自定义变量\n");
        printf("  xexport MYVAR              # 显示MYVAR的值\n");
        printf("  xexport -p                 # 显示所有变量\n");
        printf("  xexport                    # 同 xexport -p\n\n");
        printf("注意:\n");
        printf("  • 变量名只能包含字母、数字和下划线\n");
        printf("  • 变量名不能以数字开头\n");
        printf("  • 值可以包含空格（建议用引号）\n");
        printf("  • 设置的变量仅在当前Shell及其子进程中有效\n\n");
        printf("相关命令:\n");
        printf("  xenv      - 显示所有环境变量\n");
        printf("  xunset    - 删除环境变量\n\n");
        printf("对应系统命令: export\n");
        return 0;
    }
    
    // 解析选项
    int start_index = 1;
    int print_format = 0;
    
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "-p") == 0) {
        print_format = 1;
        start_index = 2;
    }
    
    // 没有参数：显示所有导出的变量
    if (cmd->arg_count == 1 || (print_format && cmd->arg_count == 2)) {
        print_all_exports();
        return 0;
    }
    
    // 处理每个参数
    for (int i = start_index; i < cmd->arg_count; i++) {
        const char *arg = cmd->args[i];
        
        // 查找等号
        char *eq = strchr(arg, '=');
        
        if (eq) {
            // VAR=value 格式：设置环境变量
            // 分离变量名和值
            size_t name_len = eq - arg;
            if (name_len == 0) {
                XSHELL_LOG_ERROR(ctx, "xexport: invalid format: '%s'\n", arg);
                return -1;
            }
            
            char name[256];
            if (name_len >= sizeof(name)) {
                XSHELL_LOG_ERROR(ctx, "xexport: variable name too long\n");
                return -1;
            }
            
            strncpy(name, arg, name_len);
            name[name_len] = '\0';
            
            // 验证变量名
            for (size_t j = 0; j < name_len; j++) {
                char c = name[j];
                if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '_')) {
                    XSHELL_LOG_ERROR(ctx, "xexport: invalid variable name: '%s'\n", name);
                    return -1;
                }
                // 不能以数字开头
                if (j == 0 && c >= '0' && c <= '9') {
                    XSHELL_LOG_ERROR(ctx, "xexport: variable name cannot start with digit: '%s'\n", name);
                    return -1;
                }
            }
            
            // 设置环境变量
            const char *value = eq + 1;
            
            if (setenv(name, value, 1) != 0) {
                XSHELL_LOG_PERROR(ctx, "xexport");
                XSHELL_LOG_ERROR(ctx, "Failed to set environment variable %s\n", name);
                return -1;
            }
        } else {
            // VAR 格式：显示指定变量
            print_export_var(arg);
        }
    }
    
    return 0;
}




