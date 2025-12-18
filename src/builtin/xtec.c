// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fopen, fclose, fgetc, putchar）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（STDIN_FILENO）

// ============================================
// xtec 命令实现函数
// ============================================
// 命令名称：xtec
// 对应系统命令：tee
// 功能：从标准输入读取数据，同时输出到标准输出和文件
// 用法：xtec [选项] <文件名> [文件名2 ...]
//
// 选项：
//   -a        追加模式（默认为覆盖）
//   --help    显示帮助信息
//
// 示例：
//   xecho "hello" | xtec output.txt           # 输出到屏幕和文件
//   xls | xtec -a log.txt                     # 追加到文件
//   xcat file.txt | xtec out1.txt out2.txt   # 输出到多个文件
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xtec(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtec - 从标准输入读取并同时输出到文件和标准输出\n\n");
        printf("用法:\n");
        printf("  xtec [选项] <文件名> [文件名2 ...]\n");
        printf("  命令 | xtec <文件名>\n\n");
        printf("说明:\n");
        printf("  从标准输入读取数据，同时写入到：\n");
        printf("  • 标准输出（屏幕）\n");
        printf("  • 指定的文件\n");
        printf("  常用于保存管道中间结果。\n\n");
        printf("参数:\n");
        printf("  文件名    要写入的文件（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  -a        追加模式（追加到文件末尾，而不是覆盖）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xecho \"hello\" | xtec output.txt\n");
        printf("    输出到屏幕：hello\n");
        printf("    写入文件：output.txt\n\n");
        printf("  xls | xtec -a log.txt\n");
        printf("    列出文件并追加到 log.txt\n\n");
        printf("  xcat data.txt | xtec copy1.txt copy2.txt\n");
        printf("    同时写入多个文件\n\n");
        printf("  xpwd | xtec -a history.log\n");
        printf("    追加当前目录到历史日志\n\n");
        printf("特性:\n");
        printf("  • 支持多个输出文件\n");
        printf("  • 支持追加模式（-a）\n");
        printf("  • 从标准输入读取（通常配合管道使用）\n");
        printf("  • 同时输出到屏幕和文件\n\n");
        printf("注意:\n");
        printf("  • 目前 XShell 还不支持管道（|），此命令暂时无法使用\n");
        printf("  • 需要先实现管道功能才能使用 xtec\n\n");
        printf("对应系统命令: tee\n");
        return 0;
    }

    // 步骤1：解析选项
    int append_mode = 0;                        // 追加模式标志（0=覆盖，1=追加）
    int start_index = 1;                        // 文件名参数起始索引

    // 检查是否有 -a 选项
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "-a") == 0) {
        append_mode = 1;                        // 启用追加模式
        start_index = 2;                        // 文件名从第3个参数开始
    }

    // 步骤2：检查是否有文件参数
    if (cmd->arg_count <= start_index) {
        XSHELL_LOG_ERROR(ctx, "xtec: missing file operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xtec --help' for more information.\n");
        return -1;
    }

    // 步骤3：打开所有输出文件
    int file_count = cmd->arg_count - start_index;  // 文件数量
    FILE **files = NULL;                        // 文件指针数组

    if (file_count > 0) {
        // 分配文件指针数组
        files = (FILE **)malloc(sizeof(FILE *) * file_count);
        if (files == NULL) {
            XSHELL_LOG_ERROR(ctx, "xtec: memory allocation failed\n");
            return -1;
        }

        // 打开所有文件
        const char *mode = append_mode ? "a" : "w";  // 根据选项确定打开模式
        for (int i = 0; i < file_count; i++) {
            const char *filename = cmd->args[start_index + i];
            files[i] = fopen(filename, mode);
            if (files[i] == NULL) {
                XSHELL_LOG_PERROR(ctx, filename);
                for (int j = 0; j < i; j++) {
                    fclose(files[j]);
                }
                free(files);
                return -1;
            }
        }
    }

    // 步骤4：从标准输入读取并输出到所有目标
    // 说明：逐字符读取，同时写入到标准输出和所有文件
    int ch;                                     // 当前读取的字符
    int has_error = 0;                          // 错误标志

    while ((ch = getchar()) != EOF) {
        // 4.1：输出到标准输出（屏幕）
        if (putchar(ch) == EOF) {
            XSHELL_LOG_ERROR(ctx, "xtec: write error to stdout\n");
            has_error = 1;
            break;
        }

        // 4.2：输出到所有文件
        for (int i = 0; i < file_count; i++) {
            if (fputc(ch, files[i]) == EOF) {
                XSHELL_LOG_ERROR(ctx, "xtec: write error to %s\n", cmd->args[start_index + i]);
                has_error = 1;
                break;
            }
        }

        if (has_error) {
            break;
        }
    }

    // 步骤5：关闭所有文件
    if (files != NULL) {
        for (int i = 0; i < file_count; i++) {
            if (files[i] != NULL) {
                fclose(files[i]);
            }
        }
        free(files);
    }

    // 步骤6：返回结果
    return has_error ? -1 : 0;
}

