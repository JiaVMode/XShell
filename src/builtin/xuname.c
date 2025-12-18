// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <sys/utsname.h>        // 系统信息（uname）

// ============================================
// xuname 命令实现函数
// ============================================
// 命令名称：xuname
// 对应系统命令：uname
// 功能：显示系统信息
// 用法：xuname [选项]
//
// 选项：
//   -a        显示所有信息
//   -s        显示内核名称（默认）
//   -n        显示网络节点主机名
//   -r        显示内核版本
//   -v        显示内核发布版本
//   -m        显示机器硬件名称
//   --help    显示帮助信息
//
// 示例：
//   xuname              # 显示内核名称
//   xuname -a           # 显示所有信息
//   xuname -r           # 显示内核版本
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xuname(Command *cmd, ShellContext *ctx) {
    (void)ctx;
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xuname - 显示系统信息\n\n");
        printf("用法:\n");
        printf("  xuname [选项]\n\n");
        printf("说明:\n");
        printf("  显示系统信息，包括内核名称、版本等。\n");
        printf("  不带选项时，默认显示内核名称。\n\n");
        printf("选项:\n");
        printf("  -a        显示所有信息\n");
        printf("  -s        显示内核名称（默认）\n");
        printf("  -n        显示网络节点主机名\n");
        printf("  -r        显示内核版本\n");
        printf("  -v        显示内核发布版本\n");
        printf("  -m        显示机器硬件名称\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xuname\n");
        printf("    显示内核名称（例如：Linux）\n\n");
        printf("  xuname -a\n");
        printf("    显示所有系统信息\n\n");
        printf("  xuname -r\n");
        printf("    显示内核版本（例如：6.2.0-26-generic）\n\n");
        printf("  xuname -m\n");
        printf("    显示机器硬件名称（例如：x86_64）\n\n");
        printf("对应系统命令: uname\n");
        return 0;
    }

    // 步骤1：获取系统信息
    struct utsname info;
    if (uname(&info) == -1) {
        XSHELL_LOG_PERROR(ctx, "xuname");
        return -1;
    }

    // 步骤2：解析选项并输出相应信息
    int show_all = 0;                           // 显示所有信息标志
    int show_sysname = 0;                       // 显示内核名称标志
    int show_nodename = 0;                      // 显示主机名标志
    int show_release = 0;                       // 显示内核版本标志
    int show_version = 0;                       // 显示发布版本标志
    int show_machine = 0;                       // 显示机器名称标志

    // 如果没有参数，默认显示内核名称
    if (cmd->arg_count == 1) {
        show_sysname = 1;
    } else {
        // 解析所有选项
        for (int i = 1; i < cmd->arg_count; i++) {
            if (strcmp(cmd->args[i], "-a") == 0) {
                show_all = 1;
            } else if (strcmp(cmd->args[i], "-s") == 0) {
                show_sysname = 1;
            } else if (strcmp(cmd->args[i], "-n") == 0) {
                show_nodename = 1;
            } else if (strcmp(cmd->args[i], "-r") == 0) {
                show_release = 1;
            } else if (strcmp(cmd->args[i], "-v") == 0) {
                show_version = 1;
            } else if (strcmp(cmd->args[i], "-m") == 0) {
                show_machine = 1;
            } else {
                XSHELL_LOG_ERROR(ctx, "xuname: invalid option: '%s'\n", cmd->args[i]);
                XSHELL_LOG_ERROR(ctx, "Try 'xuname --help' for more information.\n");
                return -1;
            }
        }
    }

    // 步骤3：输出信息
    int first = 1;                              // 第一个输出标志（用于空格分隔）

    // 如果指定了 -a，显示所有信息
    if (show_all) {
        printf("%s %s %s %s %s\n", 
               info.sysname, 
               info.nodename, 
               info.release, 
               info.version, 
               info.machine);
        return 0;
    }

    // 否则根据选项依次输出
    if (show_sysname) {
        if (!first) printf(" ");
        printf("%s", info.sysname);
        first = 0;
    }
    if (show_nodename) {
        if (!first) printf(" ");
        printf("%s", info.nodename);
        first = 0;
    }
    if (show_release) {
        if (!first) printf(" ");
        printf("%s", info.release);
        first = 0;
    }
    if (show_version) {
        if (!first) printf(" ");
        printf("%s", info.version);
        first = 0;
    }
    if (show_machine) {
        if (!first) printf(" ");
        printf("%s", info.machine);
        first = 0;
    }

    printf("\n");

    // 步骤4：返回成功
    return 0;
}

