// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（rmdir）

// ============================================
// xrmdir 命令实现函数
// ============================================
// 命令名称：xrmdir
// 对应系统命令：rmdir
// 功能：删除空目录
// 用法：xrmdir <目录名> [目录名2 ...]
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xrmdir test                # 删除空目录 test
//   xrmdir dir1 dir2 dir3      # 删除多个空目录
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xrmdir(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xrmdir - 删除空目录\n\n");
        printf("用法:\n");
        printf("  xrmdir <目录名> [目录名2 ...]\n\n");
        printf("说明:\n");
        printf("  删除一个或多个空目录。\n");
        printf("  目录必须为空（不包含任何文件或子目录）。\n");
        printf("  如果目录不为空，操作会失败。\n\n");
        printf("参数:\n");
        printf("  目录名    要删除的空目录（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xrmdir test\n");
        printf("    删除空目录 test\n\n");
        printf("  xrmdir dir1 dir2 dir3\n");
        printf("    同时删除多个空目录\n\n");
        printf("注意:\n");
        printf("  • 只能删除空目录\n");
        printf("  • 目录中有文件时会报错\n");
        printf("  • 要删除非空目录，请使用 'xrm -r'\n\n");
        printf("对应系统命令: rmdir\n");
        return 0;
    }

    // 步骤1：检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xrmdir: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xrmdir --help' for more information。\n");
        return -1;
    }

    // 步骤2：删除所有指定的目录
    int has_error = 0;                          // 错误标志

    for (int i = 1; i < cmd->arg_count; i++) {
        const char *dirname = cmd->args[i];
        
        // 尝试删除目录
        if (rmdir(dirname) == -1) {
            XSHELL_LOG_PERROR(ctx, dirname);
            has_error = 1;
        }
    }

    // 步骤3：返回结果
    return has_error ? -1 : 0;
}

