// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp）
#include <unistd.h>             // UNIX 标准函数（getuid）
#include <pwd.h>                // 密码数据库（getpwuid）

// ============================================
// xwhoami 命令实现函数
// ============================================
// 命令名称：xwhoami
// 对应系统命令：whoami
// 功能：显示当前用户名
// 用法：xwhoami
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xwhoami             # 显示当前用户名
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xwhoami(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xwhoami - 显示当前用户名\n\n");
        printf("用法:\n");
        printf("  xwhoami\n\n");
        printf("说明:\n");
        printf("  显示当前登录的用户名。\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xwhoami\n");
        printf("    显示当前用户名（例如：lab）\n\n");
        printf("对应系统命令: whoami\n");
        return 0;
    }

    // 步骤1：获取当前用户ID
    uid_t uid = getuid();

    // 步骤2：通过用户ID获取用户信息
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        XSHELL_LOG_PERROR(ctx, "xwhoami");
        return -1;
    }

    // 步骤3：输出用户名
    printf("%s\n", pw->pw_name);

    // 步骤4：返回成功
    return 0;
}

