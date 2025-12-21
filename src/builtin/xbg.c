/*
 * xbg.c - 将任务放到后台
 * 
 * 功能：将指定的已停止任务放到后台继续执行
 * 用法：xbg [job_id]
 */

#include "builtin.h"
#include "job.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>

int cmd_xbg(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xbg - 将任务放到后台\n\n");
        printf("用法:\n");
        printf("  xbg [job_id]\n\n");
        printf("说明:\n");
        printf("  将指定的已停止任务放到后台继续执行。\n\n");
        printf("参数:\n");
        printf("  job_id    任务ID（可选，默认是最后一个停止的任务）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xbg                        # 将最后一个停止的任务放到后台\n");
        printf("  xbg 1                      # 将任务1放到后台\n\n");
        printf("对应系统命令: bg\n");
        return 0;
    }
    
    // 获取作业 ID
    int job_id = -1;
    
    if (cmd->arg_count >= 2) {
        const char *arg = cmd->args[1];
        if (arg[0] == '%') arg++;
        job_id = atoi(arg);
    } else {
        // 没有指定，找最后一个停止的作业
        for (int i = 1; i <= 64; i++) {
            Job *job = job_get(i);
            if (job != NULL && job->status == JOB_STOPPED) {
                job_id = i;
            }
        }
    }
    
    if (job_id <= 0) {
        printf("xbg: 当前没有停止的任务\n");
        return -1;
    }
    
    Job *job = job_get(job_id);
    if (job == NULL) {
        printf("xbg: 任务 %d 不存在\n", job_id);
        return -1;
    }
    
    if (job->status != JOB_STOPPED) {
        printf("xbg: 任务 %d 不是停止状态\n", job_id);
        return -1;
    }
    
    // 发送 SIGCONT 继续执行
    kill(job->pid, SIGCONT);
    job->status = JOB_RUNNING;
    
    printf("[%d]+ %s &\n", job->id, job->command);
    
    return 0;
}
