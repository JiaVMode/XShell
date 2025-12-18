/*
 * xunalias.c - 删除命令别名
 * 
 * 功能：删除指定的别名
 * 用法：xunalias name [name2 ...]
 */

#include "builtin.h"
#include "alias.h"
#include <stdio.h>
#include <string.h>

int cmd_xunalias(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xunalias - 删除命令别名\n\n");
        printf("用法:\n");
        printf("  xunalias name [name2 ...]\n\n");
        printf("说明:\n");
        printf("  删除指定的命令别名。\n");
        printf("  Unalias - 取消别名。\n\n");
        printf("参数:\n");
        printf("  name      要删除的别名名称（可以多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xunalias ll                # 删除ll别名\n");
        printf("  xunalias ll la gs          # 删除多个别名\n\n");
        printf("注意:\n");
        printf("  • 删除不存在的别名会报错\n");
        printf("  • 别名名称区分大小写\n\n");
        printf("相关命令:\n");
        printf("  xalias    - 设置和显示别名\n\n");
        printf("对应系统命令: unalias\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xunalias: missing alias name\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xunalias --help' for more information.\n");
        return -1;
    }
    
    // 删除每个指定的别名
    int has_error = 0;
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *name = cmd->args[i];
        
        if (alias_remove(name) != 0) {
            XSHELL_LOG_ERROR(ctx, "xunalias: %s: not found\n", name);
            has_error = 1;
        }
    }
    
    return has_error ? -1 : 0;
}




