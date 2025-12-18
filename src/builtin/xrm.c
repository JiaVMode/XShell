// 定义特性测试宏（必须在所有头文件之前）
#define _XOPEN_SOURCE 700           // 启用 POSIX.1-2008 和 XSI 扩展（包括 lstat）

// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror）
#include <string.h>                 // 字符串处理（strcmp, strlen）
#include <unistd.h>                 // UNIX 标准函数（unlink, rmdir）
#include <sys/stat.h>               // 文件状态（stat, lstat, S_ISDIR）
#include <dirent.h>                 // 目录操作（opendir, readdir）
#include <errno.h>                  // 错误码（errno, ENOENT）
#include <limits.h>                 // 系统限制（PATH_MAX）

// ============================================
// 辅助函数：递归删除目录
// ============================================
// 参数：
//   path: 目录路径
// 返回值：
//   0  - 成功
//  -1  - 失败
// ============================================
static int remove_directory_recursive(const char *path, ShellContext *ctx) {
    DIR *dir;                                   // 目录指针
    struct dirent *entry;                       // 目录项
    char filepath[PATH_MAX];                    // 完整文件路径缓冲区
    struct stat statbuf;                        // 文件状态信息

    // 步骤1：打开目录
    dir = opendir(path);
    if (dir == NULL) {                          // 打开失败
        XSHELL_LOG_PERROR(ctx, path);
        return -1;
    }

    // 步骤2：遍历目录中的所有项
    while ((entry = readdir(dir)) != NULL) {    // 读取目录项
        // 跳过 "." 和 ".."（当前目录和父目录）
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;                           // 跳过这两个特殊目录
        }

        // 步骤3：构建完整路径
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);

        // 步骤4：获取文件/目录状态
        if (lstat(filepath, &statbuf) == -1) {  // 获取状态失败
            XSHELL_LOG_PERROR(ctx, filepath);
            closedir(dir);                      // 关闭目录
            return -1;                          // 返回失败
        }

        // 步骤5：根据类型递归删除
        if (S_ISDIR(statbuf.st_mode)) {         // 如果是目录
            // 递归删除子目录
            if (remove_directory_recursive(filepath, ctx) != 0) {
                closedir(dir);                  // 关闭目录
                return -1;                      // 返回失败
            }
        } else {                                // 如果是文件
            // 删除文件
            if (unlink(filepath) != 0) {        // unlink() 删除文件
                XSHELL_LOG_PERROR(ctx, filepath);
                closedir(dir);                  // 关闭目录
                return -1;                      // 返回失败
            }
        }
    }

    // 步骤6：关闭目录
    closedir(dir);

    // 步骤7：删除空目录本身
    if (rmdir(path) != 0) {                     // rmdir() 删除空目录
        XSHELL_LOG_PERROR(ctx, path);
        return -1;                              // 返回失败
    }

    return 0;                                   // 返回成功
}

// ============================================
// xrm 命令实现函数
// ============================================
// 命令名称：xrm
// 对应系统命令：rm
// 功能：
//   1. 删除文件
//   2. 删除目录（需要 -r 选项）
//   3. 支持多个文件/目录
// 用法：xrm [选项] <文件/目录...>
//
// 选项：
//   -r, -R   递归删除目录及其内容
//   --help   显示帮助信息
//
// 示例：
//   xrm file.txt              → 删除文件
//   xrm file1 file2           → 删除多个文件
//   xrm -r dir                → 递归删除目录
//   xrm -r dir1 file1 dir2    → 删除多个文件和目录
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xrm(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xrm - 删除文件或目录\n\n");
        printf("用法:\n");
        printf("  xrm [选项] <文件/目录...> [--help]\n\n");
        printf("说明:\n");
        printf("  删除指定的文件或目录。\n");
        printf("  Remove - 移除文件或目录。\n\n");
        printf("参数:\n");
        printf("  文件/目录  要删除的文件或目录名（可以指定多个）\n\n");
        printf("选项:\n");
        printf("  -r, -R    递归删除目录及其内容\n");
        printf("  -f        强制删除，忽略不存在的文件，不提示错误\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xrm file.txt             # 删除文件\n");
        printf("  xrm file1 file2          # 删除多个文件\n");
        printf("  xrm -r dir               # 递归删除目录\n");
        printf("  xrm -f file.txt          # 强制删除文件（忽略不存在）\n");
        printf("  xrm -rf dir              # 强制递归删除目录\n");
        printf("  xrm -r dir1 file1 dir2   # 删除多个文件和目录\n\n");
        printf("注意:\n");
        printf("  • 删除操作不可恢复，请谨慎使用\n");
        printf("  • 删除目录必须使用 -r 选项\n");
        printf("  • 需要对目标文件/目录有写权限\n\n");
        printf("对应系统命令: rm\n");
        return 0;
    }

    // 步骤1：参数检查
    if (cmd->arg_count < 2) {                   // 参数不足
        XSHELL_LOG_ERROR(ctx, "xrm: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xrm --help' for more information.\n");
        return -1;                              // 返回失败
    }

    // 步骤2：解析选项
    int recursive = 0;                          // 是否递归删除（-r 选项）
    int force = 0;                              // 是否强制删除（-f 选项）
    int start_index = 1;                        // 文件参数的起始索引

    // 检查选项（支持多个选项和组合选项）
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *arg = cmd->args[i];
        
        if (strcmp(arg, "-r") == 0 || strcmp(arg, "-R") == 0) {
            recursive = 1;                      // 启用递归删除
        } else if (strcmp(arg, "-f") == 0) {
            force = 1;                          // 启用强制删除
        } else if (strcmp(arg, "--help") == 0) {
            // 帮助信息已在前面处理
        } else if (arg[0] == '-' && strlen(arg) > 1 && arg[1] != '-') {
            // 处理组合选项（如 -rf, -fr, -rf 等）
            for (int j = 1; arg[j] != '\0'; j++) {
                switch (arg[j]) {
                    case 'r':
                    case 'R':
                        recursive = 1;
                        break;
                    case 'f':
                        force = 1;
                        break;
                    default:
                        XSHELL_LOG_ERROR(ctx, "xrm: invalid option: '-%c'\n", arg[j]);
                        XSHELL_LOG_ERROR(ctx, "Try 'xrm --help' for more information.\n");
                        return -1;
                }
            }
        } else {
            // 遇到第一个非选项参数，这是文件参数的起始位置
            start_index = i;
            break;
        }
    }

    // 步骤3：检查是否有文件参数
    if (cmd->arg_count <= start_index) {        // 没有文件参数
        XSHELL_LOG_ERROR(ctx, "xrm: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xrm --help' for more information.\n");
        return -1;                              // 返回失败
    }

    // 步骤4：遍历所有文件参数，逐个删除
    int has_error = 0;                          // 错误标志
    struct stat statbuf;                        // 文件状态信息

    for (int i = start_index; i < cmd->arg_count; i++) {
        const char *path = cmd->args[i];       // 当前要删除的路径

        // 步骤5：获取文件/目录状态
        if (lstat(path, &statbuf) != 0) {       // 获取状态失败
            if (force) {                         // 如果使用 -f 选项，忽略不存在的文件
                continue;                        // 继续处理下一个，不报错
            }
            XSHELL_LOG_PERROR(ctx, path);
            has_error = 1;                      // 标记有错误
            continue;                           // 继续处理下一个
        }

        // 步骤6：根据类型删除
        if (S_ISDIR(statbuf.st_mode)) {         // 如果是目录
            if (!recursive) {                   // 未指定 -r 选项
                XSHELL_LOG_ERROR(ctx, "xrm: cannot remove '%s': Is a directory\n", path);
                XSHELL_LOG_ERROR(ctx, "xrm: use 'xrm -r %s' to remove a directory\n", path);
                has_error = 1;                  // 标记有错误
                continue;                       // 继续处理下一个
            }
            // 递归删除目录
            if (remove_directory_recursive(path, ctx) != 0) {
                has_error = 1;                  // 标记有错误
            }
        } else {                                // 如果是文件
            // 删除文件
            if (unlink(path) != 0) {            // unlink() 删除文件
                if (force && errno == ENOENT) { // 如果使用 -f 选项且文件不存在，忽略错误
                    continue;                   // 继续处理下一个
                }
                XSHELL_LOG_PERROR(ctx, path);
                has_error = 1;                  // 标记有错误
            }
        }
    }

    // 步骤7：返回执行结果
    return has_error ? -1 : 0;                  // 有错误返回 -1，否则返回 0
}

