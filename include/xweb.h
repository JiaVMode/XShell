/**
 * @file xweb.h
 * @brief XShell 终端网页浏览器头文件
 * 
 * 在终端中浏览网页，支持使用 curl 或 wget 获取网页内容
 */

#ifndef XWEB_H
#define XWEB_H

#include "xshell.h"
#include "parser.h"
#include <stdbool.h>

// ==================== 主要函数 ====================

/**
 * @brief 获取并显示网页内容
 * @param url 网页URL
 * @return 0 成功, -1 失败
 */
int xweb_fetch(const char *url);

/**
 * @brief 交互式网页浏览器
 * @param ctx Shell 上下文
 * @return 0 成功
 */
int xweb_browser(ShellContext *ctx);

/**
 * @brief xweb 内置命令入口
 * @param cmd 命令结构
 * @param ctx Shell 上下文
 * @return 退出状态码
 */
int cmd_xweb(Command *cmd, ShellContext *ctx);

#endif // XWEB_H
