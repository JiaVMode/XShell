// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明
#include "history.h"                // 历史记录模块

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf）
#include <string.h>                 // 字符串处理（strcmp）

// ============================================
// xhistory 命令实现函数
// ============================================
// 命令名称：xhistory
// 对应系统命令：history
// 功能：显示命令历史记录
// 用法：xhistory [--help]
//
// 示例：
//   xhistory              → 显示所有历史命令
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xhistory(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xhistory 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xhistory - 显示命令历史记录\n\n");
        printf("用法:\n");
        printf("  xhistory [--help]\n\n");
        printf("说明:\n");
        printf("  显示已执行的命令历史列表。\n");
        printf("  每条命令前显示序号。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xhistory\n");
        printf("    输出:\n");
        printf("        1  xpwd\n");
        printf("        2  xls\n");
        printf("        3  xcd /tmp\n\n");
        printf("特性:\n");
        printf("  • 历史记录保存在项目目录下的 .xshell_history\n");
        printf("  • 最多保存 1000 条命令\n");
        printf("  • 自动过滤重复的连续命令\n");
        printf("  • 退出时自动保存历史\n\n");
        printf("对应系统命令: history\n");
        return 0;
    }

    // 步骤1：显示历史记录
    history_show();                             // 调用历史记录模块显示函数

    return 0;                                   // 返回成功
}

