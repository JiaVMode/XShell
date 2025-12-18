// 定义特性测试宏（必须在所有头文件之前）
#define _XOPEN_SOURCE 700           // 启用 POSIX.1-2008 和 XSI 扩展

// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror）
#include <string.h>                 // 字符串处理（strcmp, strlen）
#include <unistd.h>                 // UNIX 标准函数
#include <sys/stat.h>               // 文件状态（stat, S_ISDIR）
#include <errno.h>                  // 错误码（errno）
#include <limits.h>                 // 系统限制（PATH_MAX）

// ============================================
// xmv 命令实现函数
// ============================================
// 命令名称：xmv
// 对应系统命令：mv
// 功能：
//   1. 移动文件到另一个位置
//   2. 重命名文件/目录
//   3. 移动多个文件到目录
//   4. 移动目录
// 用法：xmv <源> <目标>
//       xmv <源...> <目录>
//
// 实现原理：
//   使用 rename() 系统调用实现移动/重命名
//   rename() 可以：
//   - 重命名文件/目录（源和目标在同一文件系统）
//   - 移动文件/目录到同一文件系统的不同位置
//
// 示例：
//   xmv old.txt new.txt        → 重命名文件
//   xmv file.txt dir/          → 移动文件到目录
//   xmv file1 file2 dir/       → 移动多个文件到目录
//   xmv olddir newdir          → 重命名目录
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xmv(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xmv 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xmv - 移动或重命名文件/目录\n\n");
        printf("用法:\n");
        printf("  xmv <源> <目标> [--help]\n");
        printf("  xmv <源...> <目录> [--help]\n\n");
        printf("说明:\n");
        printf("  移动文件/目录到新位置，或重命名文件/目录。\n");
        printf("  Move - 移动或重命名文件/目录。\n\n");
        printf("参数:\n");
        printf("  源        要移动的文件或目录（可以指定多个）\n");
        printf("  目标      目标位置（文件名或目录）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xmv old.txt new.txt          # 重命名文件\n");
        printf("  xmv file.txt dir/            # 移动文件到目录\n");
        printf("  xmv file1 file2 dir/         # 移动多个文件到目录\n");
        printf("  xmv olddir newdir            # 重命名目录\n");
        printf("  xmv dir1 dir2/               # 移动目录到目录\n\n");
        printf("注意:\n");
        printf("  • 源和目标必须在同一文件系统\n");
        printf("  • 目标存在时会覆盖（文件）\n");
        printf("  • 移动多个文件时，目标必须是目录\n\n");
        printf("对应系统命令: mv\n");
        return 0;
    }

    // 步骤1：参数检查
    if (cmd->arg_count < 3) {                   // 至少需要源和目标两个参数
        XSHELL_LOG_ERROR(ctx, "xmv: missing file operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xmv --help' for more information.\n");
        return -1;
    }

    // 步骤2：获取目标路径（最后一个参数）
    const char *dst = cmd->args[cmd->arg_count - 1];
    struct stat dst_stat;
    int dst_is_dir = 0;                         // 目标是否为目录

    // 检查目标是否为现有目录
    if (stat(dst, &dst_stat) == 0 && S_ISDIR(dst_stat.st_mode)) {
        dst_is_dir = 1;                         // 目标是目录
    }

    // 步骤3：计算源文件数量
    int num_sources = cmd->arg_count - 2;       // 源文件数量 = 总参数数 - 1（命令名）- 1（目标）

    // 步骤4：多个源文件时，目标必须是目录
    if (num_sources > 1 && !dst_is_dir) {       // 多个源，但目标不是目录
        XSHELL_LOG_ERROR(ctx, "xmv: target '%s' is not a directory\n", dst);
        return -1;
    }

    // 步骤5：遍历所有源文件，逐个移动
    int has_error = 0;                          // 错误标志

    for (int i = 1; i < cmd->arg_count - 1; i++) {
        const char *src = cmd->args[i];         // 当前源文件/目录
        char dst_path[PATH_MAX];                // 目标路径缓冲区

        // 步骤6：检查源文件是否存在
        struct stat src_stat;
        if (stat(src, &src_stat) != 0) {        // 源文件不存在
            XSHELL_LOG_PERROR(ctx, src);
            has_error = 1;                      // 标记有错误
            continue;                           // 继续处理下一个文件
        }

        // 步骤7：构建目标路径
        if (dst_is_dir) {                       // 如果目标是目录
            // 提取源文件的基本文件名（basename）
            const char *basename = strrchr(src, '/');
            basename = (basename == NULL) ? src : basename + 1;
            
            // 构建完整的目标路径：目录/文件名
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, basename);
        } else {                                // 如果目标不是目录（单个文件）
            // 直接使用目标路径
            snprintf(dst_path, sizeof(dst_path), "%s", dst);
        }

        // 步骤8：执行移动/重命名操作
        // rename(oldpath, newpath) 重命名或移动文件/目录
        // 成功返回 0，失败返回 -1
        if (rename(src, dst_path) != 0) {       // 移动失败
            XSHELL_LOG_PERROR(ctx, src);
            has_error = 1;                      // 标记有错误
        }
    }

    // 步骤9：返回执行结果
    return has_error ? -1 : 0;                  // 有错误返回 -1，否则返回 0
}

