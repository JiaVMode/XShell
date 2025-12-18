// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <time.h>               // 时间处理（time, localtime, strftime）

// ============================================
// xdate 命令实现函数
// ============================================
// 命令名称：xdate
// 对应系统命令：date
// 功能：显示当前日期和时间
// 用法：xdate [选项]
//
// 选项：
//   -u        显示UTC时间
//   --help    显示帮助信息
//
// 示例：
//   xdate               # 显示本地时间
//   xdate -u            # 显示UTC时间
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xdate(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xdate - 显示当前日期和时间\n\n");
        printf("用法:\n");
        printf("  xdate [选项]\n\n");
        printf("说明:\n");
        printf("  显示当前的日期和时间。\n");
        printf("  默认显示本地时间。\n\n");
        printf("选项:\n");
        printf("  -u        显示UTC时间（协调世界时）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xdate\n");
        printf("    显示本地时间\n");
        printf("    例如：Thu Oct 30 14:30:25 CST 2025\n\n");
        printf("  xdate -u\n");
        printf("    显示UTC时间\n");
        printf("    例如：Thu Oct 30 06:30:25 UTC 2025\n\n");
        printf("对应系统命令: date\n");
        return 0;
    }

    // 步骤1：检查是否使用UTC时间
    int use_utc = 0;                            // UTC标志
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "-u") == 0) {
        use_utc = 1;
    } else if (cmd->arg_count >= 2) {
        XSHELL_LOG_ERROR(ctx, "xdate: invalid option: '%s'\n", cmd->args[1]);
        XSHELL_LOG_ERROR(ctx, "Try 'xdate --help' for more information.\n");
        return -1;
    }

    // 步骤2：获取当前时间
    time_t now = time(NULL);
    if (now == -1) {
        XSHELL_LOG_PERROR(ctx, "xdate");
        return -1;
    }

    // 步骤3：转换为本地时间或UTC时间
    struct tm *timeinfo;
    if (use_utc) {
        timeinfo = gmtime(&now);                // UTC时间
    } else {
        timeinfo = localtime(&now);             // 本地时间
    }

    if (timeinfo == NULL) {
        XSHELL_LOG_PERROR(ctx, "xdate");
        return -1;
    }

    // 步骤4：格式化并输出时间
    char buffer[256];
    if (use_utc) {
        // UTC时间格式：Thu Oct 30 06:30:25 UTC 2025
        strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S UTC %Y", timeinfo);
    } else {
        // 本地时间格式：Thu Oct 30 14:30:25 CST 2025
        strftime(buffer, sizeof(buffer), "%a %b %d %H:%M:%S %Z %Y", timeinfo);
    }
    printf("%s\n", buffer);

    // 步骤5：返回成功
    return 0;
}

