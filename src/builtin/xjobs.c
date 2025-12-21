/*
 * xjobs.c - 显示后台任务
 * 
 * 功能：显示当前Shell的后台任务列表
 * 用法：xjobs
 */

#include "builtin.h"
#include "job.h"
#include <stdio.h>
#include <string.h>

int cmd_xjobs(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xjobs - 显示后台任务\n\n");
        printf("用法:\n");
        printf("  xjobs\n\n");
        printf("说明:\n");
        printf("  显示当前Shell的所有后台任务。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("输出格式:\n");
        printf("  [任务ID] 状态  命令\n");
        printf("  例如: [1] Running  sleep 10 &\n\n");
        printf("示例:\n");
        printf("  xjobs                      # 显示所有后台任务\n\n");
        printf("提示:\n");
        printf("  • 使用 '命令 &' 在后台执行命令\n");
        printf("  • 使用 'xfg [id]' 将后台任务调到前台\n\n");
        printf("对应系统命令: jobs\n");
        return 0;
    }
    
    // 显示所有任务
    job_print_all();
    
    return 0;
}
