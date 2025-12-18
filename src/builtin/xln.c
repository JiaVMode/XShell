// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（link, symlink）

// ============================================
// xln 命令实现函数
// ============================================
// 命令名称：xln
// 对应系统命令：ln
// 功能：创建链接（硬链接或符号链接）
// 用法：xln [选项] <源文件> <目标>
//
// 选项：
//   -s        创建符号链接（软链接）
//   --help    显示帮助信息
//
// 示例：
//   xln file.txt link.txt          # 创建硬链接
//   xln -s file.txt symlink.txt    # 创建符号链接
//   xln -s /path/to/file link      # 创建指向绝对路径的符号链接
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xln(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xln 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xln - 创建链接（硬链接或符号链接）\n\n");
        printf("用法:\n");
        printf("  xln [选项] <源文件> <目标>\n\n");
        printf("说明:\n");
        printf("  创建指向源文件的链接。\n");
        printf("  默认创建硬链接，使用 -s 选项创建符号链接。\n\n");
        printf("参数:\n");
        printf("  源文件    链接指向的文件\n");
        printf("  目标      链接的名称\n\n");
        printf("选项:\n");
        printf("  -s        创建符号链接（软链接）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xln file.txt hardlink.txt\n");
        printf("    创建硬链接\n\n");
        printf("  xln -s file.txt symlink.txt\n");
        printf("    创建符号链接\n\n");
        printf("  xln -s /path/to/file link\n");
        printf("    创建指向绝对路径的符号链接\n\n");
        printf("  xln -s ../data.txt current_data\n");
        printf("    创建相对路径的符号链接\n\n");
        printf("硬链接 vs 符号链接:\n");
        printf("  硬链接:\n");
        printf("    • 直接指向文件数据\n");
        printf("    • 不能跨文件系统\n");
        printf("    • 不能链接目录\n");
        printf("    • 源文件删除后仍可访问\n\n");
        printf("  符号链接:\n");
        printf("    • 指向文件路径\n");
        printf("    • 可以跨文件系统\n");
        printf("    • 可以链接目录\n");
        printf("    • 源文件删除后链接失效\n\n");
        printf("对应系统命令: ln\n");
        return 0;
    }

    // 步骤1：检查参数数量
    if (cmd->arg_count < 3) {
        XSHELL_LOG_ERROR(ctx, "xln: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xln --help' for more information.\n");
        return -1;
    }

    // 步骤2：解析选项
    int symbolic = 0;                           // 符号链接标志
    int src_index = 1;                          // 源文件参数索引
    int dst_index = 2;                          // 目标参数索引

    // 检查是否有 -s 选项
    if (strcmp(cmd->args[1], "-s") == 0) {
        symbolic = 1;                           // 启用符号链接模式
        src_index = 2;                          // 源文件在第3个参数
        dst_index = 3;                          // 目标在第4个参数
        
        // 检查参数是否足够
        if (cmd->arg_count < 4) {
            XSHELL_LOG_ERROR(ctx, "xln: missing destination file operand after '%s'\n", cmd->args[2]);
            XSHELL_LOG_ERROR(ctx, "Try 'xln --help' for more information.\n");
            return -1;
        }
    }

    // 步骤3：获取源文件和目标路径
    const char *src = cmd->args[src_index];
    const char *dst = cmd->args[dst_index];

    // 步骤4：创建链接
    int result;
    if (symbolic) {
        // 创建符号链接
        result = symlink(src, dst);
        if (result == -1) {
            XSHELL_LOG_PERROR(ctx, "xln");
            return -1;
        }
    } else {
        // 创建硬链接
        result = link(src, dst);
        if (result == -1) {
            XSHELL_LOG_PERROR(ctx, "xln");
            return -1;
        }
    }

    // 步骤5：返回成功
    return 0;
}

