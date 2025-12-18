// 特性测试宏：启用 POSIX 标准函数
#define _XOPEN_SOURCE 700       // For POSIX.1-2008

// 引入自定义头文件
#include "builtin.h"            // 内置命令函数声明

// 引入标准库
#include <stdio.h>              // 标准输入输出（printf, fprintf）
#include <string.h>             // 字符串处理（strcmp, strlen, strstr）
#include <stdlib.h>             // 标准库（malloc, free）
#include <sys/stat.h>           // 文件状态（stat, S_ISDIR）
#include <sys/types.h>          // 系统类型定义
#include <dirent.h>             // 目录操作（opendir, readdir）
#include <limits.h>             // 系统限制（PATH_MAX）
#include <fnmatch.h>            // 文件名匹配（fnmatch）

// ============================================
// 递归查找文件的辅助函数
// ============================================
// 功能：在指定目录中递归查找匹配模式的文件
// 参数：
//   path: 搜索路径
//   pattern: 文件名模式（支持通配符 * 和 ?）
// 返回值：无
static void find_files(const char *path, const char *pattern, ShellContext *ctx) {
    DIR *dir;
    struct dirent *entry;
    struct stat statbuf;
    char full_path[PATH_MAX];
    
    // 步骤1：打开目录
    dir = opendir(path);
    if (dir == NULL) {
        XSHELL_LOG_PERROR(ctx, path);
        return;
    }
    
    // 步骤2：遍历目录中的所有项
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        // 构建完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // 获取文件信息
        if (lstat(full_path, &statbuf) == -1) {
            XSHELL_LOG_PERROR(ctx, full_path);
            continue;
        }
        
        // 检查文件名是否匹配模式
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            printf("%s\n", full_path);
        }
        
        // 如果是目录，递归查找
        if (S_ISDIR(statbuf.st_mode)) {
            find_files(full_path, pattern, ctx);
        }
    }
    
    // 步骤3：关闭目录
    closedir(dir);
}

// ============================================
// xfind 命令实现函数
// ============================================
// 命令名称：xfind
// 对应系统命令：find
// 功能：查找文件
// 用法：xfind <路径> -name <模式>
//
// 选项：
//   -name <模式>  按文件名查找（支持通配符 * 和 ?）
//   --help        显示帮助信息
//
// 示例：
//   xfind . -name "*.txt"          # 查找所有 .txt 文件
//   xfind /home -name "test*"      # 查找以 test 开头的文件
//   xfind . -name "*.c"            # 查找所有 C 源文件
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xfind(Command *cmd, ShellContext *ctx) {

    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xfind - 查找文件\n\n");
        printf("用法:\n");
        printf("  xfind <路径> -name <模式>\n\n");
        printf("说明:\n");
        printf("  在指定路径中递归查找匹配模式的文件。\n");
        printf("  支持通配符 * （任意字符）和 ? （单个字符）。\n\n");
        printf("参数:\n");
        printf("  路径      搜索的起始路径\n");
        printf("  模式      文件名匹配模式（支持通配符）\n\n");
        printf("选项:\n");
        printf("  -name <模式>  按文件名查找\n");
        printf("  --help        显示此帮助信息\n\n");
        printf("示例:\n");
        printf("  xfind . -name \"*.txt\"\n");
        printf("    查找当前目录及子目录中所有 .txt 文件\n\n");
        printf("  xfind /home -name \"test*\"\n");
        printf("    查找 /home 中所有以 test 开头的文件\n\n");
        printf("  xfind . -name \"*.c\"\n");
        printf("    查找所有 C 源文件\n\n");
        printf("  xfind /tmp -name \"temp_*\"\n");
        printf("    查找所有临时文件\n\n");
        printf("通配符说明:\n");
        printf("  *    匹配任意数量的任意字符\n");
        printf("  ?    匹配单个任意字符\n");
        printf("  []   匹配括号中的任意一个字符\n\n");
        printf("示例模式:\n");
        printf("  *.txt        所有 .txt 文件\n");
        printf("  test*        以 test 开头的文件\n");
        printf("  *.c          所有 C 源文件\n");
        printf("  file?.txt    file1.txt, file2.txt 等\n");
        printf("  [abc]*       以 a、b 或 c 开头的文件\n\n");
        printf("注意:\n");
        printf("  • 模式需要用引号括起来，避免被 Shell 展开\n");
        printf("  • 搜索会递归进入所有子目录\n");
        printf("  • 当前实现为简化版，仅支持 -name 选项\n\n");
        printf("对应系统命令: find\n");
        return 0;
    }

    // 步骤1：检查参数数量
    if (cmd->arg_count < 4) {
        XSHELL_LOG_ERROR(ctx, "xfind: missing operand\n");
        XSHELL_LOG_ERROR(ctx, "Usage: xfind <path> -name <pattern>\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xfind --help' for more information.\n");
        return -1;
    }

    // 步骤2：解析参数
    const char *search_path = cmd->args[1];     // 搜索路径
    const char *name_option = cmd->args[2];     // -name 选项
    const char *pattern = cmd->args[3];         // 文件名模式
    
    // 去掉模式中的引号（如果有的话）
    char clean_pattern[256];
    size_t pattern_len = strlen(pattern);
    if (pattern_len >= 2 && 
        ((pattern[0] == '"' && pattern[pattern_len - 1] == '"') ||
         (pattern[0] == '\'' && pattern[pattern_len - 1] == '\''))) {
        // 有引号，去掉
        strncpy(clean_pattern, pattern + 1, pattern_len - 2);
        clean_pattern[pattern_len - 2] = '\0';
        pattern = clean_pattern;
    }

    // 检查 -name 选项
    if (strcmp(name_option, "-name") != 0) {
        XSHELL_LOG_ERROR(ctx, "xfind: unsupported option: '%s'\n", name_option);
        XSHELL_LOG_ERROR(ctx, "Currently only -name option is supported\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xfind --help' for more information.\n");
        return -1;
    }

    // 步骤3：检查搜索路径是否存在
    struct stat statbuf;
    if (lstat(search_path, &statbuf) == -1) {
        XSHELL_LOG_PERROR(ctx, search_path);
        return -1;
    }

    // 检查是否为目录
    if (!S_ISDIR(statbuf.st_mode)) {
        XSHELL_LOG_ERROR(ctx, "xfind: '%s' is not a directory\n", search_path);
        return -1;
    }

    // 步骤4：开始查找
    find_files(search_path, pattern, ctx);

    // 步骤5：返回成功
    return 0;
}

