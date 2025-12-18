// 定义特性测试宏（必须在所有头文件之前）
#define _XOPEN_SOURCE 700           // 启用 POSIX.1-2008 和 XSI 扩展

// 引入自定义头文件
#include "builtin.h"                // 内置命令函数声明
#include "utils.h"                  // 工具函数（进度条）

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, perror）
#include <string.h>                 // 字符串处理（strcmp, strlen, strcat）
#include <unistd.h>                 // UNIX 标准函数（access）
#include <sys/stat.h>               // 文件状态（stat, mkdir, S_ISDIR, S_ISREG）
#include <dirent.h>                 // 目录操作（opendir, readdir）
#include <fcntl.h>                  // 文件控制（open, O_RDONLY, O_WRONLY）
#include <errno.h>                  // 错误码（errno）
#include <limits.h>                 // 系统限制（PATH_MAX）

#define BUFFER_SIZE 8192            // 文件复制缓冲区大小（8KB）

// ============================================
// 辅助函数：复制单个文件
// ============================================
// 参数：
//   src: 源文件路径
//   dst: 目标文件路径
// 返回值：
//   0  - 成功
//  -1  - 失败
// ============================================
static int copy_file(const char *src, const char *dst, ShellContext *ctx) {
    int src_fd, dst_fd;                     // 源文件和目标文件描述符
    ssize_t nread;                          // 读取的字节数
    char buffer[BUFFER_SIZE];               // 缓冲区
    struct stat src_stat;                   // 源文件状态

    // 步骤1：打开源文件
    src_fd = open(src, O_RDONLY);           // 以只读模式打开源文件
    if (src_fd == -1) {                     // 打开失败
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src, strerror(errno));
        return -1;
    }

    // 步骤2：获取源文件状态（用于保持权限）
    if (fstat(src_fd, &src_stat) == -1) {   // 获取文件状态失败
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src, strerror(errno));
        close(src_fd);
        return -1;
    }

    // 步骤3：创建目标文件
    dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    if (dst_fd == -1) {                     // 创建失败
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", dst, strerror(errno));
        close(src_fd);
        return -1;
    }


    // 步骤4：复制文件内容
    long long copied = 0;
    while ((nread = read(src_fd, buffer, BUFFER_SIZE)) > 0) {
        if (write(dst_fd, buffer, nread) != nread) {
            XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", dst, strerror(errno));
            close(src_fd);
            close(dst_fd);
            return -1;
        }
        copied += nread;
    }

    // 检查读取是否出错
    if (nread == -1) {                      // 读取失败
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src, strerror(errno));
        close(src_fd);
        close(dst_fd);
        return -1;
    }

    // 步骤5：关闭文件
    close(src_fd);
    close(dst_fd);

    return 0;                               // 返回成功
}

// ============================================
// 辅助函数：递归复制目录
// 参数：
//   src          源目录路径
//   dst          目标目录路径
// ============================================
static int copy_directory_recursive(const char *src, const char *dst, ShellContext *ctx) {
    DIR *dir;
    struct dirent *entry;
    char src_path[PATH_MAX];
    char dst_path[PATH_MAX];
    struct stat statbuf;

    // 步骤1：创建目标目录
    if (mkdir(dst, 0755) == -1 && errno != EEXIST) {
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", dst, strerror(errno));
        return -1;
    }

    // 步骤2：打开源目录
    dir = opendir(src);
    if (dir == NULL) {
        XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src, strerror(errno));
        return -1;
    }

    // 步骤3：遍历源目录
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 "." 和 ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // 构建完整路径
        snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);

        // 获取文件状态
        if (lstat(src_path, &statbuf) == -1) {
            XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src_path, strerror(errno));
            closedir(dir);
            return -1;
        }

        // 根据类型复制
        if (S_ISDIR(statbuf.st_mode)) {     // 如果是目录
            if (copy_directory_recursive(src_path, dst_path, ctx) != 0) {
                closedir(dir);
                return -1;
            }
        } else {                            // 如果是文件
            if (copy_file(src_path, dst_path, ctx) != 0) {
                closedir(dir);
                return -1;
            }
        }
    }

    closedir(dir);
    return 0;
}

// ============================================
// xcp 命令实现函数
// ============================================
int cmd_xcp(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xcp - 复制文件或目录\n\n");
        printf("用法:\n");
        printf("  xcp [选项] <源> <目标> [--help]\n");
        printf("  xcp [选项] <源...> <目录> [--help]\n\n");
        printf("说明:\n");
        printf("  复制文件或目录到指定位置。\n");
        printf("  Copy - 复制文件或目录。\n\n");
        printf("选项:\n");
        printf("  -r, -R        递归复制目录\n");
        printf("  --help        显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xcp file1.txt file2.txt      # 复制文件\n");
        printf("  xcp file1.txt dir/           # 复制文件到目录\n");
        printf("  xcp f1 f2 f3 dir/            # 复制多个文件到目录\n");
        printf("  xcp -r dir1 dir2             # 递归复制目录\n");

        printf("注意:\n");
        printf("  • 复制目录必须使用 -r 选项\n");
        printf("  • 目标存在时会覆盖\n\n");
        printf("对应系统命令: cp\n");
        return 0;
    }

    // 步骤1：参数检查
    if (cmd->arg_count < 3) {
        XSHELL_LOG_ERROR(ctx, "xcp: missing file operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xcp --help' for more information.\n");
        return -1;
    }

    // 步骤2：解析选项
    int recursive = 0;
    int start_index = 1;

    for (int i = 1; i < cmd->arg_count; i++) {
        if (strcmp(cmd->args[i], "-r") == 0 || strcmp(cmd->args[i], "-R") == 0) {
            recursive = 1;
        } else if (strcmp(cmd->args[i], "--help") == 0) {
            // 已处理
        } else {
            start_index = i;
            break;
        }
    }

    // 重新检查参数数量
    if (cmd->arg_count < start_index + 2) {
        XSHELL_LOG_ERROR(ctx, "xcp: missing destination file operand\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xcp --help' for more information.\n");
        return -1;
    }

    // 步骤3：获取目标路径（最后一个参数）
    const char *dst = cmd->args[cmd->arg_count - 1];
    struct stat dst_stat;
    int dst_is_dir = 0;

    // 检查目标是否为目录
    if (stat(dst, &dst_stat) == 0 && S_ISDIR(dst_stat.st_mode)) {
        dst_is_dir = 1;
    }

    // 步骤4：处理复制操作
    int has_error = 0;
    int num_sources = cmd->arg_count - start_index - 1;

    // 情况1：多个源文件，目标必须是目录
    if (num_sources > 1 && !dst_is_dir) {
        XSHELL_LOG_ERROR(ctx, "xcp: target '%s' is not a directory\n", dst);
        return -1;
    }

    // 步骤5：遍历所有源文件
    for (int i = start_index; i < cmd->arg_count - 1; i++) {
        const char *src = cmd->args[i];
        struct stat src_stat;
        char dst_path[PATH_MAX];

        // 获取源文件状态
        if (lstat(src, &src_stat) == -1) {
            XSHELL_LOG_ERROR(ctx, "xcp: %s: %s\n", src, strerror(errno));
            has_error = 1;
            continue;
        }

        // 构建目标路径
        if (dst_is_dir) {
            // 提取源文件名
            const char *basename = strrchr(src, '/');
            basename = (basename == NULL) ? src : basename + 1;
            snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, basename);
        } else {
            snprintf(dst_path, sizeof(dst_path), "%s", dst);
        }

        // 根据类型复制
        if (S_ISDIR(src_stat.st_mode)) {    // 源是目录
            if (!recursive) {
                XSHELL_LOG_ERROR(ctx, "xcp: -r not specified; omitting directory '%s'\n", src);
                has_error = 1;
                continue;
            }
            if (copy_directory_recursive(src, dst_path, ctx) != 0) {
                has_error = 1;
            }
        } else if (S_ISREG(src_stat.st_mode)) {  // 源是文件
            if (copy_file(src, dst_path, ctx) != 0) {
                has_error = 1;
            }
        }
    }

    return has_error ? -1 : 0;
}

