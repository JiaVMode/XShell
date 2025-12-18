// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（gethostname）
#include <limits.h>             // 系统限制（HOST_NAME_MAX）

// 如果系统没有定义 HOST_NAME_MAX，使用默认值
#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 256
#endif

// ============================================
// xhostname 命令实现函数
// ============================================
// 命令名称：xhostname
// 对应系统命令：hostname
// 功能：显示主机名
// 用法：xhostname
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xhostname           # 显示主机名
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xhostname(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xhostname - 显示主机名\n\n");
        printf("用法:\n");
        printf("  xhostname\n\n");
        printf("说明:\n");
        printf("  显示当前系统的主机名。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xhostname\n");
        printf("    显示主机名（例如：ubuntu-server）\n\n");
        printf("对应系统命令: hostname\n");
        return 0;
    }

    // 步骤1：获取主机名
    char hostname[HOST_NAME_MAX + 1];
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        XSHELL_LOG_PERROR(ctx, "xhostname");
        return -1;
    }

    // 步骤2：输出主机名
    printf("%s\n", hostname);

    // 步骤3：返回成功
    return 0;
}

