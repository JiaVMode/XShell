/*
 * xchown.c - 修改文件所有者和组
 * 
 * 功能：修改文件或目录的所有者和组
 * 用法：xchown [选项] <用户[:组]> <文件...>
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <pwd.h>
#include <grp.h>
#include <sys/stat.h>
#include <errno.h>

// 帮助信息
static void show_help(void) {
    printf("xchown - 修改文件所有者和组\n\n");
    printf("用法:\n");
    printf("  xchown [选项] <用户[:组]> <文件>...\n\n");
    printf("说明:\n");
    printf("  修改文件或目录的所有者和/或所属组。\n");
    printf("  Change Owner - 修改文件所有者。\n\n");
    printf("参数:\n");
    printf("  用户[:组]   新的所有者用户名，可选指定组名\n");
    printf("              格式：user、user:group、:group\n");
    printf("  文件        要修改的文件或目录\n\n");
    printf("选项:\n");
    printf("  -R          递归修改目录及其内容\n");
    printf("  -h          修改符号链接本身（而非其指向的文件）\n");
    printf("  --help      显示此帮助信息\n\n");
    printf("示例:\n");
    printf("  xchown user file.txt          # 修改 file.txt 的所有者为 user\n");
    printf("  xchown user:group file.txt    # 同时修改所有者和组\n");
    printf("  xchown :group file.txt        # 只修改组\n");
    printf("  xchown -R user dir/           # 递归修改目录\n\n");
    printf("注意：此操作通常需要 root 权限。\n\n");
    printf("对应系统命令: chown\n");
}

// 解析用户名和组名
// 格式：user、user:group、:group
static int parse_owner_group(const char *spec, uid_t *uid, gid_t *gid) {
    char *copy = strdup(spec);
    if (copy == NULL) {
        perror("strdup");
        return -1;
    }
    
    char *user_part = NULL;
    char *group_part = NULL;
    char *colon = strchr(copy, ':');
    
    if (colon != NULL) {
        *colon = '\0';
        user_part = copy;
        group_part = colon + 1;
    } else {
        user_part = copy;
    }
    
    *uid = (uid_t)-1;  // -1 表示不修改
    *gid = (gid_t)-1;
    
    // 解析用户名
    if (user_part != NULL && strlen(user_part) > 0) {
        // 尝试作为用户名解析
        struct passwd *pwd = getpwnam(user_part);
        if (pwd != NULL) {
            *uid = pwd->pw_uid;
        } else {
            // 尝试作为数字 UID 解析
            char *endptr;
            long val = strtol(user_part, &endptr, 10);
            if (*endptr == '\0' && val >= 0) {
                *uid = (uid_t)val;
            } else {
                fprintf(stderr, "xchown: invalid user: '%s'\n", user_part);
                free(copy);
                return -1;
            }
        }
    }
    
    // 解析组名
    if (group_part != NULL && strlen(group_part) > 0) {
        // 尝试作为组名解析
        struct group *grp = getgrnam(group_part);
        if (grp != NULL) {
            *gid = grp->gr_gid;
        } else {
            // 尝试作为数字 GID 解析
            char *endptr;
            long val = strtol(group_part, &endptr, 10);
            if (*endptr == '\0' && val >= 0) {
                *gid = (gid_t)val;
            } else {
                fprintf(stderr, "xchown: invalid group: '%s'\n", group_part);
                free(copy);
                return -1;
            }
        }
    }
    
    free(copy);
    return 0;
}

// 修改单个文件的所有者
static int change_owner(const char *path, uid_t uid, gid_t gid, int use_lchown) {
    int result;
    
    if (use_lchown) {
        result = lchown(path, uid, gid);
    } else {
        result = chown(path, uid, gid);
    }
    
    if (result != 0) {
        fprintf(stderr, "xchown: cannot change ownership of '%s': %s\n", 
                path, strerror(errno));
        return -1;
    }
    
    return 0;
}

// xchown 命令入口
int cmd_xchown(Command *cmd, ShellContext *ctx) {
    // 避免编译器警告
    (void)ctx;
    
    // 检查帮助选项
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        show_help();
        return 0;
    }
    
    // 解析选项
    int recursive = 0;      // -R 递归
    int use_lchown = 0;     // -h 修改符号链接本身
    int arg_start = 1;      // 第一个非选项参数的索引
    
    while (arg_start < cmd->arg_count && cmd->args[arg_start][0] == '-') {
        char *opt = cmd->args[arg_start];
        
        if (strcmp(opt, "-R") == 0) {
            recursive = 1;
        } else if (strcmp(opt, "-h") == 0) {
            use_lchown = 1;
        } else if (strcmp(opt, "--") == 0) {
            arg_start++;
            break;
        } else {
            fprintf(stderr, "xchown: invalid option: %s\n", opt);
            return 1;
        }
        
        arg_start++;
    }
    
    // 检查参数数量
    if (cmd->arg_count - arg_start < 2) {
        fprintf(stderr, "xchown: missing operand\n");
        fprintf(stderr, "Usage: xchown [选项] <用户[:组]> <文件>...\n");
        fprintf(stderr, "Try 'xchown --help' for more information.\n");
        return 1;
    }
    
    // 解析用户和组
    const char *owner_spec = cmd->args[arg_start];
    uid_t uid;
    gid_t gid;
    
    if (parse_owner_group(owner_spec, &uid, &gid) != 0) {
        return 1;
    }
    
    // 处理每个文件
    int ret = 0;
    for (int i = arg_start + 1; i < cmd->arg_count; i++) {
        const char *path = cmd->args[i];
        
        // 检查文件是否存在
        struct stat st;
        if (lstat(path, &st) != 0) {
            fprintf(stderr, "xchown: cannot access '%s': %s\n", path, strerror(errno));
            ret = 1;
            continue;
        }
        
        // 修改所有者
        if (change_owner(path, uid, gid, use_lchown) != 0) {
            ret = 1;
            continue;
        }
        
        // 如果是目录且需要递归
        if (recursive && S_ISDIR(st.st_mode)) {
            // 注意：简化实现，不递归处理子目录
            // 完整实现需要使用 nftw() 或手动递归
            fprintf(stderr, "xchown: -R option requires more implementation\n");
        }
    }
    
    return ret;
}
