/**
 * @file job.c
 * @brief 后台作业管理实现
 */

#define _POSIX_C_SOURCE 200809L
#include "job.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>

// 全局作业列表
static Job g_jobs[MAX_JOBS];
static int g_next_job_id = 1;
static volatile sig_atomic_t g_sigchld_received = 0;

// 颜色定义
#define C_RESET   "\033[0m"
#define C_BOLD    "\033[1m"
#define C_GREEN   "\033[32m"
#define C_YELLOW  "\033[33m"
#define C_RED     "\033[31m"
#define C_CYAN    "\033[36m"

// 初始化作业管理
void job_init(void) {
    memset(g_jobs, 0, sizeof(g_jobs));
    g_next_job_id = 1;
}

// 添加作业
int job_add(pid_t pid, const char *command) {
    // 找一个空槽位
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid == 0) {
            g_jobs[i].id = g_next_job_id++;
            g_jobs[i].pid = pid;
            g_jobs[i].status = JOB_RUNNING;
            g_jobs[i].notified = false;
            strncpy(g_jobs[i].command, command, sizeof(g_jobs[i].command) - 1);
            g_jobs[i].command[sizeof(g_jobs[i].command) - 1] = '\0';
            return g_jobs[i].id;
        }
    }
    return -1;  // 没有空槽位
}

// 移除作业
void job_remove(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].id == job_id) {
            memset(&g_jobs[i], 0, sizeof(Job));
            return;
        }
    }
}

// 根据作业 ID 获取作业
Job* job_get(int job_id) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].id == job_id && g_jobs[i].pid != 0) {
            return &g_jobs[i];
        }
    }
    return NULL;
}

// 根据 PID 获取作业
Job* job_get_by_pid(pid_t pid) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid == pid) {
            return &g_jobs[i];
        }
    }
    return NULL;
}

// 获取作业数量
int job_count(void) {
    int count = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid != 0) {
            count++;
        }
    }
    return count;
}

// 更新所有作业状态
void job_update_status(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid != 0 && g_jobs[i].status != JOB_DONE) {
            int status;
            pid_t result = waitpid(g_jobs[i].pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
            
            if (result == g_jobs[i].pid) {
                if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    g_jobs[i].status = JOB_DONE;
                } else if (WIFSTOPPED(status)) {
                    g_jobs[i].status = JOB_STOPPED;
                } else if (WIFCONTINUED(status)) {
                    g_jobs[i].status = JOB_RUNNING;
                }
            } else if (result == -1) {
                // 进程不存在了
                g_jobs[i].status = JOB_DONE;
            }
        }
    }
}

// 获取状态字符串
static const char* job_status_str(JobStatus status) {
    switch (status) {
        case JOB_RUNNING: return "Running";
        case JOB_STOPPED: return "Stopped";
        case JOB_DONE:    return "Done";
        default:          return "Unknown";
    }
}

// 获取状态颜色
static const char* job_status_color(JobStatus status) {
    switch (status) {
        case JOB_RUNNING: return C_GREEN;
        case JOB_STOPPED: return C_YELLOW;
        case JOB_DONE:    return C_CYAN;
        default:          return C_RESET;
    }
}

// 打印所有作业
void job_print_all(void) {
    job_update_status();
    
    int printed = 0;
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid != 0) {
            printf("[%d] %s%-8s%s  %s%s%s &\n",
                   g_jobs[i].id,
                   job_status_color(g_jobs[i].status),
                   job_status_str(g_jobs[i].status),
                   C_RESET,
                   C_BOLD,
                   g_jobs[i].command,
                   C_RESET);
            printed++;
            
            // 如果已完成且未通知，标记为已通知
            if (g_jobs[i].status == JOB_DONE) {
                g_jobs[i].notified = true;
            }
        }
    }
    
    if (printed == 0) {
        printf("当前没有后台任务。\n");
    }
}

// 清理已完成的作业
void job_cleanup_done(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid != 0 && g_jobs[i].status == JOB_DONE && g_jobs[i].notified) {
            memset(&g_jobs[i], 0, sizeof(Job));
        }
    }
}

// 检查并报告已完成的作业
void job_check_done(void) {
    job_update_status();
    
    for (int i = 0; i < MAX_JOBS; i++) {
        if (g_jobs[i].pid != 0 && g_jobs[i].status == JOB_DONE && !g_jobs[i].notified) {
            printf("\n[%d]  Done                    %s\n", g_jobs[i].id, g_jobs[i].command);
            g_jobs[i].notified = true;
        }
    }
    
    job_cleanup_done();
}

// 信号处理器
void job_sigchld_handler(int sig) {
    (void)sig;
    g_sigchld_received = 1;
    
    // 收集所有已终止的子进程
    pid_t pid;
    int status;
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
        Job *job = job_get_by_pid(pid);
        if (job) {
            if (WIFEXITED(status) || WIFSIGNALED(status)) {
                job->status = JOB_DONE;
            } else if (WIFSTOPPED(status)) {
                job->status = JOB_STOPPED;
            }
        }
    }
}

// 全局变量：标记是否收到 SIGINT
static volatile sig_atomic_t g_sigint_received = 0;

// SIGINT 处理器（Ctrl+C）
// 目的：Ctrl+C 只中断当前输入行，不退出 Shell
void job_sigint_handler(int sig) {
    (void)sig;
    g_sigint_received = 1;
    // 打印换行，让用户知道 Ctrl+C 被按下了
    write(STDOUT_FILENO, "\n", 1);
}

// SIGTSTP 处理器（Ctrl+Z）
// 目的：Ctrl+Z 只对前台进程有效，Shell 本身忽略
void job_sigtstp_handler(int sig) {
    (void)sig;
    // Shell 本身忽略 SIGTSTP，只有前台子进程会被挂起
}

// 检查是否收到 SIGINT
int job_sigint_received(void) {
    if (g_sigint_received) {
        g_sigint_received = 0;
        return 1;
    }
    return 0;
}

// 安装信号处理器
void job_install_signal_handler(void) {
    struct sigaction sa;
    
    // 安装 SIGCHLD 处理器（子进程状态变化）
    sa.sa_handler = job_sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);
    
    // 安装 SIGINT 处理器（Ctrl+C）
    sa.sa_handler = job_sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;  // 不使用 SA_RESTART，让被中断的读取返回错误
    sigaction(SIGINT, &sa, NULL);
    
    // 安装 SIGTSTP 处理器（Ctrl+Z）
    sa.sa_handler = job_sigtstp_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &sa, NULL);
    
    // 忽略 SIGQUIT（Ctrl+\）
    signal(SIGQUIT, SIG_IGN);
    
    // 忽略 SIGTTOU 和 SIGTTIN（后台进程读写终端）
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}
