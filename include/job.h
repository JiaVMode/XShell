/**
 * @file job.h
 * @brief 后台作业管理
 */

#ifndef JOB_H
#define JOB_H

#include <sys/types.h>
#include <stdbool.h>

// 最大作业数
#define MAX_JOBS 64

// 作业状态
typedef enum {
    JOB_RUNNING,    // 运行中
    JOB_STOPPED,    // 已停止
    JOB_DONE        // 已完成
} JobStatus;

// 作业结构
typedef struct {
    int id;             // 作业 ID (1-based)
    pid_t pid;          // 进程 ID
    JobStatus status;   // 作业状态
    char command[256];  // 命令字符串
    bool notified;      // 是否已通知完成
} Job;

// 初始化作业管理
void job_init(void);

// 添加作业
int job_add(pid_t pid, const char *command);

// 移除作业
void job_remove(int job_id);

// 根据作业 ID 获取作业
Job* job_get(int job_id);

// 根据 PID 获取作业
Job* job_get_by_pid(pid_t pid);

// 获取作业数量
int job_count(void);

// 更新所有作业状态
void job_update_status(void);

// 打印所有作业
void job_print_all(void);

// 清理已完成的作业
void job_cleanup_done(void);

// 信号处理器（用于 SIGCHLD）
void job_sigchld_handler(int sig);

// 信号处理器（用于 SIGINT - Ctrl+C）
void job_sigint_handler(int sig);

// 信号处理器（用于 SIGTSTP - Ctrl+Z）
void job_sigtstp_handler(int sig);

// 检查是否收到 SIGINT
int job_sigint_received(void);

// 检查并通知已完成的作业
void job_check_done(void);

// 安装信号处理器
void job_install_signal_handler(void);

#endif // JOB_H
