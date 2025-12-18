// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp, strlen）
#include <stdlib.h>             // 标准库（strtol）
#include <sys/stat.h>           // 文件状态（chmod）
#include <sys/types.h>          // 系统类型定义
#include <unistd.h>             // POSIX 标准（access）

// ============================================
// 解析符号权限字符串的辅助函数
// ============================================
// 功能：解析符号模式如 +x, u+w, go-r 等
// 参数：
//   mode_str: 权限字符串
//   current_mode: 当前文件权限
//   new_mode: 输出的新权限值
// 返回值：0-成功，-1-失败
static int parse_symbolic_mode(const char *mode_str, mode_t current_mode, mode_t *new_mode) {
    *new_mode = current_mode;
    
    const char *p = mode_str;
    
    while (*p != '\0') {
        // 解析用户类别 (ugoa)
        mode_t who_mask = 0;
        int has_who = 0;
        
        while (*p == 'u' || *p == 'g' || *p == 'o' || *p == 'a') {
            has_who = 1;
            switch (*p) {
                case 'u': who_mask |= S_IRWXU; break;  // 用户权限
                case 'g': who_mask |= S_IRWXG; break;  // 组权限
                case 'o': who_mask |= S_IRWXO; break;  // 其他权限
                case 'a': who_mask |= S_IRWXU | S_IRWXG | S_IRWXO; break; // 所有
            }
            p++;
        }
        
        // 如果没有指定用户类别，默认为 a (所有)
        if (!has_who) {
            who_mask = S_IRWXU | S_IRWXG | S_IRWXO;
        }
        
        // 解析操作符 (+-=)
        if (*p != '+' && *p != '-' && *p != '=') {
            return -1;  // 无效的操作符
        }
        char op = *p++;
        
        // 解析权限 (rwx)
        mode_t perm_mask = 0;
        while (*p == 'r' || *p == 'w' || *p == 'x') {
            switch (*p) {
                case 'r':
                    if (who_mask & S_IRWXU) perm_mask |= S_IRUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IRGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IROTH;
                    break;
                case 'w':
                    if (who_mask & S_IRWXU) perm_mask |= S_IWUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IWGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IWOTH;
                    break;
                case 'x':
                    if (who_mask & S_IRWXU) perm_mask |= S_IXUSR;
                    if (who_mask & S_IRWXG) perm_mask |= S_IXGRP;
                    if (who_mask & S_IRWXO) perm_mask |= S_IXOTH;
                    break;
            }
            p++;
        }
        
        // 应用操作
        switch (op) {
            case '+':
                *new_mode |= perm_mask;
                break;
            case '-':
                *new_mode &= ~perm_mask;
                break;
            case '=':
                *new_mode = (*new_mode & ~who_mask) | perm_mask;
                break;
        }
        
        // 跳过逗号
        if (*p == ',') {
            p++;
        }
    }
    
    return 0;
}

// ============================================
// 解析八进制权限字符串的辅助函数
// ============================================
// 功能：将 "755" 或 "0755" 等字符串转换为 mode_t
// 参数：
//   mode_str: 权限字符串
//   mode: 输出的权限值
// 返回值：0-成功，-1-失败
static int parse_octal_mode(const char *mode_str, mode_t *mode) {
    char *endptr;
    long val;
    
    // 使用 strtol 解析八进制数
    val = strtol(mode_str, &endptr, 8);  // 基数为 8（八进制）
    
    // 检查是否解析成功
    if (*endptr != '\0' || val < 0 || val > 07777) {
        return -1;
    }
    
    *mode = (mode_t)val;
    return 0;
}

// ============================================
// 统一的权限解析函数
// ============================================
// 功能：自动检测并解析八进制或符号模式
// 参数：
//   mode_str: 权限字符串
//   filename: 文件名（符号模式需要获取当前权限）
//   mode: 输出的权限值
// 返回值：0-成功，-1-失败
static int parse_mode(const char *mode_str, const char *filename, mode_t *mode) {
    // 检测是否为八进制模式（全为数字）
    int is_octal = 1;
    for (const char *p = mode_str; *p != '\0'; p++) {
        if (*p < '0' || *p > '7') {
            is_octal = 0;
            break;
        }
    }
    
    if (is_octal) {
        // 八进制模式
        return parse_octal_mode(mode_str, mode);
    } else {
        // 符号模式，需要获取当前文件权限
        struct stat file_stat;
        if (stat(filename, &file_stat) == -1) {
            // 如果文件不存在，使用默认权限 644
            mode_t default_mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
            return parse_symbolic_mode(mode_str, default_mode, mode);
        } else {
            return parse_symbolic_mode(mode_str, file_stat.st_mode, mode);
        }
    }
}

// ============================================
// xchmod 命令实现函数
// ============================================
// 命令名称：xchmod
// 对应系统命令：chmod
// 功能：修改文件权限
// 用法：xchmod <权限> <文件名> [文件名2 ...]
//
// 选项：
//   --help    显示帮助信息
//
// 示例：
//   xchmod 755 script.sh       # 设置为 rwxr-xr-x
//   xchmod 644 file.txt        # 设置为 rw-r--r--
//   xchmod 600 secret.txt      # 设置为 rw-------
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xchmod(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                                  // ctx 在 xchmod 命令中未使用

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xchmod - 修改文件权限\n\n");
        printf("用法:\n");
        printf("  xchmod <权限> <文件名> [文件名2 ...]\n\n");
        printf("说明:\n");
        printf("  修改文件或目录的访问权限。\n");
        printf("  支持八进制模式（例如 755, 644）和符号模式（例如 +x, u+w）。\n\n");
        printf("参数:\n");
        printf("  权限      八进制权限值（例如 755, 644）或符号模式（例如 +x, u-w）\n");
        printf("  文件名    要修改权限的文件（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xchmod 755 script.sh\n");
        printf("    设置为可执行脚本（rwxr-xr-x）\n\n");
        printf("  xchmod 644 document.txt\n");
        printf("    设置为普通文件（rw-r--r--）\n\n");
        printf("  xchmod 600 secret.txt\n");
        printf("    设置为私有文件（rw-------）\n\n");
        printf("  xchmod 777 shared\n");
        printf("    设置为完全开放（rwxrwxrwx）\n\n");
        printf("  xchmod +x script.sh\n");
        printf("    添加执行权限\n\n");
        printf("  xchmod u+w,g-r file.txt\n");
        printf("    用户添加写权限，组移除读权限\n\n");
        printf("  xchmod a=rw document.txt\n");
        printf("    所有用户设置为读写权限\n\n");
        printf("权限说明:\n");
        printf("  权限由三组数字组成：所有者 组 其他\n");
        printf("  每个数字是以下值的和：\n");
        printf("    4 = 读（r）\n");
        printf("    2 = 写（w）\n");
        printf("    1 = 执行（x）\n\n");
        printf("  常用权限:\n");
        printf("    755 = rwxr-xr-x  可执行文件\n");
        printf("    644 = rw-r--r--  普通文件\n");
        printf("    600 = rw-------  私有文件\n");
        printf("    777 = rwxrwxrwx  完全开放\n");
        printf("    700 = rwx------  私有可执行\n\n");
        printf("  符号模式:\n");
        printf("    u/g/o/a = 用户/组/其他/所有\n");
        printf("    +/-/=   = 添加/移除/设置权限\n");
        printf("    r/w/x   = 读/写/执行权限\n");
        printf("    示例: u+x, go-w, a=r, +x\n\n");
        printf("对应系统命令: chmod\n");
        return 0;
    }

    // 步骤1：检查参数数量
    if (cmd->arg_count < 3) {
        XSHELL_LOG_ERROR(ctx, "xchmod: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xchmod --help' for more information.\n");
        return -1;
    }

    // 步骤2：修改所有指定文件的权限
    int has_error = 0;                          // 错误标志

    for (int i = 2; i < cmd->arg_count; i++) {
        const char *filename = cmd->args[i];
        mode_t mode;
        
        // 解析权限（需要文件名用于符号模式）
        if (parse_mode(cmd->args[1], filename, &mode) != 0) {
            XSHELL_LOG_ERROR(ctx, "xchmod: invalid mode: '%s'\n", cmd->args[1]);
            XSHELL_LOG_ERROR(ctx, "Mode should be octal number (e.g., 755) or symbolic (e.g., +x)\n");
            has_error = 1;
            continue;
        }
        
        // 尝试修改权限
        if (chmod(filename, mode) == -1) {
            XSHELL_LOG_PERROR(ctx, filename);
            has_error = 1;
        }
    }

    // 步骤4：返回结果
    return has_error ? -1 : 0;
}

