// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf, fopen, fgets）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（getpid, getppid）

// ============================================
// xps 命令实现函数
// ============================================
// 命令名称：xps
// 对应系统命令：ps
// 功能：显示当前Shell的进程信息
// 用法：xps
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xps                 # 显示进程信息
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
//
// 注意：
//   这是简化版实现，只显示当前Shell进程的基本信息
// ============================================

int cmd_xps(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xps 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xps - 显示进程信息\n\n");
        printf("用法:\n");
        printf("  xps\n\n");
        printf("说明:\n");
        printf("  显示当前Shell进程的基本信息。\n");
        printf("  包括进程ID、父进程ID、状态等。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xps\n");
        printf("    显示当前Shell进程信息\n\n");
        printf("注意:\n");
        printf("  这是简化版实现，仅显示当前Shell进程。\n");
        printf("  要查看所有进程，请使用系统的 ps 命令。\n\n");
        printf("对应系统命令: ps\n");
        return 0;
    }

    // 步骤1：获取进程ID
    pid_t pid = getpid();                       // 当前进程ID
    pid_t ppid = getppid();                     // 父进程ID

    // 步骤2：输出表头
    printf("  PID  PPID   S  CMD\n");

    // 步骤3：读取进程状态
    char stat_path[256];
    char state = '?';                           // 进程状态
    char comm[256] = "xshell";                  // 命令名称

    snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);
    FILE *fp = fopen(stat_path, "r");
    if (fp != NULL) {
        // 从 /proc/[pid]/stat 读取进程状态
        // 格式：pid (comm) state ppid ...
        int read_pid;
        fscanf(fp, "%d (%255[^)]) %c", &read_pid, comm, &state);
        fclose(fp);
    }

    // 步骤4：输出进程信息
    printf("%5d %5d   %c  %s\n", pid, ppid, state, comm);

    // 步骤5：返回成功
    return 0;
}

