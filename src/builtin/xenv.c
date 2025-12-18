/*
 * xenv.c - 显示或设置环境变量
 * 
 * 功能：显示所有环境变量
 * 用法：xenv
 * 
 * 注意：设置环境变量使用 xexport 命令
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>

// 外部变量：指向环境变量数组
// environ 是一个全局变量，包含所有环境变量
extern char **environ;

int cmd_xenv(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xenv - 显示所有环境变量\n\n");
        printf("用法:\n");
        printf("  xenv [--help]\n\n");
        printf("说明:\n");
        printf("  显示当前Shell的所有环境变量。\n");
        printf("  Environment - 环境。\n\n");
        printf("输出格式:\n");
        printf("  每行一个环境变量，格式为：NAME=value\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xenv                       # 显示所有环境变量\n");
        printf("  xenv | xgrep PATH          # 查找PATH相关变量\n");
        printf("  xenv | xwc -l              # 统计环境变量数量\n\n");
        printf("常见环境变量:\n");
        printf("  PATH      - 可执行文件搜索路径\n");
        printf("  HOME      - 用户主目录\n");
        printf("  USER      - 当前用户名\n");
        printf("  SHELL     - 当前Shell路径\n");
        printf("  PWD       - 当前工作目录\n");
        printf("  LANG      - 语言设置\n");
        printf("  TERM      - 终端类型\n\n");
        printf("相关命令:\n");
        printf("  xexport   - 设置环境变量\n");
        printf("  xunset    - 删除环境变量\n\n");
        printf("对应系统命令: env, printenv\n");
        return 0;
    }
    
    // 检查是否有额外参数
    if (cmd->arg_count > 1) {
        XSHELL_LOG_ERROR(ctx, "xenv: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xenv --help' for more information.\n");
        return -1;
    }
    
    // 遍历并打印所有环境变量
    if (environ == NULL) {
        XSHELL_LOG_ERROR(ctx, "xenv: no environment available\n");
        return -1;
    }
    
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    
    return 0;
}




