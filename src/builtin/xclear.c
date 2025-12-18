/*
 * xclear.c - 清屏命令
 * 
 * 功能：清除终端屏幕内容
 * 用法：xclear
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>

int cmd_xclear(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xclear - 清屏\n\n");
        printf("用法:\n");
        printf("  xclear [--help]\n\n");
        printf("说明:\n");
        printf("  清除终端屏幕内容。\n");
        printf("  Clear - 清除。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xclear                     # 清除屏幕\n\n");
        printf("实现方式:\n");
        printf("  使用ANSI转义序列清屏和移动光标到(0,0)\n\n");
        printf("对应系统命令: clear\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count > 1) {
        XSHELL_LOG_ERROR(ctx, "xclear: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xclear --help' for more information.\n");
        return -1;
    }
    
    // 使用ANSI转义序列清屏
    // \033[2J - 清除整个屏幕
    // \033[H  - 移动光标到左上角(0,0)
    printf("\033[2J\033[H");
    fflush(stdout);
    
    return 0;
}




