// 定义 POSIX 标准版本，启用 strdup 等函数
#define _POSIX_C_SOURCE 200809L

// 引入自定义头文件
#include "executor.h"                                           // 命令执行函数声明
#include "builtin.h"                                            // 内置命令函数声明（cmd_xpwd, cmd_quit等）
#include "xui.h"                                                 // UI 系统函数声明
#include "xweb.h"                                                // 网页浏览器函数声明
#include "xgame.h"                                               // 游戏函数声明
#include "job.h"                                                 // 作业管理函数声明

// 引入标准库
#include <stdio.h>                                              // 标准输入输出（fprintf）
#include <string.h>                                             // 字符串处理（strcmp, strdup）
#include <unistd.h>                                             // 系统调用（fork, exec, wait, dup2, pipe）
#include <sys/types.h>                                           // 系统类型（pid_t）
#include <sys/wait.h>                                           // 进程等待（waitpid）
#include <sys/stat.h>                                           // 文件状态（stat）
#include <fcntl.h>                                              // 文件控制（open, O_CREAT等）
#include <errno.h>                                              // 错误号（errno, strerror）
#include <stdlib.h>                                             // 标准库（atoi, malloc, realloc, free）

// 大括号展开：展开 {start..end} 表达式
// 例如：test{1..3}.txt -> test1.txt test2.txt test3.txt
// 返回：展开后的字符串数组，以 NULL 结尾，调用者负责释放
char** expand_brace(const char *arg) {
    if (arg == NULL) {
        return NULL;
    }
    
    // 查找大括号 {start..end}
    const char *open_brace = strchr(arg, '{');
    if (open_brace == NULL) {
        // 没有大括号，返回原参数
        char **result = malloc(2 * sizeof(char*));
        if (result == NULL) {
            return NULL;
        }
        result[0] = strdup(arg);
        result[1] = NULL;
        return result;
    }
    
    const char *close_brace = strchr(open_brace, '}');
    if (close_brace == NULL) {
        // 没有闭合大括号，返回原参数
        char **result = malloc(2 * sizeof(char*));
        if (result == NULL) {
            return NULL;
        }
        result[0] = strdup(arg);
        result[1] = NULL;
        return result;
    }
    
    // 查找 .. 分隔符
    const char *dots = strstr(open_brace, "..");
    if (dots == NULL || dots >= close_brace) {
        // 没有 .. 分隔符，返回原参数
        char **result = malloc(2 * sizeof(char*));
        if (result == NULL) {
            return NULL;
        }
        result[0] = strdup(arg);
        result[1] = NULL;
        return result;
    }
    
    // 提取前缀和后缀
    size_t prefix_len = open_brace - arg;
    size_t suffix_len = strlen(close_brace + 1);
    
    // 提取 start 和 end
    size_t start_len = dots - (open_brace + 1);
    size_t end_len = close_brace - (dots + 2);
    
    char *start_str = malloc(start_len + 1);
    char *end_str = malloc(end_len + 1);
    if (start_str == NULL || end_str == NULL) {
        free(start_str);
        free(end_str);
        return NULL;
    }
    
    strncpy(start_str, open_brace + 1, start_len);
    start_str[start_len] = '\0';
    strncpy(end_str, dots + 2, end_len);
    end_str[end_len] = '\0';
    
    // 转换为整数
    int start = atoi(start_str);
    int end = atoi(end_str);
    
    free(start_str);
    free(end_str);
    
    // 计算范围大小
    int step = (start <= end) ? 1 : -1;
    int count = (start <= end) ? (end - start + 1) : (start - end + 1);
    
    // 分配结果数组
    char **result = malloc((count + 1) * sizeof(char*));
    if (result == NULL) {
        return NULL;
    }
    
    // 构建前缀和后缀字符串
    char *prefix = malloc(prefix_len + 1);
    char *suffix = malloc(suffix_len + 1);
    if (prefix == NULL || suffix == NULL) {
        free(prefix);
        free(suffix);
        free(result);
        return NULL;
    }
    
    strncpy(prefix, arg, prefix_len);
    prefix[prefix_len] = '\0';
    strcpy(suffix, close_brace + 1);
    
    // 生成展开后的字符串
    int idx = 0;
    for (int i = start; (step > 0) ? (i <= end) : (i >= end); i += step) {
        // 计算需要的空间：前缀 + 数字字符串 + 后缀 + '\0'
        int num_digits = (i == 0) ? 1 : 0;
        int temp = (i < 0) ? -i : i;
        while (temp > 0) {
            num_digits++;
            temp /= 10;
        }
        if (i < 0) num_digits++; // 负号
        
        size_t total_len = prefix_len + num_digits + suffix_len + 1;
        result[idx] = malloc(total_len);
        if (result[idx] == NULL) {
            // 释放已分配的内存
            for (int j = 0; j < idx; j++) {
                free(result[j]);
            }
            free(result);
            free(prefix);
            free(suffix);
            return NULL;
        }
        
        snprintf(result[idx], total_len, "%s%d%s", prefix, i, suffix);
        idx++;
    }
    
    result[idx] = NULL;
    free(prefix);
    free(suffix);
    
    return result;
}

// 展开命令参数中的大括号表达式
// 返回：新的参数数组，调用者负责释放
static char** expand_args(char **args, int arg_count) {
    if (args == NULL || arg_count <= 0) {
        return NULL;
    }
    
    // 第一遍：计算展开后的总参数数量
    int total_count = 0;
    char ***expanded = malloc(arg_count * sizeof(char**));
    int *expanded_counts = malloc(arg_count * sizeof(int));
    
    if (expanded == NULL || expanded_counts == NULL) {
        free(expanded);
        free(expanded_counts);
        return NULL;
    }
    
    for (int i = 0; i < arg_count; i++) {
        expanded[i] = expand_brace(args[i]);
        if (expanded[i] == NULL) {
            // 展开失败，使用原参数
            expanded[i] = malloc(2 * sizeof(char*));
            if (expanded[i] != NULL) {
                expanded[i][0] = strdup(args[i]);
                expanded[i][1] = NULL;
            }
        }
        
        // 计算展开后的数量
        int count = 0;
        if (expanded[i] != NULL) {
            for (int j = 0; expanded[i][j] != NULL; j++) {
                count++;
            }
        }
        expanded_counts[i] = count;
        total_count += count;
    }
    
    // 第二遍：合并所有展开后的参数
    char **result = malloc((total_count + 1) * sizeof(char*));
    if (result == NULL) {
        // 释放已分配的内存
        for (int i = 0; i < arg_count; i++) {
            if (expanded[i] != NULL) {
                for (int j = 0; expanded[i][j] != NULL; j++) {
                    free(expanded[i][j]);
                }
                free(expanded[i]);
            }
        }
        free(expanded);
        free(expanded_counts);
        return NULL;
    }
    
    int result_idx = 0;
    for (int i = 0; i < arg_count; i++) {
        if (expanded[i] != NULL) {
            for (int j = 0; expanded[i][j] != NULL; j++) {
                result[result_idx++] = expanded[i][j];
            }
            free(expanded[i]); // 释放数组本身，但保留字符串
        }
    }
    result[result_idx] = NULL;
    
    free(expanded);
    free(expanded_counts);
    
    return result;
}

// 在PATH中查找可执行文件（使用手动解析，避免strtok_r问题）
static char* find_executable(const char *cmd_name) {
    if (cmd_name == NULL) {
        return NULL;
    }
    
    // 如果包含路径分隔符，直接返回
    if (strchr(cmd_name, '/') != NULL) {
        if (access(cmd_name, X_OK) == 0) {
            return strdup(cmd_name);
        }
        return NULL;
    }
    
    // 从PATH环境变量中查找
    char *path_env = getenv("PATH");
    if (path_env == NULL || strlen(path_env) == 0) {
        return NULL;
    }
    
    // 手动解析PATH，避免strtok_r在fork后的问题
    char full_path[1024];
    const char *p = path_env;
    
    while (*p != '\0') {
        // 找到下一个冒号或字符串结尾
        const char *end = strchr(p, ':');
        if (end == NULL) {
            end = p + strlen(p);
        }
        
        // 提取目录路径
        size_t dir_len = end - p;
        if (dir_len > 0 && dir_len < sizeof(full_path) - strlen(cmd_name) - 2) {
            strncpy(full_path, p, dir_len);
            full_path[dir_len] = '\0';
            
            // 构建完整路径
            if (full_path[dir_len - 1] != '/') {
                strcat(full_path, "/");
            }
            strcat(full_path, cmd_name);
            
            // 检查文件是否存在且可执行
            if (access(full_path, X_OK) == 0) {
                return strdup(full_path);
            }
        }
        
        // 移动到下一个路径
        if (*end == '\0') {
            break;
        }
        p = end + 1;
    }
    
    return NULL;
}

// 检查是否有任何重定向
static int has_redirect(Command *cmd) {
    return (cmd->stdout_file != NULL || cmd->stderr_file != NULL || cmd->stdin_file != NULL);
}

// 设置多重定向
static int setup_redirect(Command *cmd) {
    int fd = -1;
    
    // 设置标准输出重定向
    if (cmd->stdout_file != NULL) {
        int flags = O_CREAT | O_WRONLY;
        flags |= cmd->stdout_append ? O_APPEND : O_TRUNC;
        
        fd = open(cmd->stdout_file, flags, 0644);
        if (fd < 0) {
            perror("open stdout");
            return -1;
        }
        
        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // 设置错误输出重定向
    if (cmd->stderr_file != NULL) {
        int flags = O_CREAT | O_WRONLY;
        flags |= cmd->stderr_append ? O_APPEND : O_TRUNC;
        
        fd = open(cmd->stderr_file, flags, 0644);
        if (fd < 0) {
            perror("open stderr");
            return -1;
        }
        
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    // 设置输入重定向
    if (cmd->stdin_file != NULL) {
        fd = open(cmd->stdin_file, O_RDONLY);
        if (fd < 0) {
            perror("open stdin");
            return -1;
        }
        
        if (dup2(fd, STDIN_FILENO) < 0) {
            perror("dup2 stdin");
            close(fd);
            return -1;
        }
        close(fd);
    }
    
    return 0;
}

// 执行外部命令
static int execute_external(Command *cmd, ShellContext *ctx) {
    // 避免编译器警告
    (void)ctx;
    
    // 查找可执行文件
    char *exec_path = find_executable(cmd->name);
    if (exec_path == NULL) {
        fprintf(stderr, "%s: command not found\n", cmd->name);
        // 记录错误到日志
        extern void log_error(ShellContext *ctx, const char *format, ...);
        log_error(ctx, "Command not found: %s", cmd->name);
        return -1;
    }
    
    // fork子进程
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        // 记录错误到日志
        extern void log_error(ShellContext *ctx, const char *format, ...);
        log_error(ctx, "fork failed: %s", strerror(errno));
        free(exec_path);
        return -1;
    }
    
    if (pid == 0) {
        // 子进程
        // 设置重定向
        if (setup_redirect(cmd) != 0) {
            free(exec_path);
            exit(1);
        }
        
        // 验证args数组
        if (cmd->args == NULL) {
            fprintf(stderr, "execv: args is NULL\n");
            free(exec_path);
            exit(1);
        }
        
        // 确保args数组以NULL结尾（parse_command应该已经设置了，这里只是双重检查）
        // 实际上parse_command已经设置了cmd->args[cmd->arg_count] = NULL
        
        // 执行命令
        execv(exec_path, cmd->args);
        
        // 如果execv返回，说明执行失败
        perror("execv");
        // 注意：在子进程中无法访问 ctx，所以不记录日志
        free(exec_path);
        exit(1);
    } else {
        // 父进程
        free(exec_path);
        
        // 检查是否后台执行
        if (cmd->background) {
            // 后台执行：不等待子进程，添加到作业列表
            // 重建命令字符串
            char cmd_str[256] = "";
            for (int i = 0; i < cmd->arg_count && strlen(cmd_str) < 240; i++) {
                if (i > 0) strcat(cmd_str, " ");
                strncat(cmd_str, cmd->args[i], 240 - strlen(cmd_str));
            }
            
            int job_id = job_add(pid, cmd_str);
            printf("[%d] %d\n", job_id, pid);
            return 0;
        } else {
            // 前台执行：等待子进程
            int status;
            waitpid(pid, &status, 0);
            
            if (WIFEXITED(status)) {
                return WEXITSTATUS(status);
            } else {
                return -1;
            }
        }
    }
}

// 执行管道命令链
static int execute_pipeline(Command *cmd, ShellContext *ctx) {
    Command *current = cmd;
    int pipe_count = 0;
    
    // 计算管道数量
    while (current != NULL) {
        pipe_count++;
        current = current->pipe_next;
    }
    
    if (pipe_count == 0) {
        return -1;
    }
    
    if (pipe_count > 100) {
        fprintf(stderr, "pipeline: too many commands\n");
        return -1;
    }
    
    // 创建管道（使用固定大小数组，避免VLA问题）
    int pipes[100][2];
    for (int i = 0; i < pipe_count - 1; i++) {
        if (pipe(pipes[i]) < 0) {
            perror("pipe");
            return -1;
        }
    }
    
    // 执行每个命令
    current = cmd;
    pid_t pids[100];
    int cmd_index = 0;
    
    while (current != NULL) {
        pid_t pid = fork();
        if (pid < 0) {
            perror("fork");
            // 关闭所有管道
            for (int i = 0; i < pipe_count - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            return -1;
        }
        
        if (pid == 0) {
            // 子进程
            // 设置输入管道（不是第一个命令）
            if (cmd_index > 0) {
                dup2(pipes[cmd_index - 1][0], STDIN_FILENO);
            }
            
            // 设置输出管道（不是最后一个命令）
            if (cmd_index < pipe_count - 1) {
                dup2(pipes[cmd_index][1], STDOUT_FILENO);
            }
            
            // 关闭所有管道
            for (int i = 0; i < pipe_count - 1; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
            }
            
            // 设置重定向（最后一个命令可能有重定向）
            if (cmd_index == pipe_count - 1) {
                setup_redirect(current);
            }
            
            // 执行命令
            int result;
            if (is_builtin(current->name)) {
                result = execute_builtin(current, ctx);
                exit(result);
            } else {
                char *exec_path = find_executable(current->name);
                if (exec_path == NULL) {
                    fprintf(stderr, "%s: command not found\n", current->name);
                    exit(1);
                }
                execv(exec_path, current->args);
                perror("execv");
                free(exec_path);
                exit(1);
            }
        } else {
            // 父进程
            pids[cmd_index] = pid;
        }
        
        current = current->pipe_next;
        cmd_index++;
    }
    
    // 关闭所有管道（父进程）
    for (int i = 0; i < pipe_count - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }
    
    // 等待所有子进程
    int last_status = 0;
    for (int i = 0; i < pipe_count; i++) {
        int status;
        waitpid(pids[i], &status, 0);
        if (i == pipe_count - 1) {
            if (WIFEXITED(status)) {
                last_status = WEXITSTATUS(status);
            } else {
                last_status = -1;
            }
        }
    }
    
    return last_status;
}

// 执行单个命令（不处理命令链）
static int execute_single_command(Command *cmd, ShellContext *ctx) {
    // 步骤1：参数有效性检查
    if (cmd == NULL || cmd->name == NULL) {
        return -1;
    }

    // 步骤2：特殊处理 quit 命令
    if (strcmp(cmd->name, "quit") == 0) {
        return cmd_quit(cmd, ctx);
    }

    // 步骤2.5：展开大括号表达式（仅对参数展开，不包括命令名）
    char **expanded_args = NULL;
    int expanded_arg_count = 0;
    Command expanded_cmd = *cmd; // 复制命令结构体
    
    if (cmd->arg_count > 1) {
        // 展开参数（跳过命令名，从 args[1] 开始）
        char **args_to_expand = cmd->args + 1; // 跳过命令名
        int args_to_expand_count = cmd->arg_count - 1;
        
        expanded_args = expand_args(args_to_expand, args_to_expand_count);
        if (expanded_args != NULL) {
            // 计算展开后的参数数量
            expanded_arg_count = 0;
            while (expanded_args[expanded_arg_count] != NULL) {
                expanded_arg_count++;
            }
            
            // 构建新的参数数组（命令名 + 展开后的参数）
            char **new_args = malloc((expanded_arg_count + 2) * sizeof(char*));
            if (new_args != NULL) {
                new_args[0] = cmd->name; // 命令名
                for (int i = 0; i < expanded_arg_count; i++) {
                    new_args[i + 1] = expanded_args[i];
                }
                new_args[expanded_arg_count + 1] = NULL;
                
                expanded_cmd.args = new_args;
                expanded_cmd.arg_count = expanded_arg_count + 1;
                cmd = &expanded_cmd; // 使用展开后的命令
            } else {
                // 内存分配失败，释放已展开的参数，使用原命令
                for (int i = 0; expanded_args[i] != NULL; i++) {
                    free(expanded_args[i]);
                }
                free(expanded_args);
                expanded_args = NULL;
            }
        }
    }

    // 步骤3：检查是否有管道
    if (cmd->pipe_next != NULL) {
        int result = execute_pipeline(cmd, ctx);
        // 清理展开的参数
        if (expanded_args != NULL) {
            if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
                free(expanded_cmd.args);
            }
            for (int i = 0; expanded_args[i] != NULL; i++) {
                free(expanded_args[i]);
            }
            free(expanded_args);
        }
        return result;
    }

    // 步骤4：检查是否为内置命令
    if (is_builtin(cmd->name)) {
        // 内置命令需要处理重定向
        if (has_redirect(cmd)) {
            // 对于内置命令，我们需要在子进程中执行以支持重定向
            pid_t pid = fork();
            if (pid < 0) {
                perror("fork");
                // 清理展开的参数
                if (expanded_args != NULL) {
                    if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
                        free(expanded_cmd.args);
                    }
                    for (int i = 0; expanded_args[i] != NULL; i++) {
                        free(expanded_args[i]);
                    }
                    free(expanded_args);
                }
                return -1;
            }
            
            if (pid == 0) {
                // 子进程：设置重定向并执行
                if (setup_redirect(cmd) != 0) {
                    exit(1);
                }
                int result = execute_builtin(cmd, ctx);
                // 子进程退出前清理内存（虽然进程退出会自动释放，但为了代码一致性）
                if (expanded_args != NULL) {
                    if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
                        free(expanded_cmd.args);
                    }
                    for (int i = 0; expanded_args[i] != NULL; i++) {
                        free(expanded_args[i]);
                    }
                    free(expanded_args);
                }
                exit(result);
            } else {
                // 父进程：等待子进程
                int status;
                waitpid(pid, &status, 0);
                // 清理展开的参数
                if (expanded_args != NULL) {
                    if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
                        free(expanded_cmd.args);
                    }
                    for (int i = 0; expanded_args[i] != NULL; i++) {
                        free(expanded_args[i]);
                    }
                    free(expanded_args);
                }
                if (WIFEXITED(status)) {
                    return WEXITSTATUS(status);
                } else {
                    return -1;
                }
            }
        } else {
            int result = execute_builtin(cmd, ctx);
            // 清理展开的参数
            if (expanded_args != NULL) {
                if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
                    free(expanded_cmd.args);
                }
                for (int i = 0; expanded_args[i] != NULL; i++) {
                    free(expanded_args[i]);
                }
                free(expanded_args);
            }
            return result;
        }
    }
    
    // 步骤5：执行外部命令
    int result = execute_external(cmd, ctx);
    // 清理展开的参数
    if (expanded_args != NULL) {
        if (expanded_cmd.args != NULL && expanded_cmd.args != cmd->args) {
            free(expanded_cmd.args);
        }
        for (int i = 0; expanded_args[i] != NULL; i++) {
            free(expanded_args[i]);
        }
        free(expanded_args);
    }
    return result;
}

// 命令执行主函数
// 功能：这是命令执行的总入口，负责分发命令到不同的处理函数
// 支持重定向、管道、外部命令执行和命令链（&& 和 ||）
int execute_command(Command *cmd, ShellContext *ctx) {
    if (cmd == NULL) {
        return -1;
    }
    
    int last_status = 0;
    Command *current = cmd;
    
    while (current != NULL) {
        // 执行当前命令
        int status = execute_single_command(current, ctx);
        
        // 检查是否有命令链
        if (current->chain_next != NULL) {
            // 根据链类型决定是否执行下一个命令
            if (current->chain_type == 1) {
                // &&：只有当前命令成功（status == 0）时才执行下一个
                if (status != 0) {
                    // 命令失败，不执行后续命令
                    break;
                }
            } else if (current->chain_type == 2) {
                // ||：只有当前命令失败（status != 0）时才执行下一个
                if (status == 0) {
                    // 命令成功，不执行后续命令
                    break;
                }
            }
        }
        
        last_status = status;
        current = current->chain_next;
    }
    
    return last_status;
}

// 内置命令判断函数
// 功能：检查给定的命令名是否在内置命令列表中
// 用途：在执行命令前，需要先判断是调用内置函数还是fork + exec 外部程序
int is_builtin(const char *cmd_name) {
    // 步骤1：参数检查：防止空指针访问
    if(cmd_name == NULL) {                                      // 如果命令名为空
        return 0;                                               // 返回0表示不是内置命令    
    }

    // 步骤2：定义内置命令列表（字符串数组）
    // 注意：所有内置命令都要加 x 前缀（除了quit）
    const char *builtins[] = {
        "xpwd",                                                 // 显示当前工作目录（对应系统的pwd）
        "xcd",                                                  // 切换工作目录（对应系统的cd）
        "xls",                                                  // 列出文件和目录（对应系统的ls）
        "xecho",                                                // 输出字符串（对应系统的echo）
        "xtouch",                                               // 创建文件或更新时间戳（对应系统的touch）
        "xcat",                                                 // 显示文件内容（对应系统的cat）
        "xrm",                                                  // 删除文件或目录（对应系统的rm）
        "xcp",                                                  // 复制文件或目录（对应系统的cp）
        "xmv",                                                  // 移动或重命名文件/目录（对应系统的mv）
        "xhistory",                                             // 显示命令历史记录（对应系统的history）
        "xtec",                                                 // 从标准输入读取并输出到文件和标准输出（对应系统的tee）
        "xmkdir",                                               // 创建目录（对应系统的mkdir）
        "xrmdir",                                               // 删除空目录（对应系统的rmdir）
        "xln",                                                  // 创建链接（对应系统的ln）
        "xchmod",                                               // 修改文件权限（对应系统的chmod）
        "xchown",                                               // 修改文件所有者（对应系统的chown）
        "xfind",                                                // 查找文件（对应系统的find）
        "xuname",                                               // 显示系统信息（对应系统的uname）
        "xhostname",                                            // 显示主机名（对应系统的hostname）
        "xwhoami",                                              // 显示当前用户（对应系统的whoami）
        "xdate",                                                // 显示日期时间（对应系统的date）
        "xuptime",                                              // 显示系统运行时间（对应系统的uptime）
        "xps",                                                  // 显示进程信息（对应系统的ps）
        "xbasename",                                            // 提取文件名（对应系统的basename）
        "xdirname",                                             // 提取目录名（对应系统的dirname）
        "xreadlink",                                            // 读取符号链接（对应系统的readlink）
        "xcut",                                                 // 提取列（对应系统的cut）
        "xpaste",                                               // 合并文件行（对应系统的paste）
        "xtr",                                                  // 字符转换（对应系统的tr）
        "xcomm",                                                // 比较排序文件（对应系统的comm）
        "xstat",                                                // 显示文件详细信息（对应系统的stat）
        "xfile",                                                // 显示文件类型（对应系统的file）
        "xdu",                                                  // 显示目录大小（对应系统的du）
        "xdf",                                                  // 显示磁盘空间（对应系统的df）
        "xsplit",                                               // 分割文件（对应系统的split）
        "xjoin",                                                // 连接文件（对应系统的join）
        "xrealpath",                                            // 显示绝对路径（对应系统的realpath）
        "xmenu",                                                // 交互式菜单系统（XShell 特有功能）
        "xdiff",                                                // 比较文件差异（对应系统的diff）
        "xgrep",                                                // 在文件中搜索文本（对应系统的grep）
        "xwc",                                                  // 统计行数/字数/字节数（对应系统的wc）
        "xhead",                                                // 显示文件前N行（对应系统的head）
        "xtail",                                                // 显示文件后N行（对应系统的tail）
        "xsort",                                                // 排序文件内容（对应系统的sort）
        "xuniq",                                                // 去除重复行（对应系统的uniq）
        "xenv",                                                 // 显示所有环境变量（对应系统的env）
        "xexport",                                              // 设置环境变量（对应系统的export）
        "xunset",                                               // 删除环境变量（对应系统的unset）
        "xalias",                                               // 设置命令别名（对应系统的alias）
        "xunalias",                                             // 删除命令别名（对应系统的unalias）
        "xclear",                                               // 清屏（对应系统的clear）
        "xhelp",                                                // 显示帮助信息（对应系统的help）
        "xtype",                                                // 显示命令类型（对应系统的type）
        "xwhich",                                               // 显示命令路径（对应系统的which）
        "xsleep",                                               // 休眠指定秒数（对应系统的sleep）
        "xcalc",                                                // 简单计算器（对应系统的bc/expr）
        "xtree",                                                // 树形显示目录结构（对应系统的tree）
        "xsource",                                              // 执行脚本文件（对应系统的source）
        "xtime",                                                // 测量命令执行时间（对应系统的time）
        "xkill",                                                // 终止进程（对应系统的kill）
        "xjobs",                                                // 显示后台任务（对应系统的jobs）
        "xfg",                                                  // 将后台任务调到前台（对应系统的fg）
        "xbg",                                                  // 将任务放到后台（对应系统的bg）
        "xui",                                                  // 终端 UI 界面（XShell 特有功能）
        "xweb",                                                 // 网页浏览器（XShell 特有功能）
        "xsnake",                                               // 贪吃蛇游戏（XShell 特有功能）
        "xtetris",                                              // 俄罗斯方块（XShell 特有功能）
        "x2048",                                                // 2048游戏（XShell 特有功能）
        "xsysmon",                                              // 系统监控（XShell 特有功能）
        "quit",                                                 // 退出Shell（Shell 专有命令，不加x）
        NULL                                                    // 数组结束标记（用于判断遍历结束）
    };

    // 步骤3：遍历内置命令列表，查找匹配项
    for (int i = 0; builtins[i] != NULL; i++) {                 // 循环知道遇到NULL
        // 使用strcmp 比较字符串（相等返回0）
        if (strcmp(cmd_name, builtins[i]) == 0) {               // 找到匹配的命令
            return 1;                                           // 返回1表示是内置命令    
        }    
    }

    // 步骤4：未找到匹配：说明不是内置命令
    return 0;                                                   // 返回 0 表示不是内置命令（可能是外部命令）
}

// 内置命令执行函数
// 功能：根据命令名称，分发到对应的内置命令处理函数
// 设计模式：简单的if-else练（命令少时够用，命令多时改用函数指标表）
int execute_builtin(Command *cmd, ShellContext *ctx) {
    // 步骤1：参数检查：确保命令对象有效
    if (cmd == NULL || cmd->name == NULL) {                     // 空指针检查
        return -1;                                              // 无效参数，返回失败
    }

    // 步骤2：命令发放：根据命令名称调用相应的处理函数
    // 使用if-else 链进行字符串匹配（简单直观）

    if (strcmp(cmd->name, "xpwd") == 0) {                       // 匹配 xpwd 命令
        return cmd_xpwd(cmd, ctx);                              // 调用 xpwd 处理函数（显示当前目录）
    }
    else if (strcmp(cmd->name, "xcd") == 0) {                   // 匹配 xcd 命令
        return cmd_xcd(cmd, ctx);                               // 调用 xcd 处理函数（切换目录）
    }
    else if (strcmp(cmd->name, "xls") == 0) {                   // 匹配 xls 命令
        return cmd_xls(cmd, ctx);                               // 调用 xls 处理函数（列出文件）
    }
    else if (strcmp(cmd->name, "xecho") == 0) {                 // 匹配 xecho 命令
        return cmd_xecho(cmd, ctx);                             // 调用 xecho 处理函数（输出字符串）
    }
    else if (strcmp(cmd->name, "xtouch") == 0) {                // 匹配 xtouch 命令
        return cmd_xtouch(cmd, ctx);                            // 调用 xtouch 处理函数（创建文件或更新时间戳）
    }
    else if (strcmp(cmd->name, "xcat") == 0) {                  // 匹配 xcat 命令
        return cmd_xcat(cmd, ctx);                              // 调用 xcat 处理函数（显示文件内容）
    }
    else if (strcmp(cmd->name, "xrm") == 0) {                   // 匹配 xrm 命令
        return cmd_xrm(cmd, ctx);                               // 调用 xrm 处理函数（删除文件或目录）
    }
    else if (strcmp(cmd->name, "xcp") == 0) {                   // 匹配 xcp 命令
        return cmd_xcp(cmd, ctx);                               // 调用 xcp 处理函数（复制文件或目录）
    }
    else if (strcmp(cmd->name, "xmv") == 0) {                   // 匹配 xmv 命令
        return cmd_xmv(cmd, ctx);                               // 调用 xmv 处理函数（移动或重命名文件/目录）
    }
    else if (strcmp(cmd->name, "xhistory") == 0) {              // 匹配 xhistory 命令
        return cmd_xhistory(cmd, ctx);                          // 调用 xhistory 处理函数（显示命令历史）
    }
    else if (strcmp(cmd->name, "xtec") == 0) {                  // 匹配 xtec 命令
        return cmd_xtec(cmd, ctx);                              // 调用 xtec 处理函数（tee 功能）
    }
    else if (strcmp(cmd->name, "xmkdir") == 0) {                // 匹配 xmkdir 命令
        return cmd_xmkdir(cmd, ctx);                            // 调用 xmkdir 处理函数（创建目录）
    }
    else if (strcmp(cmd->name, "xrmdir") == 0) {                // 匹配 xrmdir 命令
        return cmd_xrmdir(cmd, ctx);                            // 调用 xrmdir 处理函数（删除空目录）
    }
    else if (strcmp(cmd->name, "xln") == 0) {                   // 匹配 xln 命令
        return cmd_xln(cmd, ctx);                               // 调用 xln 处理函数（创建链接）
    }
    else if (strcmp(cmd->name, "xchmod") == 0) {                // 匹配 xchmod 命令
        return cmd_xchmod(cmd, ctx);                            // 调用 xchmod 处理函数（修改权限）
    }
    else if (strcmp(cmd->name, "xchown") == 0) {                // 匹配 xchown 命令
        return cmd_xchown(cmd, ctx);                            // 调用 xchown 处理函数（修改所有者）
    }
    else if (strcmp(cmd->name, "xfind") == 0) {                 // 匹配 xfind 命令
        return cmd_xfind(cmd, ctx);                             // 调用 xfind 处理函数（查找文件）
    }
    else if (strcmp(cmd->name, "xuname") == 0) {                // 匹配 xuname 命令
        return cmd_xuname(cmd, ctx);                            // 调用 xuname 处理函数（显示系统信息）
    }
    else if (strcmp(cmd->name, "xhostname") == 0) {             // 匹配 xhostname 命令
        return cmd_xhostname(cmd, ctx);                         // 调用 xhostname 处理函数（显示主机名）
    }
    else if (strcmp(cmd->name, "xwhoami") == 0) {               // 匹配 xwhoami 命令
        return cmd_xwhoami(cmd, ctx);                           // 调用 xwhoami 处理函数（显示当前用户）
    }
    else if (strcmp(cmd->name, "xdate") == 0) {                 // 匹配 xdate 命令
        return cmd_xdate(cmd, ctx);                             // 调用 xdate 处理函数（显示日期时间）
    }
    else if (strcmp(cmd->name, "xuptime") == 0) {               // 匹配 xuptime 命令
        return cmd_xuptime(cmd, ctx);                           // 调用 xuptime 处理函数（显示系统运行时间）
    }
    else if (strcmp(cmd->name, "xps") == 0) {                   // 匹配 xps 命令
        return cmd_xps(cmd, ctx);                               // 调用 xps 处理函数（显示进程信息）
    }
    else if (strcmp(cmd->name, "xbasename") == 0) {            // 匹配 xbasename 命令
        return cmd_xbasename(cmd, ctx);                         // 调用 xbasename 处理函数（提取文件名）
    }
    else if (strcmp(cmd->name, "xdirname") == 0) {             // 匹配 xdirname 命令
        return cmd_xdirname(cmd, ctx);                          // 调用 xdirname 处理函数（提取目录名）
    }
    else if (strcmp(cmd->name, "xreadlink") == 0) {             // 匹配 xreadlink 命令
        return cmd_xreadlink(cmd, ctx);                         // 调用 xreadlink 处理函数（读取符号链接）
    }
    else if (strcmp(cmd->name, "xcut") == 0) {                  // 匹配 xcut 命令
        return cmd_xcut(cmd, ctx);                              // 调用 xcut 处理函数（提取列）
    }
    else if (strcmp(cmd->name, "xpaste") == 0) {                // 匹配 xpaste 命令
        return cmd_xpaste(cmd, ctx);                            // 调用 xpaste 处理函数（合并文件行）
    }
    else if (strcmp(cmd->name, "xtr") == 0) {                   // 匹配 xtr 命令
        return cmd_xtr(cmd, ctx);                               // 调用 xtr 处理函数（字符转换）
    }
    else if (strcmp(cmd->name, "xcomm") == 0) {                 // 匹配 xcomm 命令
        return cmd_xcomm(cmd, ctx);                             // 调用 xcomm 处理函数（比较排序文件）
    }
    else if (strcmp(cmd->name, "xstat") == 0) {                 // 匹配 xstat 命令
        return cmd_xstat(cmd, ctx);                             // 调用 xstat 处理函数（显示文件详细信息）
    }
    else if (strcmp(cmd->name, "xfile") == 0) {                 // 匹配 xfile 命令
        return cmd_xfile(cmd, ctx);                             // 调用 xfile 处理函数（显示文件类型）
    }
    else if (strcmp(cmd->name, "xdu") == 0) {                   // 匹配 xdu 命令
        return cmd_xdu(cmd, ctx);                               // 调用 xdu 处理函数（显示目录大小）
    }
    else if (strcmp(cmd->name, "xdf") == 0) {                   // 匹配 xdf 命令
        return cmd_xdf(cmd, ctx);                               // 调用 xdf 处理函数（显示磁盘空间）
    }
    else if (strcmp(cmd->name, "xsplit") == 0) {                // 匹配 xsplit 命令
        return cmd_xsplit(cmd, ctx);                            // 调用 xsplit 处理函数（分割文件）
    }
    else if (strcmp(cmd->name, "xjoin") == 0) {                 // 匹配 xjoin 命令
        return cmd_xjoin(cmd, ctx);                             // 调用 xjoin 处理函数（连接文件）
    }
    else if (strcmp(cmd->name, "xrealpath") == 0) {            // 匹配 xrealpath 命令
        return cmd_xrealpath(cmd, ctx);                         // 调用 xrealpath 处理函数（显示绝对路径）
    }
    else if (strcmp(cmd->name, "xmenu") == 0) {                // 匹配 xmenu 命令
        return cmd_xmenu(cmd, ctx);                             // 调用 xmenu 处理函数（交互式菜单）
    }
    else if (strcmp(cmd->name, "xdiff") == 0) {                // 匹配 xdiff 命令
        return cmd_xdiff(cmd, ctx);                             // 调用 xdiff 处理函数（比较文件差异）
    }
    else if (strcmp(cmd->name, "xgrep") == 0) {                 // 匹配 xgrep 命令
        return cmd_xgrep(cmd, ctx);                             // 调用 xgrep 处理函数（在文件中搜索文本）
    }
    else if (strcmp(cmd->name, "xwc") == 0) {                   // 匹配 xwc 命令
        return cmd_xwc(cmd, ctx);                               // 调用 xwc 处理函数（统计行数/字数/字节数）
    }
    else if (strcmp(cmd->name, "xhead") == 0) {                 // 匹配 xhead 命令
        return cmd_xhead(cmd, ctx);                             // 调用 xhead 处理函数（显示文件前N行）
    }
    else if (strcmp(cmd->name, "xtail") == 0) {                 // 匹配 xtail 命令
        return cmd_xtail(cmd, ctx);                             // 调用 xtail 处理函数（显示文件后N行）
    }
    else if (strcmp(cmd->name, "xsort") == 0) {                 // 匹配 xsort 命令
        return cmd_xsort(cmd, ctx);                             // 调用 xsort 处理函数（排序文件内容）
    }
    else if (strcmp(cmd->name, "xuniq") == 0) {                 // 匹配 xuniq 命令
        return cmd_xuniq(cmd, ctx);                             // 调用 xuniq 处理函数（去除重复行）
    }
    else if (strcmp(cmd->name, "xenv") == 0) {                  // 匹配 xenv 命令
        return cmd_xenv(cmd, ctx);                              // 调用 xenv 处理函数（显示所有环境变量）
    }
    else if (strcmp(cmd->name, "xexport") == 0) {               // 匹配 xexport 命令
        return cmd_xexport(cmd, ctx);                           // 调用 xexport 处理函数（设置环境变量）
    }
    else if (strcmp(cmd->name, "xunset") == 0) {                // 匹配 xunset 命令
        return cmd_xunset(cmd, ctx);                            // 调用 xunset 处理函数（删除环境变量）
    }
    else if (strcmp(cmd->name, "xalias") == 0) {                // 匹配 xalias 命令
        return cmd_xalias(cmd, ctx);                            // 调用 xalias 处理函数（设置/显示别名）
    }
    else if (strcmp(cmd->name, "xunalias") == 0) {              // 匹配 xunalias 命令
        return cmd_xunalias(cmd, ctx);                          // 调用 xunalias 处理函数（删除别名）
    }
    else if (strcmp(cmd->name, "xclear") == 0) {                // 匹配 xclear 命令
        return cmd_xclear(cmd, ctx);                            // 调用 xclear 处理函数（清屏）
    }
    else if (strcmp(cmd->name, "xhelp") == 0) {                 // 匹配 xhelp 命令
        return cmd_xhelp(cmd, ctx);                             // 调用 xhelp 处理函数（显示帮助信息）
    }
    else if (strcmp(cmd->name, "xtype") == 0) {                 // 匹配 xtype 命令
        return cmd_xtype(cmd, ctx);                             // 调用 xtype 处理函数（显示命令类型）
    }
    else if (strcmp(cmd->name, "xwhich") == 0) {                // 匹配 xwhich 命令
        return cmd_xwhich(cmd, ctx);                            // 调用 xwhich 处理函数（显示命令路径）
    }
    else if (strcmp(cmd->name, "xsleep") == 0) {                // 匹配 xsleep 命令
        return cmd_xsleep(cmd, ctx);                            // 调用 xsleep 处理函数（休眠）
    }
    else if (strcmp(cmd->name, "xcalc") == 0) {                 // 匹配 xcalc 命令
        return cmd_xcalc(cmd, ctx);                             // 调用 xcalc 处理函数（计算器）
    }
    else if (strcmp(cmd->name, "xtree") == 0) {                 // 匹配 xtree 命令
        return cmd_xtree(cmd, ctx);                             // 调用 xtree 处理函数（树形显示目录）
    }
    else if (strcmp(cmd->name, "xsource") == 0) {              // 匹配 xsource 命令
        return cmd_xsource(cmd, ctx);                          // 调用 xsource 处理函数（执行脚本文件）
    }
    else if (strcmp(cmd->name, "xtime") == 0) {                 // 匹配 xtime 命令
        return cmd_xtime(cmd, ctx);                            // 调用 xtime 处理函数（测量执行时间）
    }
    else if (strcmp(cmd->name, "xkill") == 0) {                // 匹配 xkill 命令
        return cmd_xkill(cmd, ctx);                            // 调用 xkill 处理函数（终止进程）
    }
    else if (strcmp(cmd->name, "xjobs") == 0) {                // 匹配 xjobs 命令
        return cmd_xjobs(cmd, ctx);                            // 调用 xjobs 处理函数（显示后台任务）
    }
    else if (strcmp(cmd->name, "xfg") == 0) {                  // 匹配 xfg 命令
        return cmd_xfg(cmd, ctx);                              // 调用 xfg 处理函数（将任务调到前台）
    }
    else if (strcmp(cmd->name, "xbg") == 0) {                  // 匹配 xbg 命令
        return cmd_xbg(cmd, ctx);                              // 调用 xbg 处理函数（将任务放到后台）
    }
    else if (strcmp(cmd->name, "xui") == 0) {                  // 匹配 xui 命令
        return cmd_xui(cmd, ctx);                              // 调用 xui 处理函数（终端 UI）
    }
    else if (strcmp(cmd->name, "xweb") == 0) {                 // 匹配 xweb 命令
        return cmd_xweb(cmd, ctx);                             // 调用 xweb 处理函数（网页浏览器）
    }
    else if (strcmp(cmd->name, "xsnake") == 0) {               // 匹配 xsnake 命令
        return cmd_xsnake(cmd, ctx);                           // 调用 xsnake 处理函数（贪吃蛇游戏）
    }
    else if (strcmp(cmd->name, "xtetris") == 0) {
        return cmd_xtetris(cmd, ctx);
    }
    else if (strcmp(cmd->name, "x2048") == 0) {
        return cmd_x2048(cmd, ctx);
    }
    else if (strcmp(cmd->name, "xsysmon") == 0) {
        return cmd_xsysmon(cmd, ctx);
    }
    else if (strcmp(cmd->name, "quit") == 0) {                  // 匹配 quit 命令
        return cmd_quit(cmd, ctx);                              // 调用 quit 处理函数（设置退出标志）
    }

    // 步骤3：命令未实现：如果到这里，说明命令在列表中但为未实现
    // 这种情况通常是开发中途（已加入builtins[] 但未写处理函数）
    fprintf(stderr, "%s: builtin command not implemented\n", cmd->name);
    return -1;                                                  // 返回失败状态
}