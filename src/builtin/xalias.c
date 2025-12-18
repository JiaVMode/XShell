/*
 * xalias.c - 设置和显示命令别名
 * 
 * 功能：创建命令别名
 * 用法：xalias [name='value']
 *       xalias name (显示指定别名)
 *       xalias (显示所有别名)
 */

#include "builtin.h"
#include "alias.h"
#include <stdio.h>
#include <string.h>

int cmd_xalias(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xalias - 设置和显示命令别名\n\n");
        printf("用法:\n");
        printf("  xalias [name='value']      # 设置别名\n");
        printf("  xalias name                # 显示指定别名\n");
        printf("  xalias                     # 显示所有别名\n\n");
        printf("说明:\n");
        printf("  创建命令别名，为常用命令设置简短的名称。\n");
        printf("  Alias - 别名。\n\n");
        printf("参数:\n");
        printf("  name='value'  别名名称和对应的命令\n");
        printf("  name          只显示指定别名\n\n");
        printf("选项:\n");
        printf("  --help        显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xalias ll='xls -lah'       # 设置ll别名\n");
        printf("  xalias la='xls -a'         # 设置la别名\n");
        printf("  xalias gs='xgrep -rn'      # 设置gs别名\n");
        printf("  xalias ll                  # 显示ll别名\n");
        printf("  xalias                     # 显示所有别名\n\n");
        printf("别名格式:\n");
        printf("  别名名称只能包含字母、数字和下划线\n");
        printf("  别名值可以包含空格，建议用单引号包围\n");
        printf("  使用等号连接名称和值：name='value'\n\n");
        printf("使用别名:\n");
        printf("  设置别名后，可以像使用普通命令一样使用别名：\n");
        printf("  xalias ll='xls -lah'\n");
        printf("  ll                         # 等同于 xls -lah\n\n");
        printf("注意:\n");
        printf("  • 别名不能递归展开\n");
        printf("  • 别名最多%d个\n", MAX_ALIASES);
        printf("  • 别名仅在当前Shell会话中有效\n\n");
        printf("相关命令:\n");
        printf("  xunalias  - 删除别名\n\n");
        printf("对应系统命令: alias\n");
        return 0;
    }
    
    // 没有参数：显示所有别名
    if (cmd->arg_count == 1) {
        alias_list();
        return 0;
    }
    
    // 处理参数
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *arg = cmd->args[i];
        
        // 查找等号
        char *eq = strchr(arg, '=');
        
        if (eq) {
            // name='value' 格式：设置别名
            // 分离名称和值
            size_t name_len = eq - arg;
            if (name_len == 0) {
                XSHELL_LOG_ERROR(ctx, "xalias: invalid format: '%s'\n", arg);
                return -1;
            }
            
            char name[64];
            if (name_len >= sizeof(name)) {
                XSHELL_LOG_ERROR(ctx, "xalias: alias name too long\n");
                return -1;
            }
            
            strncpy(name, arg, name_len);
            name[name_len] = '\0';
            
            // 验证别名名称
            for (size_t j = 0; j < name_len; j++) {
                char c = name[j];
                if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                      (c >= '0' && c <= '9') || c == '_')) {
                    XSHELL_LOG_ERROR(ctx, "xalias: invalid alias name: '%s'\n", name);
                    return -1;
                }
            }
            
            // 获取值（去除引号）
            const char *value = eq + 1;
            char value_buf[256];
            size_t value_len = strlen(value);
            
            // 去除首尾的单引号或双引号
            if (value_len >= 2 &&
                ((value[0] == '\'' && value[value_len - 1] == '\'') ||
                 (value[0] == '"' && value[value_len - 1] == '"'))) {
                if (value_len - 2 >= sizeof(value_buf)) {
                    XSHELL_LOG_ERROR(ctx, "xalias: alias value too long\n");
                    return -1;
                }
                strncpy(value_buf, value + 1, value_len - 2);
                value_buf[value_len - 2] = '\0';
                value = value_buf;
            }
            
            // 设置别名
            if (alias_get(name) == NULL && alias_count() >= MAX_ALIASES) {
                XSHELL_LOG_ERROR(ctx, "xalias: too many aliases (max %d)\n", MAX_ALIASES);
                return -1;
            }
            if (alias_set(name, value) != 0) {
                XSHELL_LOG_ERROR(ctx, "xalias: failed to set alias '%s'\n", name);
                return -1;
            }
        } else {
            // name 格式：显示指定别名
            const char *value = alias_get(arg);
            if (value) {
                printf("alias %s='%s'\n", arg, value);
            } else {
                XSHELL_LOG_ERROR(ctx, "xalias: %s: not found\n", arg);
                return -1;
            }
        }
    }
    
    return 0;
}




