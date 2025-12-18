/*
 * xtime.c - 测量命令执行时间
 * 
 * 功能：执行命令并测量其执行时间
 * 用法：xtime <command>
 */

#include "builtin.h"
#include "parser.h"
#include "executor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

int cmd_xtime(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtime - 测量命令执行时间\n\n");
        printf("用法:\n");
        printf("  xtime <command> [args...]\n\n");
        printf("说明:\n");
        printf("  执行命令并测量其执行时间（精确到毫秒）。\n");
        printf("  Time - 时间。\n\n");
        printf("参数:\n");
        printf("  command   要执行的命令及其参数\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xtime xls                    # 测量xls执行时间\n");
        printf("  xtime xsleep 2               # 测量休眠时间\n");
        printf("  xtime xcalc '100 * 100'      # 测量计算时间\n\n");
        printf("输出格式:\n");
        printf("  命令输出...\n");
        printf("  执行时间: X.XXX秒\n\n");
        printf("注意:\n");
        printf("  • 时间精度为毫秒\n");
        printf("  • 只测量命令本身的执行时间\n");
        printf("  • 不包括命令解析时间\n\n");
        printf("对应系统命令: time\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xtime: missing command\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xtime --help' for more information.\n");
        return -1;
    }
    
    // 构建要执行的命令（去掉xtime）
    // 将剩余参数组合成命令字符串
    char command_str[1024] = "";
    for (int i = 1; i < cmd->arg_count; i++) {
        if (i > 1) {
            strcat(command_str, " ");
        }
        strcat(command_str, cmd->args[i]);
    }
    
    // 解析命令
    Command *time_cmd = parse_command(command_str);
    if (time_cmd == NULL) {
        XSHELL_LOG_ERROR(ctx, "xtime: parse error\n");
        return -1;
    }
    
    // 记录开始时间
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // 执行命令
    int result = execute_command(time_cmd, ctx);
    
    // 记录结束时间
    gettimeofday(&end, NULL);
    
    // 计算时间差（秒）
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds / 1000000.0;
    
    // 输出执行时间
    printf("\n执行时间: %.3f秒\n", elapsed);
    
    // 释放命令对象
    free_command(time_cmd);
    
    return result;
}

