/*
 * xsleep.c - 休眠命令
 * 
 * 功能：休眠指定秒数
 * 用法：xsleep <seconds>
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>

// 检查字符串是否是有效的数字
static int is_valid_number(const char *str) {
    if (!str || *str == '\0') {
        return 0;
    }
    
    // 可以有前导空格
    while (isspace(*str)) {
        str++;
    }
    
    // 可以有正负号
    if (*str == '+' || *str == '-') {
        if (*str == '-') {
            return 0; // 不允许负数
        }
        str++;
    }
    
    // 至少要有一个数字
    if (!isdigit(*str)) {
        return 0;
    }
    
    // 检查所有字符都是数字
    while (*str) {
        if (!isdigit(*str)) {
            return 0;
        }
        str++;
    }
    
    return 1;
}

int cmd_xsleep(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xsleep - 休眠指定秒数\n\n");
        printf("用法:\n");
        printf("  xsleep <seconds>\n\n");
        printf("说明:\n");
        printf("  暂停执行指定的秒数。\n");
        printf("  Sleep - 休眠。\n\n");
        printf("参数:\n");
        printf("  seconds   休眠的秒数（正整数）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xsleep 1                   # 休眠1秒\n");
        printf("  xsleep 5                   # 休眠5秒\n");
        printf("  xsleep 60                  # 休眠60秒（1分钟）\n\n");
        printf("注意:\n");
        printf("  • seconds必须是非负整数\n");
        printf("  • 休眠期间Shell将被阻塞\n");
        printf("  • 可以用Ctrl+C中断休眠\n\n");
        printf("对应系统命令: sleep\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xsleep: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xsleep --help' for more information.\n");
        return -1;
    }
    
    if (cmd->arg_count > 2) {
        XSHELL_LOG_ERROR(ctx, "xsleep: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xsleep --help' for more information.\n");
        return -1;
    }
    
    // 验证参数
    const char *seconds_str = cmd->args[1];
    if (!is_valid_number(seconds_str)) {
        XSHELL_LOG_ERROR(ctx, "xsleep: invalid time interval '%s'\n", seconds_str);
        return -1;
    }
    
    // 转换为整数
    int seconds = atoi(seconds_str);
    if (seconds < 0) {
        XSHELL_LOG_ERROR(ctx, "xsleep: invalid time interval '%s'\n", seconds_str);
        return -1;
    }
    
    // 休眠
    sleep(seconds);
    
    return 0;
}




