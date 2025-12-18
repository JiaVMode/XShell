/*
 * xbg.c - 将任务放到后台
 * 
 * 功能：将指定的已停止任务放到后台继续执行
 * 用法：xbg [job_id]
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

int cmd_xbg(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xbg - 将任务放到后台\n\n");
        printf("用法:\n");
        printf("  xbg [job_id]\n\n");
        printf("说明:\n");
        printf("  将指定的已停止任务放到后台继续执行。\n");
        printf("  Background - 后台。\n\n");
        printf("参数:\n");
        printf("  job_id    任务ID（可选，默认是最后一个任务）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xbg                        # 将最后一个任务放到后台\n");
        printf("  xbg 1                      # 将任务1放到后台\n");
        printf("  xbg %1                     # 同上（%1表示任务1）\n\n");
        printf("注意:\n");
        printf("  • 只能对已停止的任务使用\n");
        printf("  • 任务会在后台继续执行\n");
        printf("  • 目前XShell还不支持后台执行（&），此功能暂时不可用\n\n");
        printf("对应系统命令: bg\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count > 2) {
        XSHELL_LOG_ERROR(ctx, "xbg: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xbg --help' for more information.\n");
        return -1;
    }
    
    // TODO: 实现后台任务管理
    // 目前XShell还不支持后台执行（&），所以无法将任务放到后台
    if (cmd->arg_count == 2) {
        const char *job_id = cmd->args[1];
        printf("xbg: 任务 %s 不存在\n", job_id);
    } else {
        printf("xbg: 当前没有后台任务\n");
    }
    printf("提示：XShell目前还不支持后台执行（&），此功能将在后续版本中实现。\n");
    
    return -1;
}



