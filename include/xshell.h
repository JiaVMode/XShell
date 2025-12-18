// 头文件保护
#ifndef XSHELL_H                    // 如果XSHELL_H 未定义
#define XSHELL_H                    // 则定义XSHELL_H 

// 标准输入输出库
#include <stdio.h>                  // 提供 printf, fprintf, fgets 等函数
#include <stdlib.h>                 // 提供 malloc, free, calloc 等内存管理函数
#include <string.h>                 // 提供 strcmp, strcpy, strlen 等字符串函数
#include <unistd.h>                 // 提供 getcwd, chdir, fork, exec 等系统调用
#include <linux/limits.h>           // 提供 PATH_MAX 常量（最大路径长度）
#include <errno.h>                  // 提供 errno

// 常量定义
#define MAX_INPUT_LENGTH 4096       // 用户输入命令的最大长度（字节）
#define MAX_TOKEN 256               // 命令行最多可分割的token数量
#define MAX_ARGS 128                // 单个命令最多可接收的参数数量

// 数据结构定义
// Shell 上下文结构体：保存 Shell 运行时的全局状态
typedef struct {
    char cwd[PATH_MAX];             // 当前工作目录的完整路径
    char prev_dir[PATH_MAX];        // 上一个工作目录（用于 xcd，返回）
    char *home_dir;                 // 用户主目录（用于xcd无参数时返回）
    int running;                    // Shell 运行标志：1 = 运行中，0 = 退出
    int last_exit_status;           // 上一条命令的退出状态码（0表示成功）
    FILE *log_file;                 // 日志文件指针（用于记录错误信息）
} ShellContext;

// 函数声明
// Shell 核心函数

// 初始化 Shell 环境（获取当前目录、设置初始状态等）
int init_shell(ShellContext *ctx);

// Shell 主循环（读取命令、解析、执行，循环往复）
void shell_loop(ShellContext *ctx);

// 显示命令提示符（如:[\home\user\]#)
void display_prompt(ShellContext *ctx);

// 执行单条命令行（解析并执行用户输入的命令）
int execute_command_line(const char *line, ShellContext *ctx);

// 清理Shell 资源（退出前的清理工作）
void cleanup_shell(ShellContext *ctx);

// 日志记录函数
// 记录错误信息到日志文件和标准错误流
void log_error(ShellContext *ctx, const char *format, ...);

// 统一的错误输出宏：
// - 同时向 stderr 输出错误信息
// - 并写入 XShell 的错误日志文件
// 使用说明：
//   XSHELL_LOG_PERROR(ctx, label);
//   XSHELL_LOG_ERROR(ctx, "xrm: cannot remove '%s'", path);
//
// 日志输出格式参考 xshell_implementation_plan.md：
//   [时间戳] PID=进程ID CMD="命令" errno=错误码: 错误描述
#define XSHELL_LOG_PERROR(ctx, label)                                      \
    do {                                                                   \
        int _saved_errno = errno;                                          \
        fprintf(stderr, "%s: %s\n", (label), strerror(_saved_errno));      \
        if ((ctx) != NULL) {                                               \
            log_error((ctx), "CMD=\"%s\" errno=%d: %s",                    \
                      (label), _saved_errno, strerror(_saved_errno));      \
        }                                                                  \
    } while (0)

#define XSHELL_LOG_ERROR(ctx, fmt, ...)                                    \
    do {                                                                   \
        fprintf(stderr, fmt, ##__VA_ARGS__);                               \
        if ((ctx) != NULL) {                                               \
            log_error((ctx), fmt, ##__VA_ARGS__);                          \
        }                                                                  \
    } while (0)

#endif                              // 头文件保护结束