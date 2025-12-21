/*
 * xfg.c - 将后台任务调到前台
 * 
 * 功能：将指定的后台任务调到前台执行
 * 用法：xfg [job_id]
 */

#include "builtin.h"
#include "job.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <signal.h>

int cmd_xfg(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xfg - 将后台任务调到前台\n\n");
        printf("用法:\n");
        printf("  xfg [job_id]\n\n");
        printf("说明:\n");
        printf("  将指定的后台任务调到前台执行。\n\n");
        printf("参数:\n");
        printf("  job_id    任务ID（可选，默认是最后一个任务）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xfg                        # 将最后一个任务调到前台\n");
        printf("  xfg 1                      # 将任务1调到前台\n\n");
        printf("对应系统命令: fg\n");
        return 0;
    }
    
    // 获取作业 ID
    int job_id = -1;
    
    if (cmd->arg_count >= 2) {
        // 跳过可能的 % 前缀
        const char *arg = cmd->args[1];
        if (arg[0] == '%') arg++;
        job_id = atoi(arg);
    } else {
        // 没有指定，找最后一个运行中的作业
        for (int i = 1; i <= 64; i++) {
            Job *job = job_get(i);
            if (job != NULL && job->status == JOB_RUNNING) {
                job_id = i;
            }
        }
    }
    
    if (job_id <= 0) {
        printf("xfg: 当前没有后台任务\n");
        return -1;
    }
    
    Job *job = job_get(job_id);
    if (job == NULL) {
        printf("xfg: 任务 %d 不存在\n", job_id);
        return -1;
    }
    
    printf("%s\n", job->command);
    
    // 如果是停止状态，发送 SIGCONT
    if (job->status == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
        job->status = JOB_RUNNING;
    }
    
    // 等待进程完成
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    
    if (WIFSTOPPED(status)) {
        job->status = JOB_STOPPED;
        printf("\n[%d]+  Stopped                 %s\n", job->id, job->command);
    } else {
        // 进程结束，移除作业
        job_remove(job_id);
    }
    
    return 0;
}
