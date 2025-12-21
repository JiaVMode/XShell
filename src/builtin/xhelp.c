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
    printf("╔══════════════════════════════════════════════════════════════════════╗\n");
    printf("║                      XShell 内置命令列表                              ║\n");
    printf("╚══════════════════════════════════════════════════════════════════════╝\n\n");
    
    printf("\033[1;36m【基础命令】\033[0m\n");
    printf("  xpwd      - 显示当前工作目录\n");
    printf("  xcd       - 切换目录\n");
    printf("  xls       - 列出文件和目录（支持 -l, -a, -h）\n");
    printf("  xecho     - 输出字符串（支持 -n, -e 转义）\n");
    printf("  xclear    - 清屏\n");
    printf("  quit      - 退出 Shell\n\n");
    
    printf("\033[1;36m【文件操作】\033[0m\n");
    printf("  xtouch    - 创建文件或更新时间戳\n");
    printf("  xcat      - 显示文件内容（支持 -n 行号）\n");
    printf("  xrm       - 删除文件或目录（支持 -r 递归）\n");
    printf("  xcp       - 复制文件或目录（支持 -r 递归）\n");
    printf("  xmv       - 移动或重命名文件\n");
    printf("  xstat     - 显示文件详细信息\n");
    printf("  xfile     - 显示文件类型\n");
    printf("  xreadlink - 读取符号链接目标\n");
    printf("  xrealpath - 显示绝对路径\n");
    printf("  xbasename - 提取文件名\n");
    printf("  xdirname  - 提取目录名\n\n");
    
    printf("\033[1;36m【目录操作】\033[0m\n");
    printf("  xmkdir    - 创建目录（支持 -p 递归）\n");
    printf("  xrmdir    - 删除空目录\n");
    printf("  xtree     - 树形显示目录结构（支持 -L 深度）\n");
    printf("  xfind     - 查找文件（支持 -name 模式）\n");
    printf("  xdu       - 显示目录大小\n");
    printf("  xdf       - 显示磁盘空间\n\n");
    
    printf("\033[1;36m【权限与链接】\033[0m\n");
    printf("  xchmod    - 修改文件权限（支持八进制和符号模式）\n");
    printf("  xchown    - 修改文件所有者\n");
    printf("  xln       - 创建链接（支持 -s 符号链接）\n\n");
    
    printf("\033[1;36m【文本处理】\033[0m\n");
    printf("  xgrep     - 搜索文本（支持 -i, -n, -v, -c, -w）\n");
    printf("  xwc       - 统计行数/字数/字节数（-l, -w, -c）\n");
    printf("  xhead     - 显示文件前N行（-n N）\n");
    printf("  xtail     - 显示文件后N行（-n N）\n");
    printf("  xsort     - 排序文件内容（-r, -n, -u）\n");
    printf("  xuniq     - 去除重复行（-c, -d, -u）\n");
    printf("  xdiff     - 比较文件差异（支持 -u 统一格式）\n");
    printf("  xcut      - 提取列（-f 字段, -d 分隔符）\n");
    printf("  xpaste    - 合并文件行\n");
    printf("  xtr       - 字符转换\n");
    printf("  xcomm     - 比较排序文件\n");
    printf("  xsplit    - 分割文件\n");
    printf("  xjoin     - 连接文件\n\n");
    
    printf("\033[1;36m【系统信息】\033[0m\n");
    printf("  xuname    - 系统信息（-a, -s, -r, -m）\n");
    printf("  xhostname - 主机名\n");
    printf("  xwhoami   - 当前用户\n");
    printf("  xdate     - 日期时间（支持 -u UTC）\n");
    printf("  xuptime   - 系统运行时间\n");
    printf("  xps       - 进程信息\n\n");
    
    printf("\033[1;36m【环境变量和别名】\033[0m\n");
    printf("  xenv      - 显示所有环境变量\n");
    printf("  xexport   - 设置环境变量\n");
    printf("  xunset    - 删除环境变量\n");
    printf("  xalias    - 设置命令别名\n");
    printf("  xunalias  - 删除命令别名\n\n");
    
    printf("\033[1;36m【进程与作业控制】\033[0m\n");
    printf("  xkill     - 终止进程（支持信号名）\n");
    printf("  xjobs     - 显示后台任务\n");
    printf("  xfg       - 将后台任务调到前台\n");
    printf("  xbg       - 将任务放到后台继续执行\n\n");
    
    printf("\033[1;36m【实用工具】\033[0m\n");
    printf("  xhelp     - 显示帮助信息\n");
    printf("  xtype     - 显示命令类型\n");
    printf("  xwhich    - 显示命令路径\n");
    printf("  xsleep    - 休眠指定秒数\n");
    printf("  xcalc     - 简单计算器\n");
    printf("  xtime     - 测量命令执行时间\n");
    printf("  xsource   - 执行脚本文件\n");
    printf("  xtec      - Tee 功能（输出到文件和屏幕）\n");
    printf("  xhistory  - 命令历史记录\n\n");
    
    printf("\033[1;36m【特色功能】\033[0m\n");
    printf("  xui       - 交互式终端 UI 界面\n");
    printf("  xmenu     - 交互式菜单系统\n");
    printf("  xweb      - 网页浏览器（搜索引擎）\n");
    printf("  xsysmon   - 系统监控（CPU/内存/磁盘）\n");
    printf("  xsnake    - 贪吃蛇游戏\n");
    printf("  xtetris   - 俄罗斯方块游戏\n");
    printf("  x2048     - 2048 游戏\n\n");
    
    printf("──────────────────────────────────────────────────────────────────────────\n");
    printf("使用 '\033[1mxhelp <command>\033[0m' 查看特定命令的详细帮助。\n");
    printf("使用 '\033[1m<command> --help\033[0m' 也可以查看命令帮助。\n");
    printf("使用 '\033[1m命令 &\033[0m' 在后台执行命令。\n");
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




