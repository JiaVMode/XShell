// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror）
#include <string.h>                 // 字符串处理（strcmp）
#include <unistd.h>                 // UNIX 标准函数（getcwd）
#include <limits.h>                 // 系统限制常量（PATH_MAX）

// xpwd命令实现
// 命令名称：xpwd
// 对应系统命令：pwd
// 使用示例：[\home\user\]# xpwd
//          /home/user
int cmd_xpwd(Command *cmd, ShellContext *ctx) {
    // 步骤1：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xpwd - 显示当前工作目录\n\n");
        printf("用法:\n");
        printf("  xpwd [--help]\n\n");
        printf("说明:\n");
        printf("  显示当前工作目录的完整路径。\n");
        printf("  Print Working Directory - 打印当前工作目录。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xpwd\n");
        printf("    输出: /home/lab/LJ/XShell\n\n");
        printf("对应系统命令: pwd\n");
        return 0;
    }

    // 步骤2：参数标记为未使用（xpwd 命令不需要额外参数）
    (void)ctx;                      // Shell 上下文也暂时用不到（getcwd 是独立的系统调用）

    // 步骤3：声明缓冲区存储当前工作目录
    char cwd[PATH_MAX];             // PATH_MAX 是系统定义的最大路径长度（通常4096）

    // 步骤4：调用系统函数获取当前工作目录
    // getcwd(buf, size) 成功返回buf指针，失败返回NULL
    if (getcwd(cwd, sizeof(cwd)) != NULL) {     // 获取成功
        printf("%s\n", cwd);                    // 打印当前目录路径（加换行符）
        return 0;                               // 返回 0 表示命令执行成功
    } else {                                    // 获取失败（极少情况：权限问题、路径被删除等）
        XSHELL_LOG_PERROR(ctx, "getcwd");
        return -1;                              // 返回 -1 表示命令执行失败
    }
}

// quit 命令实现
// 命令名称：quit
// 功能：退出 Shell 程序
// 实现原理：设置 ctx->running = 0，导致 shell_loop() 中的 while 循环退出
// 使用示例：[\home\user\]# quit
//          ######## Quiting XShell ########
int cmd_quit(Command *cmd, ShellContext *ctx) {
    // 步骤1：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("quit - 退出 XShell\n\n");
        printf("用法:\n");
        printf("  quit [--help]\n\n");
        printf("说明:\n");
        printf("  退出 XShell 程序，返回到系统 Shell。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("快捷键:\n");
        printf("  Ctrl+D    等同于 quit 命令\n\n");
        printf("示例:\n");
        printf("  quit\n");
        printf("    退出 XShell\n");
        return 0;
    }
    
    // 步骤2：设置 Shell 退出标志
    // ctx->running 是 Shell 主循环的控制变量：
    //   - 1 表示继续运行（默认值）
    //   - 0 表示退出 Shell
    ctx->running = 0;                           // 设置为 0，shell_loop() 将在下次循环检查时退出
    
    // 步骤3：返回成功状态
    return 0;                                   // quit 命令总是成功（无法失败）
}