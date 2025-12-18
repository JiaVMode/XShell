// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp, strrchr, strlen）
#include <sys/stat.h>           // 文件状态（mkdir, stat）
#include <sys/types.h>          // 系统类型定义
#include <errno.h>              // 错误码（errno, EEXIST）
#include <limits.h>             // 系统限制（PATH_MAX）

// ============================================
// 递归创建目录的辅助函数
// ============================================
// 功能：创建多级目录（mkdir -p 的功能）
// 参数：
//   path: 要创建的目录路径
//   mode: 目录权限
// 返回值：0-成功，-1-失败
static int mkdir_recursive(const char *path, mode_t mode, ShellContext *ctx) {
    char tmp[PATH_MAX];
    char *p = NULL;
    size_t len;
    
    // 步骤1：复制路径到临时缓冲区
    snprintf(tmp, sizeof(tmp), "%s", path);
    len = strlen(tmp);
    
    // 步骤2：移除末尾的斜杠
    if (tmp[len - 1] == '/') {
        tmp[len - 1] = 0;
    }
    
    // 步骤3：逐级创建目录
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            // 暂时截断路径
            *p = 0;
            
            // 尝试创建当前级别的目录
            if (mkdir(tmp, mode) == -1) {
                if (errno != EEXIST) {
                    XSHELL_LOG_PERROR(ctx, tmp);
                    return -1;
                }
            }
            
            // 恢复斜杠
            *p = '/';
        }
    }
    
    // 步骤4：创建最后一级目录
    if (mkdir(tmp, mode) == -1) {
        if (errno != EEXIST) {
            XSHELL_LOG_PERROR(ctx, tmp);
            return -1;
        }
    }
    
    return 0;
}

// ============================================
// xmkdir 命令实现函数
// ============================================
// 命令名称：xmkdir
// 对应系统命令：mkdir
// 功能：创建目录
// 用法：xmkdir [选项] <目录名> [目录名2 ...]
//
// 选项：
//   -p        创建多级目录（父目录不存在时自动创建）
//   --help    显示帮助信息
//
// 示例：
//   xmkdir test                # 创建单个目录
//   xmkdir dir1 dir2 dir3      # 创建多个目录
//   xmkdir -p path/to/dir      # 创建多级目录
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xmkdir(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xmkdir - 创建目录\n\n");
        printf("用法:\n");
        printf("  xmkdir [选项] <目录名> [目录名2 ...]\n\n");
        printf("说明:\n");
        printf("  创建一个或多个目录。\n");
        printf("  默认情况下，父目录必须已存在。\n\n");
        printf("参数:\n");
        printf("  目录名    要创建的目录（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  -p        创建多级目录（父目录不存在时自动创建）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xmkdir test\n");
        printf("    创建 test 目录\n\n");
        printf("  xmkdir dir1 dir2 dir3\n");
        printf("    同时创建多个目录\n\n");
        printf("  xmkdir -p path/to/dir\n");
        printf("    创建多级目录（自动创建 path 和 to）\n\n");
        printf("  xmkdir -p project/src project/include\n");
        printf("    创建项目目录结构\n\n");
        printf("权限:\n");
        printf("  新目录权限为 0755 (rwxr-xr-x)\n\n");
        printf("对应系统命令: mkdir\n");
        return 0;
    }

    // 步骤1：检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xmkdir: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xmkdir --help' for more information.\n");
        return -1;
    }

    // 步骤2：解析选项
    int parent_mode = 0;                        // -p 选项标志
    int start_index = 1;                        // 目录名参数起始索引

    // 检查是否有 -p 选项
    if (strcmp(cmd->args[1], "-p") == 0) {
        parent_mode = 1;                        // 启用多级创建模式
        start_index = 2;                        // 目录名从第3个参数开始
    }

    // 检查是否有目录参数
    if (cmd->arg_count <= start_index) {
        XSHELL_LOG_ERROR(ctx, "xmkdir: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xmkdir --help' for more information.\n");
        return -1;
    }

    // 步骤3：创建所有指定的目录
    int has_error = 0;                          // 错误标志
    mode_t mode = 0755;                         // 目录权限（rwxr-xr-x）

    for (int i = start_index; i < cmd->arg_count; i++) {
        const char *dirname = cmd->args[i];
        
        if (parent_mode) {
            if (mkdir_recursive(dirname, mode, ctx) != 0) {
                has_error = 1;
            }
        } else {
            if (mkdir(dirname, mode) == -1) {
                XSHELL_LOG_PERROR(ctx, dirname);
                has_error = 1;
            }
        }
    }

    // 步骤4：返回结果
    return has_error ? -1 : 0;
}

