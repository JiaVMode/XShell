// 头文件保护：防止重复包含
#ifndef UTILS_H             // 如果 UTILS_H 未定义
#define UTILS_H             // 则定义 UTILS_H

// 引入标准库
#include <ctype.h>          // 字符类型判断函数（isspace 等）

// 函数声明
// 工具函数：提供字符串处理等辅助功能

// 去除字符串首尾的空白字符（空格、制表符、换行符等）
// 功能：修改原字符串，将开头的空白跳过，将末尾的空白替换为 \0
// 参数：str - 要处理的字符串（会被修改）
// 返回：指向处理后字符串开头的指针（可能与原指针不同）
// 注意：返回的指针指向原字符串内部，不需要单独释放
char* trim(char *str);

// 检查字符串是否为空行（NULL、空字符串或只包含空白字符）
// 功能：判断一行输入是否没有实际内容
// 参数：line - 要检查的字符串
// 返回：1=是空行，0=不是空行（包含非空白字符）
// 用途：Shell 主循环中跳过空行输入
int is_empty_line(const char *line);

// 规范化路径：将所有反斜杠 \ 转换为正斜杠 /
// 参数：path - 要处理的路径字符串（会被直接修改）
// 返回：指向修改后字符串的指针（与输入相同）
// 用途：让用户可以使用反斜杠输入路径，系统自动转换
// 注意：此函数会直接修改原字符串
char* normalize_path(char *path);

// ===== 彩色输出支持 =====

// ANSI 颜色代码
#define COLOR_RESET   "\033[0m"
#define COLOR_BLACK   "\033[30m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_MAGENTA "\033[35m"
#define COLOR_CYAN    "\033[36m"
#define COLOR_WHITE   "\033[37m"
#define COLOR_BOLD    "\033[1m"

// 检查终端是否支持颜色
// 返回：1=支持，0=不支持
int is_color_supported(void);

// 设置文本颜色
// 参数：color_name - 颜色名称（red, green, blue, yellow, cyan, magenta, black, white）
// 返回：颜色代码字符串
const char* set_color(const char *color_name);

// 重置颜色
// 返回：重置代码字符串
const char* reset_color(void);


#endif                      // UTILS_H  // 头文件保护结束
