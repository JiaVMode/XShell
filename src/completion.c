// 定义 POSIX 标准版本，启用 strdup 等函数
// 说明：C99 标准下 strdup 不在标准库中，需要启用 POSIX 扩展
#define _POSIX_C_SOURCE 200809L

// 引入头文件
#include "completion.h"                                 // Tab 补全功能声明
#include "utils.h"                                      // 工具函数（normalize_path）
#include <stdio.h>                                      // 标准输入输出
#include <stdlib.h>                                     // 内存管理（malloc, free）
#include <string.h>                                     // 字符串处理（strcmp, strcpy, strdup 等）
#include <dirent.h>                                     // 目录操作（opendir, readdir）
#include <sys/stat.h>                                   // 文件状态（stat, S_ISDIR）
#include <unistd.h>                                     // UNIX 标准（access）
#include <linux/limits.h>                               // PATH_MAX 常量

// 提取需要补全的路径部分
// 功能：从用户输入中解析出目录前缀和待补全的文件名
// 例如："xcd /home/lab/L" -> prefix="/home/lab/", partial="L"
void extract_path_to_complete(const char *input, int cursor_pos,
                               char *prefix, char *partial) {
    // 步骤1：初始化输出参数为空字符串
    prefix[0] = '\0';                                   // 初始化 prefix 为空
    partial[0] = '\0';                                  // 初始化 partial 为空
    
    // 步骤2：参数有效性检查
    if (input == NULL || cursor_pos < 0) {              // 检查无效输入
        return;                                         // 直接返回，输出为空字符串
    }
    
    // 步骤3：找到光标位置之前最后一个空格（分隔命令和参数）
    // 目的：提取路径参数部分（跳过命令名，如 "xcd"）
    int start = 0;                                      // 路径参数的起始位置
    for (int i = cursor_pos - 1; i >= 0; i--) {         // 从光标位置向前搜索
        if (input[i] == ' ' || input[i] == '\t') {      // 找到空格或制表符
            start = i + 1;                              // 记录参数起始位置
            break;                                      // 跳出循环
        }
    }
    
    // 步骤4：提取路径部分（从 start 到 cursor_pos）
    char path[PATH_MAX];                                // 临时缓冲区存储路径
    int path_len = cursor_pos - start;                  // 计算路径长度
    if (path_len >= PATH_MAX) path_len = PATH_MAX - 1;  // 防止缓冲区溢出
    strncpy(path, input + start, path_len);             // 复制路径字符串
    path[path_len] = '\0';                              // 添加字符串结束符
    
    // 步骤5：规范化路径（将反斜杠 \ 转换为正斜杠 /）
    normalize_path(path);
    
    // 步骤6：找到最后一个 / 分隔目录和文件名
    char *last_slash = strrchr(path, '/');              // 查找最后一个 /
    
    if (last_slash != NULL) {
        // 情况1：路径包含 /，分离目录和文件名
        // 例如："/home/lab/L" -> prefix="/home/lab/", partial="L"
        int prefix_len = last_slash - path + 1;         // 计算前缀长度（包含 /）
        strncpy(prefix, path, prefix_len);              // 复制目录前缀
        prefix[prefix_len] = '\0';                      // 添加结束符
        strcpy(partial, last_slash + 1);                // 复制文件名部分（/ 之后）
    } else {
        // 情况2：路径不包含 /，在当前目录查找
        // 例如："LJ" -> prefix="", partial="LJ"
        strcpy(prefix, "");                             // 空前缀表示当前目录
        strcpy(partial, path);                          // 整个路径作为待补全部分
    }
}

// 获取路径补全建议
// 功能：查找所有匹配给定前缀的文件/目录
// 返回：匹配项数量，匹配项通过 matches 参数返回
int get_path_completions(const char *input, int cursor_pos, char ***matches) {
    char prefix[PATH_MAX];                              // 目录前缀
    char partial[PATH_MAX];                             // 待补全的文件名部分
    
    // 步骤1：提取需要补全的部分
    extract_path_to_complete(input, cursor_pos, prefix, partial);
    
    // 步骤2：确定搜索目录
    char search_dir[PATH_MAX];                          // 要搜索的目录路径
    if (strlen(prefix) == 0) {
        // 情况1：没有路径前缀，在当前目录搜索
        strcpy(search_dir, ".");                        // "." 表示当前目录
    } else {
        // 情况2：有路径前缀，在指定目录搜索
        strcpy(search_dir, prefix);                     // 复制前缀路径
        // 移除末尾的 /（因为 opendir 不需要末尾斜杠）
        int len = strlen(search_dir);                   // 获取路径长度
        if (len > 0 && search_dir[len-1] == '/') {      // 如果末尾是 /
            search_dir[len-1] = '\0';                   // 移除末尾的 /
        }
    }
    
    // 步骤3：打开目录
    DIR *dir = opendir(search_dir);                     // 打开目录流
    if (dir == NULL) {                                  // 打开失败（目录不存在或无权限）
        *matches = NULL;                                // 设置输出为 NULL
        return 0;                                       // 返回 0 表示无匹配
    }
    
    // 步骤4：第一遍遍历 - 统计匹配数量
    // 原因：需要知道数量才能分配正确大小的数组
    int count = 0;                                      // 匹配计数器
    struct dirent *entry;                               // 目录项结构体
    int partial_len = strlen(partial);                  // 待补全部分的长度
    
    while ((entry = readdir(dir)) != NULL) {            // 遍历目录中的每一项
        // 跳过 . 和 ..（当前目录和父目录）
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;                                   // 跳过这两个特殊目录
        }
        
        // 跳过隐藏文件（以 . 开头），除非用户输入了 .
        if (partial_len == 0 && entry->d_name[0] == '.') {
            continue;                                   // 默认不显示隐藏文件
        }
        
        // 检查文件名是否匹配前缀
        if (strncmp(entry->d_name, partial, partial_len) == 0) {
            count++;                                    // 找到一个匹配，计数器加 1
        }
    }
    
    // 步骤5：如果没有匹配项，直接返回
    if (count == 0) {                                   // 没有找到任何匹配
        closedir(dir);                                  // 关闭目录流
        *matches = NULL;                                // 设置输出为 NULL
        return 0;                                       // 返回 0
    }
    
    // 步骤6：分配匹配数组
    *matches = (char **)malloc(count * sizeof(char *)); // 分配指针数组（每个指向一个字符串）
    if (*matches == NULL) {                             // 内存分配失败
        closedir(dir);                                  // 关闭目录流
        return 0;                                       // 返回 0
    }
    
    // 步骤7：第二遍遍历 - 收集匹配项
    rewinddir(dir);                                     // 重置目录流到开头
    int index = 0;                                      // 当前数组索引
    
    while ((entry = readdir(dir)) != NULL && index < count) {
        // 再次跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;                                   // 跳过
        }
        
        // 再次跳过隐藏文件
        if (partial_len == 0 && entry->d_name[0] == '.') {
            continue;                                   // 跳过
        }
        
        // 再次检查匹配
        if (strncmp(entry->d_name, partial, partial_len) == 0) {
            // 步骤7.1：构建完整路径以检查是否为目录
            char full_path[PATH_MAX];                   // 完整路径缓冲区
            if (strlen(search_dir) > 0 && strcmp(search_dir, ".") != 0) {
                // 有路径前缀，拼接路径
                snprintf(full_path, sizeof(full_path), "%s/%s", search_dir, entry->d_name);
            } else {
                // 在当前目录，直接使用文件名
                snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            }
            
            // 步骤7.2：使用 stat 检查是否为目录
            struct stat st;                             // 文件状态结构体
            int is_dir = 0;                             // 是否为目录的标志
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                is_dir = 1;                             // 是目录
            }
            
            // 步骤7.3：为目录添加 / 后缀，方便继续补全
            if (is_dir) {
                // 为目录名分配内存（文件名长度 + "/" + 结束符）
                (*matches)[index] = (char *)malloc(strlen(entry->d_name) + 2);
                if ((*matches)[index] != NULL) {        // 内存分配成功
                    sprintf((*matches)[index], "%s/", entry->d_name); // 添加 / 后缀
                }
            } else {
                // 普通文件，直接复制文件名
                (*matches)[index] = strdup(entry->d_name); // 复制字符串
            }
            
            index++;                                    // 索引加 1
        }
    }
    
    // 步骤8：关闭目录并返回匹配数量
    closedir(dir);                                      // 关闭目录流
    return count;                                       // 返回匹配项数量
}

// 释放补全匹配项
// 功能：释放 get_path_completions() 分配的所有内存
void free_completions(char **matches, int count) {
    // 步骤1：检查空指针
    if (matches == NULL) return;                        // 如果为 NULL，直接返回
    
    // 步骤2：释放每个字符串
    for (int i = 0; i < count; i++) {                   // 遍历数组
        if (matches[i] != NULL) {                       // 如果字符串指针不为 NULL
            free(matches[i]);                           // 释放字符串内存
        }
    }
    
    // 步骤3：释放数组本身
    free(matches);                                      // 释放指针数组
}

// ===== 增强补全功能 =====

// 获取补全类型
CompletionType get_completion_type(const char *input, int cursor_pos) {
    if (input == NULL || cursor_pos <= 0) {
        return COMPLETION_TYPE_PATH;
    }
    
    // 提取光标前的部分
    char before_cursor[4096];
    int len = (cursor_pos < (int)sizeof(before_cursor) - 1) ? cursor_pos : sizeof(before_cursor) - 1;
    strncpy(before_cursor, input, len);
    before_cursor[len] = '\0';
    
    // 跳过前导空格
    int start = 0;
    while (start < len && (before_cursor[start] == ' ' || before_cursor[start] == '\t')) {
        start++;
    }
    
    if (start >= len) {
        return COMPLETION_TYPE_COMMAND;  // 只有空格，补全命令
    }
    
    // 查找第一个空格（命令名结束）
    int cmd_end = start;
    while (cmd_end < len && before_cursor[cmd_end] != ' ' && before_cursor[cmd_end] != '\t') {
        cmd_end++;
    }
    
    // 提取命令名
    char cmd_name[256];
    int cmd_len = cmd_end - start;
    if (cmd_len >= (int)sizeof(cmd_name)) cmd_len = sizeof(cmd_name) - 1;
    strncpy(cmd_name, before_cursor + start, cmd_len);
    cmd_name[cmd_len] = '\0';
    
    // 检查是否在输入选项（以 -- 或 - 开头）
    int arg_start = cmd_end;
    while (arg_start < len && (before_cursor[arg_start] == ' ' || before_cursor[arg_start] == '\t')) {
        arg_start++;
    }
    
    if (arg_start < len) {
        // 检查是否以 -- 或 - 开头
        if (before_cursor[arg_start] == '-' && arg_start + 1 < len && before_cursor[arg_start + 1] == '-') {
            return COMPLETION_TYPE_OPTION;  // 选项补全
        }
        
        // 根据命令名判断补全类型
        if (strcmp(cmd_name, "xcd") == 0 || strcmp(cmd_name, "xmkdir") == 0 || 
            strcmp(cmd_name, "xrmdir") == 0) {
            return COMPLETION_TYPE_DIR_ONLY;  // 只补全目录
        }
        
        // 对于需要文件参数的命令，可以只补全文件（简化实现，暂不区分）
        // 可以根据需要扩展
    } else {
        // 没有参数，可能是命令名补全
        return COMPLETION_TYPE_COMMAND;
    }
    
    return COMPLETION_TYPE_PATH;  // 默认路径补全
}

// 获取命令名补全
int get_command_completions(const char *partial, char ***matches) {
    // 内置命令列表（从 executor.c 中获取）
    const char *builtins[] = {
        "xpwd", "xcd", "xls", "xecho", "quit",
        "xtouch", "xcat", "xrm", "xcp", "xmv",
        "xhistory", "xtec", "xmkdir", "xrmdir", "xln",
        "xchmod", "xfind", "xuname", "xhostname", "xwhoami",
        "xdate", "xuptime", "xps", "xbasename", "xdirname",
        "xreadlink", "xcut", "xpaste", "xtr", "xcomm",
        "xstat", "xfile", "xdu", "xdf", "xsplit",
        "xjoin", "xrealpath", "xmenu",
        "xdiff", "xgrep", "xwc", "xhead", "xtail",
        "xsort", "xuniq", "xenv", "xexport", "xunset",
        "xalias", "xunalias", "xclear", "xhelp", "xtype",
        "xwhich", "xsleep", "xcalc", "xtree", "xsource",
        "xtime", "xkill", "xjobs", "xfg", "xbg",
        NULL
    };
    
    int partial_len = strlen(partial);
    int count = 0;
    
    // 第一遍：统计匹配数量
    for (int i = 0; builtins[i] != NULL; i++) {
        if (strncmp(builtins[i], partial, partial_len) == 0) {
            count++;
        }
    }
    
    if (count == 0) {
        *matches = NULL;
        return 0;
    }
    
    // 分配数组
    *matches = (char **)malloc(count * sizeof(char *));
    if (*matches == NULL) {
        return 0;
    }
    
    // 第二遍：收集匹配项
    int index = 0;
    for (int i = 0; builtins[i] != NULL && index < count; i++) {
        if (strncmp(builtins[i], partial, partial_len) == 0) {
            (*matches)[index] = strdup(builtins[i]);
            index++;
        }
    }
    
    return count;
}

// 获取选项补全
int get_option_completions(const char *cmd_name, const char *partial, char ***matches) {
    // 常见选项列表（简化实现，可以根据命令动态生成）
    const char *common_options[] = {
        "--help", "-h", "--version", "-v"
    };
    
    // 根据命令名返回特定选项（简化实现）
    // 实际应该解析命令的帮助信息或定义
    const char *options[20];
    int option_count = 0;
    
    // 添加通用选项
    options[option_count++] = "--help";
    options[option_count++] = "-h";
    
    // 根据命令添加特定选项（简化实现）
    if (strcmp(cmd_name, "xls") == 0) {
        options[option_count++] = "-l";
        options[option_count++] = "-a";
        options[option_count++] = "-h";
        options[option_count++] = "-R";
    } else if (strcmp(cmd_name, "xcp") == 0) {
        options[option_count++] = "-r";
        options[option_count++] = "-R";
        options[option_count++] = "--progress";
    } else if (strcmp(cmd_name, "xcat") == 0) {
        options[option_count++] = "-n";
        options[option_count++] = "-b";
    } else if (strcmp(cmd_name, "xrm") == 0) {
        options[option_count++] = "-r";
        options[option_count++] = "-f";
        options[option_count++] = "-i";
    } else if (strcmp(cmd_name, "xfind") == 0) {
        options[option_count++] = "-name";
        options[option_count++] = "-type";
        options[option_count++] = "-size";
    }
    
    int partial_len = strlen(partial);
    int count = 0;
    
    // 第一遍：统计匹配数量
    for (int i = 0; i < option_count; i++) {
        if (strncmp(options[i], partial, partial_len) == 0) {
            count++;
        }
    }
    
    if (count == 0) {
        *matches = NULL;
        return 0;
    }
    
    // 分配数组
    *matches = (char **)malloc(count * sizeof(char *));
    if (*matches == NULL) {
        return 0;
    }
    
    // 第二遍：收集匹配项
    int index = 0;
    for (int i = 0; i < option_count && index < count; i++) {
        if (strncmp(options[i], partial, partial_len) == 0) {
            (*matches)[index] = strdup(options[i]);
            index++;
        }
    }
    
    return count;
}

// 获取增强路径补全
int get_enhanced_path_completions(const char *input, int cursor_pos, 
                                   CompletionType completion_type, char ***matches) {
    char prefix[PATH_MAX];
    char partial[PATH_MAX];
    
    extract_path_to_complete(input, cursor_pos, prefix, partial);
    
    char search_dir[PATH_MAX];
    if (strlen(prefix) == 0) {
        strcpy(search_dir, ".");
    } else {
        strcpy(search_dir, prefix);
        int len = strlen(search_dir);
        if (len > 0 && search_dir[len-1] == '/') {
            search_dir[len-1] = '\0';
        }
    }
    
    DIR *dir = opendir(search_dir);
    if (dir == NULL) {
        *matches = NULL;
        return 0;
    }
    
    int count = 0;
    struct dirent *entry;
    int partial_len = strlen(partial);
    
    // 第一遍：统计匹配数量
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (partial_len == 0 && entry->d_name[0] == '.') {
            continue;
        }
        
        if (strncmp(entry->d_name, partial, partial_len) != 0) {
            continue;
        }
        
        // 根据补全类型过滤
        if (completion_type == COMPLETION_TYPE_DIR_ONLY) {
            char full_path[PATH_MAX];
            if (strlen(search_dir) > 0 && strcmp(search_dir, ".") != 0) {
                snprintf(full_path, sizeof(full_path), "%s/%s", search_dir, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            }
            
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISDIR(st.st_mode)) {
                count++;
            }
        } else if (completion_type == COMPLETION_TYPE_FILE_ONLY) {
            char full_path[PATH_MAX];
            if (strlen(search_dir) > 0 && strcmp(search_dir, ".") != 0) {
                snprintf(full_path, sizeof(full_path), "%s/%s", search_dir, entry->d_name);
            } else {
                snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
            }
            
            struct stat st;
            if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
                count++;
            }
        } else {
            count++;
        }
    }
    
    if (count == 0) {
        closedir(dir);
        *matches = NULL;
        return 0;
    }
    
    *matches = (char **)malloc(count * sizeof(char *));
    if (*matches == NULL) {
        closedir(dir);
        return 0;
    }
    
    // 第二遍：收集匹配项
    rewinddir(dir);
    int index = 0;
    
    while ((entry = readdir(dir)) != NULL && index < count) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        if (partial_len == 0 && entry->d_name[0] == '.') {
            continue;
        }
        
        if (strncmp(entry->d_name, partial, partial_len) != 0) {
            continue;
        }
        
        char full_path[PATH_MAX];
        if (strlen(search_dir) > 0 && strcmp(search_dir, ".") != 0) {
            snprintf(full_path, sizeof(full_path), "%s/%s", search_dir, entry->d_name);
        } else {
            snprintf(full_path, sizeof(full_path), "%s", entry->d_name);
        }
        
        struct stat st;
        int is_dir = 0;
        if (stat(full_path, &st) == 0) {
            is_dir = S_ISDIR(st.st_mode);
        }
        
        // 根据补全类型过滤
        if (completion_type == COMPLETION_TYPE_DIR_ONLY && !is_dir) {
            continue;
        }
        if (completion_type == COMPLETION_TYPE_FILE_ONLY && is_dir) {
            continue;
        }
        
        if (is_dir) {
            (*matches)[index] = (char *)malloc(strlen(entry->d_name) + 2);
            if ((*matches)[index] != NULL) {
                sprintf((*matches)[index], "%s/", entry->d_name);
            }
        } else {
            (*matches)[index] = strdup(entry->d_name);
        }
        
        index++;
    }
    
    closedir(dir);
    return count;
}

// 智能补全（主入口函数）
int get_smart_completions(const char *input, int cursor_pos, char ***matches) {
    CompletionType type = get_completion_type(input, cursor_pos);
    
    switch (type) {
        case COMPLETION_TYPE_COMMAND: {
            // 提取部分命令名
            char partial[256] = {0};
            int start = 0;
            while (start < cursor_pos && (input[start] == ' ' || input[start] == '\t')) {
                start++;
            }
            int len = cursor_pos - start;
            if (len > 0 && len < (int)sizeof(partial)) {
                strncpy(partial, input + start, len);
                partial[len] = '\0';
            }
            return get_command_completions(partial, matches);
        }
        
        case COMPLETION_TYPE_OPTION: {
            // 提取命令名和部分选项
            char cmd_name[256] = {0};
            char partial[256] = {0};
            
            int cmd_start = 0;
            while (cmd_start < cursor_pos && (input[cmd_start] == ' ' || input[cmd_start] == '\t')) {
                cmd_start++;
            }
            
            int cmd_end = cmd_start;
            while (cmd_end < cursor_pos && input[cmd_end] != ' ' && input[cmd_end] != '\t') {
                cmd_end++;
            }
            
            int cmd_len = cmd_end - cmd_start;
            if (cmd_len > 0 && cmd_len < (int)sizeof(cmd_name)) {
                strncpy(cmd_name, input + cmd_start, cmd_len);
                cmd_name[cmd_len] = '\0';
            }
            
            int opt_start = cmd_end;
            while (opt_start < cursor_pos && (input[opt_start] == ' ' || input[opt_start] == '\t')) {
                opt_start++;
            }
            
            int opt_len = cursor_pos - opt_start;
            if (opt_len > 0 && opt_len < (int)sizeof(partial)) {
                strncpy(partial, input + opt_start, opt_len);
                partial[opt_len] = '\0';
            }
            
            return get_option_completions(cmd_name, partial, matches);
        }
        
        case COMPLETION_TYPE_DIR_ONLY:
        case COMPLETION_TYPE_FILE_ONLY:
        case COMPLETION_TYPE_PATH:
        default:
            return get_enhanced_path_completions(input, cursor_pos, type, matches);
    }
}
