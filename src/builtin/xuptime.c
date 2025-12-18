// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf, fopen, fgets）
#include <string.h>             // 字符串处理（strcmp, sscanf）
#include <time.h>               // 时间处理（time, localtime, strftime）

// ============================================
// xuptime 命令实现函数
// ============================================
// 命令名称：xuptime
// 对应系统命令：uptime
// 功能：显示系统运行时间
// 用法：xuptime
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xuptime             # 显示系统运行时间
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xuptime(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xuptime 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xuptime - 显示系统运行时间\n\n");
        printf("用法:\n");
        printf("  xuptime\n\n");
        printf("说明:\n");
        printf("  显示系统已运行的时间。\n");
        printf("  包括当前时间、系统运行时长等信息。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xuptime\n");
        printf("    显示系统运行时间\n");
        printf("    例如：14:30:25 up 2:15, 1 user\n\n");
        printf("对应系统命令: uptime\n");
        return 0;
    }

    // 步骤1：获取当前时间
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    if (timeinfo == NULL) {
        XSHELL_LOG_PERROR(ctx, "xuptime");
        return -1;
    }

    // 步骤2：输出当前时间
    printf("%02d:%02d:%02d up ", 
           timeinfo->tm_hour, 
           timeinfo->tm_min, 
           timeinfo->tm_sec);

    // 步骤3：从 /proc/uptime 读取系统运行时间
    FILE *fp = fopen("/proc/uptime", "r");
    if (fp == NULL) {
        // 如果读取失败，只显示基本信息
        printf("(uptime info unavailable)\n");
        return 0;
    }

    // 读取运行时间（秒）
    double uptime_seconds;
    if (fscanf(fp, "%lf", &uptime_seconds) != 1) {
        fclose(fp);
        printf("(uptime info unavailable)\n");
        return 0;
    }
    fclose(fp);

    // 步骤4：转换运行时间为天、小时、分钟
    int days = (int)(uptime_seconds / 86400);
    int hours = (int)((uptime_seconds - days * 86400) / 3600);
    int minutes = (int)((uptime_seconds - days * 86400 - hours * 3600) / 60);

    // 步骤5：格式化输出运行时间
    if (days > 0) {
        printf("%d day%s, %d:%02d", days, (days > 1) ? "s" : "", hours, minutes);
    } else if (hours > 0) {
        printf("%d:%02d", hours, minutes);
    } else {
        printf("%d min", minutes);
    }

    // 步骤6：尝试读取用户数
    fp = fopen("/var/run/utmp", "r");
    if (fp != NULL) {
        fclose(fp);
        // 简化实现，显示固定的用户数信息
        printf(", 1 user");
    }

    printf("\n");

    // 步骤7：返回成功
    return 0;
}

