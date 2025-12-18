/*
 * xkill.c - 终止进程
 * 
 * 功能：向指定进程发送信号（默认SIGTERM）
 * 用法：xkill <pid> [-s signal]
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <ctype.h>

// 将信号名转换为信号编号
static int signal_name_to_num(const char *name) {
    // 去掉SIG前缀（如果有）
    const char *sig_name = name;
    if (strncmp(name, "SIG", 3) == 0) {
        sig_name = name + 3;
    }
    
    // 常见信号
    if (strcasecmp(sig_name, "TERM") == 0 || strcasecmp(sig_name, "TERMINATE") == 0) {
        return SIGTERM;
    } else if (strcasecmp(sig_name, "KILL") == 0) {
        return SIGKILL;
    } else if (strcasecmp(sig_name, "INT") == 0 || strcasecmp(sig_name, "INTERRUPT") == 0) {
        return SIGINT;
    } else if (strcasecmp(sig_name, "HUP") == 0 || strcasecmp(sig_name, "HANGUP") == 0) {
        return SIGHUP;
    } else if (strcasecmp(sig_name, "STOP") == 0) {
        return SIGSTOP;
    } else if (strcasecmp(sig_name, "CONT") == 0 || strcasecmp(sig_name, "CONTINUE") == 0) {
        return SIGCONT;
    }
    
    return -1;
}

int cmd_xkill(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xkill - 终止进程\n\n");
        printf("用法:\n");
        printf("  xkill <pid> [-s signal]\n\n");
        printf("说明:\n");
        printf("  向指定进程ID发送信号（默认SIGTERM）。\n");
        printf("  Kill - 终止。\n\n");
        printf("参数:\n");
        printf("  pid       进程ID（正整数）\n\n");
        printf("选项:\n");
        printf("  -s signal 要发送的信号（默认：SIGTERM）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("常用信号:\n");
        printf("  SIGTERM   - 终止信号（默认，允许进程清理）\n");
        printf("  SIGKILL   - 强制终止（无法被忽略）\n");
        printf("  SIGINT    - 中断信号（类似Ctrl+C）\n");
        printf("  SIGHUP    - 挂起信号\n");
        printf("  SIGSTOP   - 暂停进程\n");
        printf("  SIGCONT   - 继续进程\n\n");
        printf("示例:\n");
        printf("  xkill 1234                  # 终止进程1234\n");
        printf("  xkill 1234 -s SIGKILL        # 强制终止\n");
        printf("  xkill 1234 -s KILL           # 同上（可省略SIG前缀）\n\n");
        printf("注意:\n");
        printf("  • 需要进程ID（PID）\n");
        printf("  • 默认发送SIGTERM信号\n");
        printf("  • SIGKILL无法被捕获或忽略\n");
        printf("  • 只能终止有权限的进程\n\n");
        printf("对应系统命令: kill\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xkill: missing pid argument\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xkill --help' for more information.\n");
        return -1;
    }
    
    int signal = SIGTERM; // 默认信号
    int pid_arg_index = 1;
    
    // 解析选项
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "-s") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xkill: option requires an argument -- 's'\n");
                return -1;
            }
            
            const char *sig_name = cmd->args[++i];
            int sig_num = signal_name_to_num(sig_name);
            
            if (sig_num == -1) {
                // 尝试作为数字解析
                sig_num = atoi(sig_name);
                if (sig_num <= 0) {
                    XSHELL_LOG_ERROR(ctx, "xkill: invalid signal '%s'\n", sig_name);
                    return -1;
                }
            }
            
            signal = sig_num;
        } else if (strcmp(cmd->args[i], "--help") == 0) {
            // 已经在上面处理了
        } else {
            pid_arg_index = i;
        }
    }
    
    // 解析PID
    const char *pid_str = cmd->args[pid_arg_index];
    
    // 验证PID是数字
    for (int i = 0; pid_str[i] != '\0'; i++) {
        if (!isdigit(pid_str[i])) {
            XSHELL_LOG_ERROR(ctx, "xkill: invalid pid '%s'\n", pid_str);
            return -1;
        }
    }
    
    pid_t pid = atoi(pid_str);
    if (pid <= 0) {
        XSHELL_LOG_ERROR(ctx, "xkill: invalid pid '%s'\n", pid_str);
        return -1;
    }
    
    // 发送信号
    if (kill(pid, signal) != 0) {
        XSHELL_LOG_PERROR(ctx, "xkill");
        return -1;
    }
    
    printf("已向进程 %d 发送信号 %d\n", pid, signal);
    
    return 0;
}

