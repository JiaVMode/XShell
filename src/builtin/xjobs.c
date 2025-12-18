/*
 * xjobs.c - 显示后台任务
 * 
 * 功能：显示当前Shell的后台任务列表
 * 用法：xjobs
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>

int cmd_xjobs(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xjobs - 显示后台任务\n\n");
        printf("用法:\n");
        printf("  xjobs\n\n");
        printf("说明:\n");
        printf("  显示当前Shell的所有后台任务。\n");
        printf("  Jobs - 任务。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("输出格式:\n");
        printf("  [任务ID] 状态  命令\n");
        printf("  例如: [1] Running  xsleep 10 &\n\n");
        printf("示例:\n");
        printf("  xjobs                      # 显示所有后台任务\n\n");
        printf("注意:\n");
        printf("  • 任务ID从1开始递增\n");
        printf("  • 状态可能是：Running, Stopped, Done\n");
        printf("  • 目前XShell还不支持后台执行（&），此命令暂时为空\n\n");
        printf("对应系统命令: jobs\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count > 1) {
        XSHELL_LOG_ERROR(ctx, "xjobs: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xjobs --help' for more information.\n");
        return -1;
    }
    
    // TODO: 实现后台任务管理
    // 目前XShell还不支持后台执行（&），所以没有任务可显示
    printf("当前没有后台任务。\n");
    printf("提示：XShell目前还不支持后台执行（&），此功能将在后续版本中实现。\n");
    
    return 0;
}



