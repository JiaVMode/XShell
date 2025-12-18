// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror）
#include <string.h>                 // 字符串处理（strcmp, strerror）
#include <fcntl.h>                  // 文件控制（open, O_CREAT）
#include <unistd.h>                 // UNIX 标准函数（close）
#include <sys/stat.h>               // 文件状态（S_IRUSR, S_IWUSR）
#include <sys/types.h>              // 系统数据类型
#include <utime.h>                  // 文件时间操作（utime）
#include <errno.h>                  // 错误码（errno, ENOENT）

// ============================================
// xtouch 命令实现函数
// ============================================
// 命令名称：xtouch
// 对应系统命令：touch
// 功能：
//   1. 若文件不存在，创建空文件
//   2. 若文件已存在，更新访问时间和修改时间为当前时间
// 用法：xtouch <文件名> [文件名2] ...
//
// 实现原理：
//   - 先尝试用 utime(filename, NULL) 更新时间戳
//   - 如果文件不存在（errno == ENOENT），则用 open() 创建文件
//
// 示例：
//   xtouch test.txt          → 创建 test.txt 或更新其时间戳
//   xtouch file1 file2       → 同时处理多个文件
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xtouch(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtouch - 创建文件或更新时间戳\n\n");
        printf("用法:\n");
        printf("  xtouch <文件名> [文件名2 ...] [--help]\n\n");
        printf("说明:\n");
        printf("  更新文件的访问时间和修改时间为当前时间。\n");
        printf("  若文件不存在，则创建一个空文件。\n\n");
        printf("参数:\n");
        printf("  文件名    要创建或更新的文件名（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xtouch test.txt          # 创建或更新 test.txt\n");
        printf("  xtouch file1 file2       # 同时处理多个文件\n");
        printf("  xtouch /tmp/note.md      # 使用绝对路径\n\n");
        printf("行为说明:\n");
        printf("  • 文件不存在：创建空文件\n");
        printf("  • 文件已存在：更新访问时间和修改时间为当前时间\n");
        printf("  • 权限：新文件权限为 0644 (rw-r--r--)\n\n");
        printf("对应系统命令: touch\n");
        return 0;
    }

    // 步骤1：参数检查
    if (cmd->arg_count < 2) {                       // 参数不足（没有指定文件名）
        XSHELL_LOG_ERROR(ctx, "xtouch: missing file operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xtouch --help' for more information.\n");
        return -1;                                  // 返回失败状态
    }

    // 步骤2：遍历所有文件参数，逐个处理
    int has_error = 0;                              // 错误标志
    
    for (int i = 1; i < cmd->arg_count; i++) {      // 从第 1 个参数开始
        const char *filename = cmd->args[i];        // 当前文件名
        
        // 步骤3：尝试更新文件时间戳
        // utime(filename, NULL) 将文件的访问时间和修改时间设置为当前时间
        // 如果文件存在，返回 0；如果文件不存在，返回 -1 且 errno == ENOENT
        if (utime(filename, NULL) == 0) {           // 成功更新时间戳
            // 文件已存在且时间戳已更新，继续处理下一个文件
            continue;
        }
        
        // 步骤4：检查错误原因
        if (errno != ENOENT) {                      // 如果不是"文件不存在"错误
            XSHELL_LOG_ERROR(ctx, "xtouch: %s: %s\n", filename, strerror(errno));
            has_error = 1;                          // 标记有错误
            continue;                               // 处理下一个文件
        }
        
        // 步骤5：文件不存在，创建新文件
        // open() 参数说明：
        //   - O_CREAT：文件不存在时创建
        //   - O_WRONLY：以只写模式打开
        //   - 0644：文件权限（rw-r--r--，所有者可读写，其他人只读）
        int fd = open(filename, O_CREAT | O_WRONLY, 0644);
        
        if (fd == -1) {                             // 创建失败
            XSHELL_LOG_ERROR(ctx, "xtouch: %s: %s\n", filename, strerror(errno));
            has_error = 1;                          // 标记有错误
        } else {                                    // 创建成功
            close(fd);                              // 关闭文件描述符
        }
    }
    
    // 步骤6：返回执行结果
    return has_error ? -1 : 0;                      // 有错误返回 -1，否则返回 0
}

