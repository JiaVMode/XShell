// 定义 POSIX 标准版本，启用strdup等函数
#define _POSIX_C_SOURCE 200809L

// 引入自定义头文件
#include "utils.h"                              // 工具函数声明

// 引入标准库函数
#include <stdio.h>                               // 标准输入输出（printf, fflush, stdout）
#include <string.h>                             // 字符串处理函数（strlen, strdup）
#include <ctype.h>                              // 字符类型判断函数（isspace）
#include <stdlib.h>                             // 标准库（getenv, malloc, free）
#include <unistd.h>                             // POSIX 函数（isatty, getcwd, getlogin）
#include <strings.h>                            // 字符串函数（strcasecmp）
#include <time.h>                                // 时间函数（time, localtime, strftime）
#include <sys/ioctl.h>                           // 终端控制（ioctl, TIOCGWINSZ）
#include <sys/time.h>                            // 时间函数（gettimeofday）

// 字符串处理函数
// 功能：去除字符串首尾的空白字符（空白、制表符、换行符等）
// 实现原理：
//      1. 从开头跳过所有空白字符（移动指针）
//      2. 从末尾向前找到最后一个非空白字符
//      3. 在该字符后面添加 \0 阶段字符串
char* trim(char *str) {
    // 步骤1：参数检查：防止空指针访问
    if (str == NULL){                           // 如果传入空指针
        return NULL;                            // 直接返回NULL    
    }

    // 步骤2：跳过开头的空白字符
    // isspace() 判断字符是否为空白（空格、\t、\n、\r、\f、\v）
    // (unsigned char) 转换是为了避免负数字符的未定义行为
    while (isspace((unsigned char)*str)){       // 当前字符是空白
        str++;                                  // 指针向后移动（跳过空白字符）
    }

    // 步骤3：检查是否整个字符串都是空白
    if (*str == '\0') {                         // 如果到达字符串结尾（全是空白）
        return str;                             // 直接返回（指向 \0）    
    }

    // 步骤4：找到末尾的非空白字符
    char *end = str + strlen(str) - 1;          // end 指向字符串最后一个字符
    // 从后向前遍历，跳过末尾的空白字符
    while(end > str && isspace((unsigned char)*end)) {
        end--;                                  // 指针向前移动
    }

    // 步骤5：在末尾非空白字符后添加字符串结束符
    *(end + 1) = '\0';                          // 在非空白字符后面截断字符串

    // 步骤6：返回处理后的字符串指针
    return str;                                 // 返回指向第一个非空白字符的指针
}

// 空行检查函数
// 功能：检查字符串是否为空行（NULL、空字符串或只包含空白字符）
// 用途：Shell 主循环中用于跳过用户输入的空行
// 例如："   \t  \n" -> 返回 1（是空行）
//      "hello" -> 返回 0（不是空行）
int is_empty_line(const char *line) {
    // 步骤1：参数检查：NULL 指针是为空行
    if(line == NULL) {                          // 如果传入空指针
        return 1;                               // 返回 1 表示是空行（安全处理）    
    }

    // 步骤2：遍历字符串的每个字符，检查是否都是空白
    // *line 等价于line[0]，循环知道遇到字符串结束符 \0
    while(*line) {                              // 当前字符不是 \0（字符串未结束）
        // 检查当前字符是否为非空白字符
        if (!isspace((unsigned char)*line)) {   // 不是空白字符
            return 0;                           // 返回 0 表示不是空行（发现了有效内容）
        }
        line++;                                 // 指针后移，检查下一个字符
    }

    // 循环结束说明遍历了整个字符串，所有字符都是空白（或字符串为空）

    // 步骤3：返回结果：所有字符都是空白
    return 1;                                   // 返回 1 表示是空行
}

// 路径规范化函数
// 功能：将路径中的所有反斜杠 \ 转换为正斜杠 /
// 实现原理：遍历字符串，将所有 \ 替换为 /
char* normalize_path(char *path) {
    // 步骤1：参数检查：防止空指针访问
    if (path == NULL) {                         // 如果传入空指针
        return NULL;                            // 直接返回 NULL
    }

    // 步骤2：遍历字符串，将所有反斜杠替换为正斜杠
    // 使用 for 循环遍历到字符串结束符 \0
    for (int i = 0; path[i] != '\0'; i++) {     // 循环直到遇到字符串结尾
        if (path[i] == '\\') {                  // 如果当前字符是反斜杠（backslash）
            path[i] = '/';                      // 替换为正斜杠（forward slash）
        }
    }

    // 步骤3：返回修改后的字符串指针
    return path;                                // 返回指向原字符串的指针（已被修改）
}

// ===== 彩色输出支持 =====

// 检查终端是否支持颜色
int is_color_supported(void) {
    // 检查 TERM 环境变量
    const char *term = getenv("TERM");
    if (term == NULL) {
        return 0;
    }
    
    // 检查是否是终端
    if (!isatty(STDOUT_FILENO)) {
        return 0;
    }
    
    // 检查 TERM 是否支持颜色（常见值：xterm, xterm-256color, screen, linux等）
    if (strstr(term, "xterm") != NULL ||
        strstr(term, "screen") != NULL ||
        strstr(term, "linux") != NULL ||
        strstr(term, "vt100") != NULL ||
        strstr(term, "color") != NULL) {
        return 1;
    }
    
    return 0;
}

// 设置文本颜色
const char* set_color(const char *color_name) {
    if (color_name == NULL || !is_color_supported()) {
        return "";  // 不支持颜色时返回空字符串
    }
    
    // 转换为小写进行比较
    if (strcasecmp(color_name, "black") == 0) {
        return COLOR_BLACK;
    } else if (strcasecmp(color_name, "red") == 0) {
        return COLOR_RED;
    } else if (strcasecmp(color_name, "green") == 0) {
        return COLOR_GREEN;
    } else if (strcasecmp(color_name, "yellow") == 0) {
        return COLOR_YELLOW;
    } else if (strcasecmp(color_name, "blue") == 0) {
        return COLOR_BLUE;
    } else if (strcasecmp(color_name, "magenta") == 0) {
        return COLOR_MAGENTA;
    } else if (strcasecmp(color_name, "cyan") == 0) {
        return COLOR_CYAN;
    } else if (strcasecmp(color_name, "white") == 0) {
        return COLOR_WHITE;
    } else if (strcasecmp(color_name, "bold") == 0) {
        return COLOR_BOLD;
    }
    
    return "";  // 未知颜色
}

// 重置颜色
const char* reset_color(void) {
    if (is_color_supported()) {
        return COLOR_RESET;
    }
    return "";
}

