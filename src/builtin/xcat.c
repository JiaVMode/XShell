// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror, fopen, fclose）
#include <string.h>                 // 字符串处理（strcmp）
#include <errno.h>                  // 错误码（errno）
#include <sys/stat.h>               // 文件状态（stat, S_ISDIR）

// ============================================
// 辅助函数：显示单个文件的内容
// ============================================
// 参数：
//   filename: 文件名（可以是 "-" 表示标准输入）
//   show_line_numbers: 是否显示行号
//   show_all: 是否显示所有不可见字符（-A 选项）
//   show_tabs: 是否显示制表符（-T 选项）
//   line_number: 指向当前行号的指针（用于多文件时连续编号）
//   at_line_start: 指向行首标记的指针
// 返回值：
//   0  - 成功
//  -1  - 失败
// ============================================
static int cat_file(const char *filename, int show_line_numbers, 
                   int show_all, int show_tabs,
                   int *line_number, int *at_line_start,
                   ShellContext *ctx) {
    FILE *file;                                 // 文件指针
    int ch;                                     // 当前读取的字符

    // 步骤1：打开文件
    // 特殊处理："-" 表示标准输入
    if (strcmp(filename, "-") == 0) {           // 文件名是 "-"
        file = stdin;                           // 使用标准输入
    } else {                                    // 普通文件
        // 先检查是否是目录
        struct stat st;
        if (stat(filename, &st) == 0) {         // 获取文件状态成功
            if (S_ISDIR(st.st_mode)) {           // 是目录
                XSHELL_LOG_ERROR(ctx, "xcat: %s: Is a directory\n", filename);
                return -1;                       // 返回失败
            }
        }
        
        file = fopen(filename, "r");            // 以只读模式打开文件
        if (file == NULL) {                     // 打开失败
            XSHELL_LOG_ERROR(ctx, "xcat: %s: %s\n", filename, strerror(errno));
            return -1;                          // 返回失败
        }
    }

    // 步骤2：逐字符读取并输出文件内容
    while ((ch = fgetc(file)) != EOF) {         // 读取一个字符，直到文件结束（EOF）
        // 如果需要显示行号，且当前在行首，则输出行号
        if (show_line_numbers && *at_line_start) {
            printf("%6d  ", *line_number);      // 输出行号（右对齐，6位宽）
            *at_line_start = 0;                 // 标记已经不在行首
        }

        // 处理不可见字符的显示
        if (show_all) {
            // -A 选项：显示所有不可见字符
            if (ch == '\t') {
                printf("^I");                   // 制表符显示为 ^I
            } else if (ch == '\n') {
                printf("$\n");                  // 换行符显示为 $ 后跟换行
                (*line_number)++;               // 行号加1
                *at_line_start = 1;             // 标记下一个字符在行首
            } else if (ch < 32 || ch == 127) {
                // 其他控制字符显示为 ^X
                printf("^%c", ch + 64);
            } else {
                putchar(ch);                    // 普通字符直接输出
            }
        } else if (show_tabs && ch == '\t') {
            // -T 选项：只显示制表符
            printf("^I");                       // 制表符显示为 ^I
        } else {
            // 普通模式：直接输出字符
            putchar(ch);                        // 输出字符到标准输出
            if (ch == '\n') {                   // 遇到换行符
                (*line_number)++;               // 行号加1
                *at_line_start = 1;             // 标记下一个字符在行首
            }
        }
    }

    // 步骤3：关闭文件
    // 注意：不关闭标准输入（stdin）
    if (file != stdin) {                        // 如果不是标准输入
        fclose(file);                           // 关闭文件，释放资源
    }

    return 0;                                   // 返回成功
}

// ============================================
// xcat 命令实现函数
// ============================================
// 命令名称：xcat
// 对应系统命令：cat
// 功能：
//   1. 显示文件内容到标准输出
//   2. 支持多个文件（按顺序连接显示）
//   3. 支持 -n 选项（显示行号）
//   4. 支持 "-" 作为标准输入
// 用法：xcat [选项] <文件1> [文件2 ...]
//
// 选项：
//   -n      显示行号
//   --help  显示帮助信息
//
// 示例：
//   xcat file.txt              → 显示 file.txt 内容
//   xcat file1.txt file2.txt   → 连接显示两个文件
//   xcat -n file.txt           → 显示内容并加行号
//   xcat -                     → 从标准输入读取
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xcat(Command *cmd, ShellContext *ctx) {
    // ctx 现在被使用，用于日志记录

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xcat - 连接文件并打印到标准输出\n\n");
        printf("用法:\n");
        printf("  xcat [选项] [文件...] [--help]\n\n");
        printf("说明:\n");
        printf("  连接文件内容并打印到标准输出。\n");
        printf("  若没有指定文件，或文件名为 -，则从标准输入读取。\n\n");
        printf("参数:\n");
        printf("  文件      要显示的文件名（可以指定多个）\n");
        printf("  -         表示标准输入\n\n");
        printf("选项:\n");
        printf("  -n        显示行号\n");
        printf("  -A        显示所有不可见字符（制表符显示为 ^I，行尾显示为 $）\n");
        printf("  -T        显示制表符为 ^I\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xcat file.txt              # 显示文件内容\n");
        printf("  xcat file1 file2           # 连接显示多个文件\n");
        printf("  xcat -n file.txt           # 显示内容并加行号\n");
        printf("  xcat -A file.txt           # 显示所有不可见字符\n");
        printf("  xcat -T file.txt            # 显示制表符为 ^I\n");
        printf("  xcat -                     # 从标准输入读取\n");
        printf("  xecho \"Hello\" | xcat -     # 管道使用（未来支持）\n\n");
        printf("行为说明:\n");
        printf("  • 多个文件会按顺序连接显示\n");
        printf("  • -n 选项会在每行前显示行号\n");
        printf("  • 无参数时从标准输入读取\n\n");
        printf("对应系统命令: cat\n");
        return 0;
    }

    // 步骤1：解析选项
    int show_line_numbers = 0;                  // 是否显示行号（-n 选项）
    int show_all = 0;                           // 是否显示所有不可见字符（-A 选项）
    int show_tabs = 0;                          // 是否显示制表符（-T 选项）
    int start_index = 1;                        // 文件参数的起始索引

    // 检查选项（支持多个选项）
    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "-n") == 0) {
            show_line_numbers = 1;              // 启用行号显示
        } else if (strcmp(cmd->args[i], "-A") == 0) {
            show_all = 1;                       // 启用显示所有不可见字符
        } else if (strcmp(cmd->args[i], "-T") == 0) {
            show_tabs = 1;                      // 启用显示制表符
        } else if (strcmp(cmd->args[i], "--help") == 0) {
            // 帮助信息已在前面处理
        } else {
            // 遇到第一个非选项参数，这是文件参数的起始位置
            start_index = i;
            break;
        }
    }

    // 步骤2：初始化行号和行首标记（用于多文件连续编号）
    int line_number = 1;                        // 当前行号（从 1 开始）
    int at_line_start = 1;                      // 是否在行首

    // 步骤3：检查是否有文件参数
    if (cmd->arg_count <= start_index) {        // 没有文件参数
        // 从标准输入读取
        // 注意：在重定向情况下，这会从重定向的输入读取
        return cat_file("-", show_line_numbers, show_all, show_tabs, &line_number, &at_line_start, ctx);
    }

    // 步骤4：遍历所有文件参数，逐个显示
    int has_error = 0;                          // 错误标志

    for (int i = start_index; i < cmd->arg_count; i++) {
        const char *filename = cmd->args[i];    // 当前文件名

        // 显示文件内容（传递行号指针，使多文件行号连续）
        if (cat_file(filename, show_line_numbers, show_all, show_tabs, &line_number, &at_line_start, ctx) != 0) {
            has_error = 1;                      // 标记有错误
            // 继续处理下一个文件（与系统 cat 行为一致）
        }
    }

    // 步骤5：返回执行结果
    return has_error ? -1 : 0;                  // 有错误返回 -1，否则返回 0
}

