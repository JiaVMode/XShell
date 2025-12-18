/*
 * xstat.c - 显示文件详细信息
 * 
 * 功能：显示文件的详细统计信息（大小、权限、时间等）
 * 用法：xstat [选项] <文件>...
 * 
 * 选项：
 *   -c <格式>    指定输出格式（简化实现，支持基本格式）
 *   --help       显示帮助信息
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sysmacros.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件>...\n", cmd_name);
    printf("功能: 显示文件的详细统计信息\n");
    printf("选项:\n");
    printf("  -c <格式>      指定输出格式（如 %%s=大小, %%n=文件名）\n");
    printf("  -h, --help    显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s file.txt\n", cmd_name);
    printf("  %s -c \"%%s\" file.txt    # 只显示文件大小\n", cmd_name);
}

// 格式化文件权限
static void format_permissions(mode_t mode, char *str) {
    str[0] = (mode & S_IRUSR) ? 'r' : '-';
    str[1] = (mode & S_IWUSR) ? 'w' : '-';
    str[2] = (mode & S_IXUSR) ? 'x' : '-';
    str[3] = (mode & S_IRGRP) ? 'r' : '-';
    str[4] = (mode & S_IWGRP) ? 'w' : '-';
    str[5] = (mode & S_IXGRP) ? 'x' : '-';
    str[6] = (mode & S_IROTH) ? 'r' : '-';
    str[7] = (mode & S_IWOTH) ? 'w' : '-';
    str[8] = (mode & S_IXOTH) ? 'x' : '-';
    str[9] = '\0';
}

// 获取文件类型字符
static char get_file_type(mode_t mode) {
    if (S_ISREG(mode)) return '-';
    if (S_ISDIR(mode)) return 'd';
    if (S_ISLNK(mode)) return 'l';
    if (S_ISCHR(mode)) return 'c';
    if (S_ISBLK(mode)) return 'b';
    if (S_ISFIFO(mode)) return 'p';
    if (S_ISSOCK(mode)) return 's';
    return '?';
}

// 格式化时间
static void format_time(time_t t, char *str, size_t size) {
    struct tm *tm_info = localtime(&t);
    strftime(str, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

// 处理格式化字符串
static void print_formatted(const char *format, const char *filename, struct stat *st) {
    const char *p = format;
    while (*p != '\0') {
        if (*p == '%' && *(p + 1) != '\0') {
            p++;
            switch (*p) {
                case 'n':  // 文件名
                    printf("%s", filename);
                    break;
                case 's':  // 文件大小
                    printf("%ld", (long)st->st_size);
                    break;
                case 'a':  // 访问权限（八进制）
                    printf("%o", st->st_mode & 0777);
                    break;
                case 'A':  // 访问权限（字符串）
                    {
                        char perm[10];
                        format_permissions(st->st_mode, perm);
                        printf("%s", perm);
                    }
                    break;
                case 'U':  // 用户ID
                    printf("%d", (int)st->st_uid);
                    break;
                case 'G':  // 组ID
                    printf("%d", (int)st->st_gid);
                    break;
                case 'i':  // inode号
                    printf("%lu", (unsigned long)st->st_ino);
                    break;
                case 'h':  // 硬链接数
                    printf("%lu", (unsigned long)st->st_nlink);
                    break;
                case 't':  // 文件类型
                    printf("%c", get_file_type(st->st_mode));
                    break;
                case '%':  // 转义的%
                    putchar('%');
                    break;
                default:
                    putchar('%');
                    putchar(*p);
                    break;
            }
            p++;
        } else {
            putchar(*p);
            p++;
        }
    }
}

// 显示文件统计信息
static int show_file_stat(const char *filename, const char *format, ShellContext *ctx) {
    struct stat st;
    
    if (stat(filename, &st) != 0) {
        XSHELL_LOG_ERROR(ctx, "xstat: %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    if (format != NULL) {
        // 使用自定义格式
        print_formatted(format, filename, &st);
        putchar('\n');
    } else {
        // 默认格式：显示详细信息
        char perm[10];
        char mtime[64], atime[64], ctime[64];
        struct passwd *pwd = getpwuid(st.st_uid);
        struct group *grp = getgrgid(st.st_gid);
        
        format_permissions(st.st_mode, perm);
        format_time(st.st_mtime, mtime, sizeof(mtime));
        format_time(st.st_atime, atime, sizeof(atime));
        format_time(st.st_ctime, ctime, sizeof(ctime));
        
        printf("  文件: %s\n", filename);
        printf("  大小: %ld\t\t", (long)st.st_size);
        printf("块: %ld\t\t", (long)st.st_blocks);
        printf("IO块: %ld\t", (long)st.st_blksize);
        printf("设备: %lu/%lu\t", (unsigned long)major(st.st_dev), (unsigned long)minor(st.st_dev));
        printf("Inode: %lu\t", (unsigned long)st.st_ino);
        printf("硬链接: %lu\n", (unsigned long)st.st_nlink);
        printf("权限: (%o/%s)  Uid: (%d/%s)  Gid: (%d/%s)\n",
               st.st_mode & 0777, perm,
               (int)st.st_uid, pwd ? pwd->pw_name : "?",
               (int)st.st_gid, grp ? grp->gr_name : "?");
        printf("最近访问: %s\n", atime);
        printf("最近修改: %s\n", mtime);
        printf("最近更改: %s\n", ctime);
    }
    
    return 0;
}

// xstat 命令实现
int cmd_xstat(Command *cmd, ShellContext *ctx) {
    const char *format = NULL;
    int has_files = 0;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-c") == 0) {
            if (i + 1 >= cmd->arg_count) {
                XSHELL_LOG_ERROR(ctx, "xstat: 错误: -c 选项需要参数\n");
                return -1;
            }
            format = cmd->args[i + 1];
            i += 2;
        } else {
            break;
        }
    }
    
    // 处理文件
    for (; i < cmd->arg_count; i++) {
        show_file_stat(cmd->args[i], format, ctx);
        has_files = 1;
    }
    
    if (!has_files) {
        XSHELL_LOG_ERROR(ctx, "xstat: 错误: 需要指定文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    return 0;
}

