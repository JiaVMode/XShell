// 定义 POSIX 和 GNU 扩展，启用更多系统函数
#define _POSIX_C_SOURCE 200809L             // POSIX 2008 标准
#define _XOPEN_SOURCE 700                   // X/Open 7 标准
#define _DEFAULT_SOURCE                     // 默认功能（包括 BSD 扩展）

// 引入自定义头文件
#include "builtin.h"                        // 内置命令函数声明

// 引入标准库
#include <stdio.h>                          // 标准输入输出（printf, perror）
#include <stdlib.h>                         // 标准库（malloc, free, qsort）
#include <string.h>                         // 字符串处理（strcmp, strlen, strcasecmp）
#include <strings.h>                        // BSD 字符串函数（strcasecmp）
#include <sys/types.h>                      // 系统类型（off_t, mode_t, ssize_t）
#include <sys/stat.h>                       // 文件状态（stat, lstat, S_ISDIR, S_ISLNK 等）
#include <dirent.h>                         // 目录操作（opendir, readdir, closedir）
#include <time.h>                           // 时间处理（localtime, strftime）
#include <pwd.h>                            // 用户信息（getpwuid）
#include <grp.h>                            // 组信息（getgrgid）
#include <unistd.h>                         // UNIX 标准（readlink）
#include <linux/limits.h>                   // PATH_MAX 常量

// ANSI 颜色代码定义
// 用途：为不同类型的文件添加颜色，提升可读性
#define COLOR_RESET   "\033[0m"             // 重置颜色
#define COLOR_DIR     "\033[1;34m"          // 蓝色（目录）
#define COLOR_EXEC    "\033[1;32m"          // 绿色（可执行文件）
#define COLOR_LINK    "\033[1;36m"          // 青色（符号链接）
#define COLOR_NORMAL  "\033[0m"             // 普通文件（默认色）

// 文件信息结构体
// 用途：存储每个文件的详细信息，便于排序和格式化输出
typedef struct {
    char name[256];                         // 文件名
    struct stat st;                         // 文件状态信息（权限、大小、时间等）
    char full_path[PATH_MAX];               // 完整路径（用于获取文件信息）
} FileInfo;

// xls 命令选项结构体
// 用途：存储用户指定的命令行选项
typedef struct {
    int show_all;                           // -a：显示隐藏文件（以 . 开头）
    int long_format;                        // -l：详细列表格式
    int human_readable;                     // -h：人性化显示文件大小（如 1.5K, 2.3M）
    int use_color;                          // 是否使用彩色输出（默认开启）
} LsOptions;

// 文件比较函数（用于 qsort 排序）
// 功能：按文件名字母顺序排序（忽略大小写）
// 返回：< 0 表示 a < b，0 表示相等，> 0 表示 a > b
static int compare_files(const void *a, const void *b) {
    const FileInfo *fa = (const FileInfo *)a;   // 转换为 FileInfo 指针
    const FileInfo *fb = (const FileInfo *)b;   // 转换为 FileInfo 指针
    return strcasecmp(fa->name, fb->name);      // 忽略大小写比较（目录优先可以在这里实现）
}

// 格式化文件大小（人性化显示）
// 功能：将字节数转换为人类可读的格式（B, K, M, G）
// 例如：1024 -> "1.0K"，1048576 -> "1.0M"
static void format_size_human(off_t size, char *buf, size_t bufsize) {
    const char *units[] = {"B", "K", "M", "G", "T"}; // 单位数组
    int unit_index = 0;                         // 当前单位索引
    double dsize = (double)size;                // 转换为浮点数便于计算
    
    // 循环除以 1024 直到小于 1024 或达到最大单位
    while (dsize >= 1024.0 && unit_index < 4) {
        dsize /= 1024.0;                        // 除以 1024
        unit_index++;                           // 单位升级（B->K->M->G->T）
    }
    
    // 格式化输出
    if (unit_index == 0) {                      // 字节（B）
        snprintf(buf, bufsize, "%4.0f%s", dsize, units[unit_index]);
    } else {                                    // K, M, G, T
        snprintf(buf, bufsize, "%4.1f%s", dsize, units[unit_index]);
    }
}

// 格式化文件权限
// 功能：将数字权限（如 0755）转换为字符串（如 rwxr-xr-x）
static void format_permissions(mode_t mode, char *buf) {
    // 文件类型
    buf[0] = S_ISDIR(mode) ? 'd' :              // 目录
             S_ISLNK(mode) ? 'l' :              // 符号链接
             S_ISCHR(mode) ? 'c' :              // 字符设备
             S_ISBLK(mode) ? 'b' :              // 块设备
             S_ISFIFO(mode) ? 'p' :             // 管道
             S_ISSOCK(mode) ? 's' : '-';        // 套接字或普通文件
    
    // 用户权限（所有者）
    buf[1] = (mode & S_IRUSR) ? 'r' : '-';      // 读
    buf[2] = (mode & S_IWUSR) ? 'w' : '-';      // 写
    buf[3] = (mode & S_IXUSR) ? 'x' : '-';      // 执行
    
    // 组权限
    buf[4] = (mode & S_IRGRP) ? 'r' : '-';      // 读
    buf[5] = (mode & S_IWGRP) ? 'w' : '-';      // 写
    buf[6] = (mode & S_IXGRP) ? 'x' : '-';      // 执行
    
    // 其他用户权限
    buf[7] = (mode & S_IROTH) ? 'r' : '-';      // 读
    buf[8] = (mode & S_IWOTH) ? 'w' : '-';      // 写
    buf[9] = (mode & S_IXOTH) ? 'x' : '-';      // 执行
    
    buf[10] = '\0';                             // 字符串结束符
}

// 获取文件颜色代码
// 功能：根据文件类型返回对应的 ANSI 颜色代码
static const char* get_file_color(const struct stat *st) {
    if (S_ISDIR(st->st_mode)) {                 // 目录
        return COLOR_DIR;
    } else if (S_ISLNK(st->st_mode)) {          // 符号链接
        return COLOR_LINK;
    } else if (st->st_mode & S_IXUSR) {         // 可执行文件（用户可执行位）
        return COLOR_EXEC;
    }
    return COLOR_NORMAL;                        // 普通文件
}

// 打印详细列表格式（-l 选项）
// 功能：显示文件的详细信息（权限、所有者、大小、时间等）
static void print_long_format(const FileInfo *file, const LsOptions *opts) {
    char perms[11];                             // 权限字符串（如 drwxr-xr-x）
    char size_str[16];                          // 文件大小字符串
    char time_str[32];                          // 时间字符串
    struct tm *tm_info;                         // 时间结构体
    struct passwd *pw;                          // 用户信息
    struct group *gr;                           // 组信息
    
    // 步骤1：格式化权限
    format_permissions(file->st.st_mode, perms);
    
    // 步骤2：获取用户名和组名
    pw = getpwuid(file->st.st_uid);             // 根据 UID 获取用户信息
    gr = getgrgid(file->st.st_gid);             // 根据 GID 获取组信息
    
    // 步骤3：格式化文件大小
    if (opts->human_readable) {                 // -h 选项：人性化显示
        format_size_human(file->st.st_size, size_str, sizeof(size_str));
    } else {                                    // 普通格式：显示字节数
        snprintf(size_str, sizeof(size_str), "%8ld", (long)file->st.st_size);
    }
    
    // 步骤4：格式化修改时间
    tm_info = localtime(&file->st.st_mtime);    // 转换为本地时间
    strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info); // 格式：Jan 15 14:30
    
    // 步骤5：输出详细信息
    // 格式：权限 链接数 用户 组 大小 时间 文件名
    printf("%s %3ld %-8s %-8s %s %s ",
           perms,                               // 权限
           (long)file->st.st_nlink,             // 硬链接数
           pw ? pw->pw_name : "?",              // 用户名（失败显示 ?）
           gr ? gr->gr_name : "?",              // 组名（失败显示 ?）
           size_str,                            // 文件大小
           time_str);                           // 修改时间
    
    // 步骤6：输出文件名（带颜色）
    if (opts->use_color) {
        printf("%s%s%s", get_file_color(&file->st), file->name, COLOR_RESET);
    } else {
        printf("%s", file->name);
    }
    
    // 步骤7：目录添加 / 后缀，符号链接显示目标
    if (S_ISDIR(file->st.st_mode)) {
        printf("/");
    } else if (S_ISLNK(file->st.st_mode)) {
        char link_target[PATH_MAX];
        ssize_t len = readlink(file->full_path, link_target, sizeof(link_target) - 1);
        if (len != -1) {
            link_target[len] = '\0';
            printf(" -> %s", link_target);      // 显示链接目标
        }
    }
    
    printf("\n");                               // 换行
}

// 打印简单格式（默认）
// 功能：简洁显示文件名，带颜色和类型后缀
// 如果指定了 -h 选项，还会显示文件大小
static void print_simple_format(const FileInfo *file, const LsOptions *opts) {
    // 输出文件名（带颜色）
    if (opts->use_color) {
        printf("%s%s%s", get_file_color(&file->st), file->name, COLOR_RESET);
    } else {
        printf("%s", file->name);
    }
    
    // 添加类型后缀
    if (S_ISDIR(file->st.st_mode)) {
        printf("/");                            // 目录加 /
    } else if (S_ISLNK(file->st.st_mode)) {
        printf("@");                            // 符号链接加 @
    } else if (file->st.st_mode & S_IXUSR) {
        printf("*");                            // 可执行文件加 *
    }
    
    // 如果指定了 -h 选项（人性化大小），在文件名后显示大小
    if (opts->human_readable) {
        char size_buf[16];                      // 大小字符串缓冲区
        format_size_human(file->st.st_size, size_buf, sizeof(size_buf));
        printf(" (%s)", size_buf);              // 显示大小，格式如 (1.2K)
    }
    
    printf("  ");                               // 两个空格分隔（多列显示）
}

// 解析命令选项
// 功能：解析 -l, -a, -h 等选项
static void parse_options(Command *cmd, LsOptions *opts, const char **path) {
    // 初始化选项为默认值
    opts->show_all = 0;                         // 默认不显示隐藏文件
    opts->long_format = 0;                      // 默认简单格式
    opts->human_readable = 0;                   // 默认显示字节数
    opts->use_color = 1;                        // 默认使用彩色
    *path = ".";                                // 默认当前目录
    
    // 遍历所有参数
    for (int i = 1; i < cmd->arg_count; i++) {
        if (cmd->args[i][0] == '-') {           // 如果是选项（以 - 开头）
            // 遍历选项字符串中的每个字符（支持 -la 这样的组合）
            for (int j = 1; cmd->args[i][j] != '\0'; j++) {
                switch (cmd->args[i][j]) {
                    case 'l':
                        opts->long_format = 1;  // -l：详细列表
                        break;
                    case 'a':
                        opts->show_all = 1;     // -a：显示所有文件（包括隐藏）
                        break;
                    case 'h':
                        opts->human_readable = 1; // -h：人性化大小
                        break;
                    default:
                        // 未知选项，忽略
                        break;
                }
            }
        } else {
            // 不是选项，认为是路径参数
            *path = cmd->args[i];
        }
    }
}

// xls 命令主函数
// 命令名称：xls
// 对应系统命令：ls
// 支持选项：
//   -l：详细列表格式
//   -a：显示隐藏文件
//   -h：人性化显示文件大小
//   -la, -lh, -lah 等组合
// 使用示例：
//   xls              -> 显示当前目录
//   xls -l           -> 详细列表
//   xls -la          -> 详细列表，包括隐藏文件
//   xls -lh /home    -> 详细列表，人性化大小，显示 /home
int cmd_xls(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xls - 列出文件和目录\n\n");
        printf("用法:\n");
        printf("  xls [选项] [路径] [--help]\n\n");
        printf("说明:\n");
        printf("  显示指定目录下的文件和文件夹。\n");
        printf("  List - 列出文件和目录。\n\n");
        printf("选项:\n");
        printf("  -l        详细列表格式（权限、所有者、大小、时间）\n");
        printf("  -a        显示隐藏文件（以 . 开头的文件）\n");
        printf("  -h        人性化显示文件大小（KB、MB、GB）\n");
        printf("            单独使用：简洁列表 + 文件大小\n");
        printf("            配合 -l：详细列表 + 人性化大小\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("组合选项:\n");
        printf("  -la       详细列表 + 隐藏文件\n");
        printf("  -lh       详细列表 + 人性化大小\n");
        printf("  -lah      详细列表 + 隐藏文件 + 人性化大小\n");
        printf("  -ah       简洁列表 + 隐藏文件 + 大小\n\n");
        printf("彩色输出:\n");
        printf("  蓝色      目录（/ 后缀）\n");
        printf("  绿色      可执行文件（* 后缀）\n");
        printf("  青色      符号链接（@ 后缀）\n");
        printf("  默认色    普通文件\n\n");
        printf("示例:\n");
        printf("  xls                  # 简洁列表\n");
        printf("  xls -h               # 简洁列表 + 文件大小\n");
        printf("  xls -l               # 详细列表（字节）\n");
        printf("  xls -lh              # 详细列表（人性化大小）\n");
        printf("  xls -a               # 显示隐藏文件\n");
        printf("  xls -lah /home       # 详细列表 + 隐藏文件 + 人性化大小\n\n");
        printf("对应系统命令: ls\n");
        return 0;
    }

    LsOptions opts;                             // 选项结构体
    const char *path;                           // 目录路径
    
    // 步骤1：解析命令选项
    parse_options(cmd, &opts, &path);
    
    // 步骤2：打开目录
    DIR *dir = opendir(path);
    if (dir == NULL) {
        XSHELL_LOG_PERROR(ctx, path);
        return -1;
    }
    
    // 步骤3：收集所有文件信息到数组（用于排序）
    FileInfo *files = NULL;                     // 文件信息数组
    int file_count = 0;                         // 文件数量
    int capacity = 32;                          // 数组容量（初始 32，需要时扩展）
    
    files = (FileInfo *)malloc(capacity * sizeof(FileInfo)); // 分配初始内存
    if (files == NULL) {
        XSHELL_LOG_PERROR(ctx, "malloc");
        closedir(dir);
        return -1;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        // 步骤3.1：跳过 . 和 ..（除非指定 -a）
        if (!opts.show_all) {
            if (entry->d_name[0] == '.') {      // 隐藏文件（以 . 开头）
                // 但 . 和 .. 总是跳过
                if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
                    continue;
                }
                continue;                       // 跳过其他隐藏文件
            }
        }
        
        // 步骤3.2：如果数组已满，扩展容量
        if (file_count >= capacity) {
            capacity *= 2;                      // 容量翻倍
            FileInfo *new_files = (FileInfo *)realloc(files, capacity * sizeof(FileInfo));
            if (new_files == NULL) {
                XSHELL_LOG_PERROR(ctx, "realloc");
                free(files);
                closedir(dir);
                return -1;
            }
            files = new_files;
        }
        
        // 步骤3.3：保存文件信息
        strncpy(files[file_count].name, entry->d_name, sizeof(files[file_count].name) - 1);
        files[file_count].name[sizeof(files[file_count].name) - 1] = '\0';
        
        // 构建完整路径
        snprintf(files[file_count].full_path, sizeof(files[file_count].full_path),
                 "%s/%s", path, entry->d_name);
        
        // 获取文件状态
        if (lstat(files[file_count].full_path, &files[file_count].st) != 0) {
            // lstat 失败，使用默认值
            memset(&files[file_count].st, 0, sizeof(struct stat));
        }
        
        file_count++;
    }
    
    closedir(dir);
    
    // 步骤4：排序文件列表
    qsort(files, file_count, sizeof(FileInfo), compare_files);
    
    // 步骤5：输出文件列表
    for (int i = 0; i < file_count; i++) {
        if (opts.long_format) {
            print_long_format(&files[i], &opts); // 详细格式
        } else {
            print_simple_format(&files[i], &opts); // 简单格式
        }
    }
    
    // 简单格式需要额外换行
    if (!opts.long_format && file_count > 0) {
        printf("\n");
    }
    
    // 步骤6：清理内存
    free(files);
    
    // 标记上下文为已使用
    (void)ctx;
    
    return 0;
}
