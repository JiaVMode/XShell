// 头文件保护：防止重复包含
#ifndef EXECUTOR_H                          // 如果EXECUTOR_H 未定义
#define EXECUTOR_H                          // 则定义EXECUTOR_H

// 依赖的头文件
#include "parser.h"                         // Command 结构体定义（需要用于函数参数）
#include "xshell.h"                         // ShellContext结构体定义（需要用于函数参数）

// 函数声明
// 命令执行器：负责分发和执行各类命令

// 执行命令（主入口函数）
// 功能：判断命令类型（内置/外部），并调用相应的执行函数
// 参数：cmd - 解析后的命令对象，ctx - Shell 上下文
// 返回：命令退出状态（0=成功，非0=失败）
int execute_command(Command *cmd, ShellContext *ctx);

// 判断是否为内置命令
// 功能：检查命令名是否在内置命令列表中
// 参数：cmd_name - 命令名称字符串（如"xpwd", "quit")
// 返回：1=是内置命令，0=不是内置命令
int is_builtin(const char *cmd_name);

// 执行内置命令
//功能：根据命令名称调用的内置命令处理函数
// 参数：cmd - 命令对象，ctx - Shell 上下文
// 返回：命令退出状态（0=成功，非0=失败）
int execute_builtin(Command *cmd, ShellContext *ctx);

#endif