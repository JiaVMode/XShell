/*
 * xhelp.c - 显示帮助信息
 * 
 * 功能：显示所有命令的列表或特定命令的帮助
 * 用法：xhelp [command]
 */

#include "builtin.h"
#include "executor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// 显示所有命令列表
static void show_all_commands(void) {
    printf("XShell 内置命令列表\n");
    printf("==================\n\n");
    
    printf("基础命令:\n");
    printf("  xpwd      - 显示当前工作目录\n");
    printf("  xcd       - 切换目录\n");
    printf("  xls       - 列出文件和目录\n");
    printf("  xecho     - 输出字符串\n");
    printf("  quit      - 退出Shell\n\n");
    
    printf("文件操作:\n");
    printf("  xtouch    - 创建文件或更新时间戳\n");
    printf("  xcat      - 显示文件内容\n");
    printf("  xrm       - 删除文件或目录\n");
    printf("  xcp       - 复制文件或目录\n");
    printf("  xmv       - 移动或重命名文件\n\n");
    
    printf("目录操作:\n");
    printf("  xmkdir    - 创建目录\n");
    printf("  xrmdir    - 删除空目录\n\n");
    
    printf("权限与链接:\n");
    printf("  xchmod    - 修改文件权限\n");
    printf("  xchown    - 修改文件所有者\n");
    printf("  xln       - 创建链接\n\n");
    
    printf("查找与其他:\n");
    printf("  xfind     - 查找文件\n");
    printf("  xtec      - Tee功能\n");
    printf("  xhistory  - 命令历史\n\n");
    
    printf("系统信息:\n");
    printf("  xuname    - 系统信息\n");
    printf("  xhostname - 主机名\n");
    printf("  xwhoami   - 当前用户\n");
    printf("  xdate     - 日期时间\n");
    printf("  xuptime   - 运行时间\n");
    printf("  xps       - 进程信息\n\n");
    
    printf("文本处理:\n");
    printf("  xgrep     - 搜索文本\n");
    printf("  xwc       - 统计行数/字数/字节数\n");
    printf("  xhead     - 显示文件前N行\n");
    printf("  xtail     - 显示文件后N行\n");
    printf("  xsort     - 排序文件内容\n");
    printf("  xuniq     - 去除重复行\n\n");
    
    printf("环境变量和别名:\n");
    printf("  xenv      - 显示所有环境变量\n");
    printf("  xexport   - 设置环境变量\n");
    printf("  xunset    - 删除环境变量\n");
    printf("  xalias    - 设置命令别名\n");
    printf("  xunalias  - 删除命令别名\n\n");
    
    printf("实用工具:\n");
    printf("  xclear    - 清屏\n");
    printf("  xhelp     - 显示帮助信息\n");
    printf("  xtype     - 显示命令类型\n");
    printf("  xwhich    - 显示命令路径\n");
    printf("  xsleep    - 休眠指定秒数\n");
    printf("  xcalc     - 简单计算器\n");
    printf("  xtree     - 树形显示目录结构\n\n");
    
    printf("使用 'xhelp <command>' 查看特定命令的详细帮助。\n");
    printf("使用 '<command> --help' 也可以查看命令帮助。\n");
}

int cmd_xhelp(Command *cmd, ShellContext *ctx) {
    // 避免编译器警告
    (void)ctx;
    
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xhelp - 显示帮助信息\n\n");
        printf("用法:\n");
        printf("  xhelp [command]\n\n");
        printf("说明:\n");
        printf("  显示所有命令的列表或特定命令的帮助信息。\n");
        printf("  Help - 帮助。\n\n");
        printf("参数:\n");
        printf("  command   要查看帮助的命令名（可选）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xhelp                      # 显示所有命令列表\n");
        printf("  xhelp xls                  # 显示xls命令的帮助\n");
        printf("  xhelp xgrep                # 显示xgrep命令的帮助\n\n");
        printf("注意:\n");
        printf("  也可以直接使用 '<command> --help' 查看命令帮助。\n");
        return 0;
    }
    
    // 没有参数：显示所有命令列表
    if (cmd->arg_count == 1) {
        show_all_commands();
        return 0;
    }
    
    // 显示特定命令的帮助
    const char *command = cmd->args[1];
    
    // 构造带--help的命令
    Command help_cmd;
    help_cmd.name = (char*)command;
    help_cmd.args[0] = (char*)command;
    help_cmd.args[1] = "--help";
    help_cmd.arg_count = 2;
    
    // 检查是否是内置命令
    if (is_builtin(command)) {
        return execute_builtin(&help_cmd, ctx);
    } else {
        printf("xhelp: %s: command not found\n", command);
        printf("Use 'xhelp' to see all available commands.\n");
        return -1;
    }
}




