/*
 * xsource.c - 执行脚本文件
 * 
 * 功能：读取脚本文件并逐行执行其中的命令
 * 用法：xsource <file>
 */

#include "builtin.h"
#include "parser.h"
#include "executor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

int cmd_xsource(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xsource - 执行脚本文件\n\n");
        printf("用法:\n");
        printf("  xsource <file>\n\n");
        printf("说明:\n");
        printf("  读取脚本文件并逐行执行其中的命令。\n");
        printf("  Source - 源，执行。\n\n");
        printf("参数:\n");
        printf("  file      要执行的脚本文件路径\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xsource script.sh              # 执行脚本文件\n");
        printf("  xsource .xshellrc              # 执行配置文件\n\n");
        printf("注意:\n");
        printf("  • 脚本文件必须是文本文件\n");
        printf("  • 每行一个命令\n");
        printf("  • 空行和以#开头的行会被忽略\n");
        printf("  • 如果命令执行失败，会继续执行下一行\n");
        printf("  • 脚本中的quit命令会退出Shell\n\n");
        printf("对应系统命令: source, .\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xsource: missing file argument\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xsource --help' for more information.\n");
        return -1;
    }
    
    if (cmd->arg_count > 2) {
        XSHELL_LOG_ERROR(ctx, "xsource: too many arguments\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xsource --help' for more information.\n");
        return -1;
    }
    
    const char *filename = cmd->args[1];
    
    // 打开文件
    FILE *file = fopen(filename, "r");
    if (!file) {
        XSHELL_LOG_PERROR(ctx, "xsource");
        return -1;
    }
    
    // 读取并执行每一行
    char line[1024];
    int line_num = 0;
    int error_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // 去除换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }
        
        // 去除前导和尾随空格
        char *start = line;
        while (*start == ' ' || *start == '\t') {
            start++;
        }
        
        // 跳过空行和注释行
        if (*start == '\0' || *start == '#') {
            continue;
        }
        
        // 解析命令
        Command *script_cmd = parse_command(start);
        if (script_cmd == NULL) {
            XSHELL_LOG_ERROR(ctx, "xsource: %s:%d: parse error\n", filename, line_num);
            error_count++;
            continue;
        }
        
        // 执行命令
        if (execute_command(script_cmd, ctx) != 0) {
            XSHELL_LOG_ERROR(ctx, "xsource: %s:%d: command failed\n", filename, line_num);
            error_count++;
        }
        
        // 释放命令对象
        free_command(script_cmd);
        
        // 如果Shell已退出，停止执行
        if (!ctx->running) {
            break;
        }
    }
    
    fclose(file);
    
    if (error_count > 0) {
        return -1;
    }
    
    return 0;
}

