// 定义 POSIX 标准版本，启用 strdup 等函数
#define _POSIX_C_SOURCE 200809L

// 引入自定义头文件
#include "xshell.h"      // Shell 核心定义（ShellContext 结构体、常量等）
#include "parser.h"      // 命令解析器（parse_command, free_command）
#include "executor.h"    // 命令执行器（execute_command）
#include "utils.h"       // 工具函数（is_empty_line, trim）
#include "input.h"       // 输入处理（带 Tab 补全）
#include "history.h"     // 历史记录系统（history_init, history_add, history_cleanup）
#include "alias.h"       // 别名管理系统（alias_init, alias_cleanup）
// 引入标准库
#include <stdio.h>       // 标准输入输出（printf, fprintf, fgets, va_list）
#include <stdlib.h>      // 标准库函数（getenv）
#include <string.h>      // 字符串处理（strcpy, strcspn, strlen）
#include <stdarg.h>      // 可变参数（va_list, va_start, va_end）
#include <time.h>        // 时间函数（time, localtime, strftime）
#include <errno.h>       // 错误码（errno）
#include <sys/types.h>   // PID 类型定义

// 全局变量和回调函数
// 全局指针：用于提示符回调函数访问 Shell 上下文
// 说明：因为回调函数签名不能传递自定义参数，所以使用全局变量
// 用途：在 Tab 补全显示选项后，回调函数需要访问上下文来显示提示符
static ShellContext *g_shell_ctx = NULL;               // 全局 Shell 上下文指针

// 提示符回调函数（用于 Tab 补全后重新显示提示符）
// 功能：当用户按两次 Tab 显示所有匹配项后，调用此函数重新显示提示符
// 使用场景：
//   用户操作：[\home]# xcd l<Tab><Tab>
//   系统显示：lab/  lost+found/
//   此函数显示：[\home]# xcd l       <- 重新显示提示符和当前输入
// 注意：此函数通过全局变量 g_shell_ctx 访问 Shell 上下文
static void prompt_callback(void) {
    if (g_shell_ctx != NULL) {                          // 检查全局指针是否有效
        display_prompt(g_shell_ctx);                    // 调用提示符显示函数
    }
}

// 初始化 Shell 环境
int init_shell(ShellContext *ctx) {
    // 获取当前工作目录并存储到 ctx->cwd
    if (getcwd(ctx->cwd, sizeof(ctx->cwd)) == NULL) {  // getcwd 失败返回 NULL
        perror("getcwd");  // 打印系统错误信息到 stderr
        return -1;         // 初始化失败，返回 -1
    }
    
    // 初始化上一个目录为当前目录（首次启动时没有"上一个"目录）
    strcpy(ctx->prev_dir, ctx->cwd);
    
    // 获取用户主目录（从环境变量 HOME）
    ctx->home_dir = getenv("HOME");  // getenv 返回环境变量的值
    if (ctx->home_dir == NULL) {     // 如果 HOME 环境变量不存在
        ctx->home_dir = "/tmp";      // 使用默认目录 /tmp
    }
    
    // 初始化日志文件
    // 注意：日志文件名为 .xshell_error（符合 xshell_implementation_plan.md 要求）
    ctx->log_file = fopen(".xshell_error", "a");  // 以追加模式打开日志文件
    if (ctx->log_file == NULL) {
        // 如果无法打开日志文件，输出警告但不影响 Shell 运行
        fprintf(stderr, "Warning: Cannot open log file .xshell_error\n");
        ctx->log_file = NULL;  // 设置为 NULL，日志功能将不工作
    }
    
    // 设置 Shell 初始状态
    ctx->running = 1;           // 设置运行标志为 1（表示 Shell 正在运行）
    ctx->last_exit_status = 0;  // 上一条命令退出状态初始化为 0（成功）
    
    return 0;  // 初始化成功，返回 0
}

// Shell 主循环（核心逻辑）
void shell_loop(ShellContext *ctx) {
    char line[MAX_INPUT_LENGTH];  // 声明字符数组，存储用户输入的命令行（最大 4096 字节）
    
    // 设置全局上下文指针（用于提示符回调）
    // 说明：将当前 Shell 上下文保存到全局变量，以便回调函数可以访问
    // 用途：Tab 补全显示选项后，回调函数需要此指针来显示提示符
    g_shell_ctx = ctx;
    
    // 初始化历史记录系统
    // 功能：加载保存的历史记录（从 ~/.xshell_history）
    history_init();
    
    // 初始化别名系统
    // 功能：初始化别名表（清空所有别名）
    alias_init();
    
    // 显示欢迎信息（Shell 启动时打印一次）
    printf("######## Welcome to XShell! ########\n");
    
    // 主循环：只要 ctx->running 为 1（真），就持续执行
    while (ctx->running) {
        // 显示命令提示符（如 [\home\user\]#）
        display_prompt(ctx);
        
        // 从标准输入读取一行用户输入（支持 Tab 补全）
        // 功能说明：
        //   1. 逐字符读取（不等待回车）
        //   2. Tab 键触发路径补全
        //   3. 双击 Tab 显示所有匹配项
        //   4. 退格键删除字符
        //   5. Ctrl+D 退出
        // 参数说明：
        //   - line: 存储输入的缓冲区
        //   - sizeof(line): 缓冲区大小
        //   - prompt_callback: 提示符回调函数（显示匹配项后重新显示提示符）
        if (read_line_with_completion(line, sizeof(line), prompt_callback) == NULL) {
            printf("\n");  // 打印换行符（美化输出）
            break;         // 跳出循环，Shell 将退出
        }
        
        // 去除字符串末尾的换行符（如果有）
        line[strcspn(line, "\n")] = '\0';  // 用空字符 \0 替换 \n
        
        // 检查是否为空行（只包含空格、制表符等空白字符）
        if (is_empty_line(line)) {  // 如果是空行
            continue;  // 跳过本次循环，继续下一次循环
        }
        
        // 检查是否是 for 循环的开始（多行支持）
        char *trimmed = line;
        while (*trimmed == ' ' || *trimmed == '\t') trimmed++;
        if (strncmp(trimmed, "for ", 4) == 0) {
            // 检查是否已经包含 done（单行形式）
            if (strstr(trimmed, "done") == NULL) {
                // 多行 for 循环，继续读取直到找到 done
                char full_command[MAX_INPUT_LENGTH * 10];  // 足够大的缓冲区
                strcpy(full_command, line);
                strcat(full_command, " ");  // 添加空格分隔
                
                int found_done = 0;
                while (!found_done && strlen(full_command) < sizeof(full_command) - MAX_INPUT_LENGTH) {
                    // 显示继续提示符
                    printf("> ");
                    fflush(stdout);
                    
                    // 读取下一行
                    char next_line[MAX_INPUT_LENGTH];
                    if (read_line_with_completion(next_line, sizeof(next_line), prompt_callback) == NULL) {
                        break;  // Ctrl+D 退出
                    }
                    
                    // 去除换行符
                    next_line[strcspn(next_line, "\n")] = '\0';
                    
                    // 追加到完整命令
                    if (strlen(full_command) + strlen(next_line) + 2 < sizeof(full_command)) {
                        strcat(full_command, next_line);
                        strcat(full_command, " ");
                    }
                    
                    // 检查是否包含 done
                    char *next_trimmed = next_line;
                    while (*next_trimmed == ' ' || *next_trimmed == '\t') next_trimmed++;
                    if (strcmp(next_trimmed, "done") == 0 || 
                        (strncmp(next_trimmed, "done", 4) == 0 && 
                         (next_trimmed[4] == '\0' || next_trimmed[4] == ' ' || next_trimmed[4] == '\t'))) {
                        found_done = 1;
                    }
                }
                
                // 去除末尾的空格
                size_t len = strlen(full_command);
                while (len > 0 && full_command[len - 1] == ' ') {
                    full_command[--len] = '\0';
                }
                
                // 使用完整命令替换 line
                if (strlen(full_command) < sizeof(line)) {
                    strcpy(line, full_command);
                } else {
                    // 命令太长，截断
                    strncpy(line, full_command, sizeof(line) - 1);
                    line[sizeof(line) - 1] = '\0';
                }
            }
        }
        
        // 添加命令到历史记录
        // 说明：在执行命令前记录，即使命令失败也能保留历史
        // 功能：自动过滤空行和重复命令
        history_add(line);
        
        // 执行用户输入的命令
        execute_command_line(line, ctx);
        
        // 确保输出缓冲区被刷新
        fflush(stdout);
    }
    
    // 退出循环后，显示退出信息
    printf("######## Quiting XShell ########\n");
    
    // 清理历史记录系统
    // 功能：保存历史记录到文件并释放内存
    history_cleanup();
    
    // 清理别名系统
    // 功能：释放别名表内存
    alias_cleanup();
    
    // 清理全局上下文指针
    // 说明：将全局指针设置为 NULL，避免悬空指针（dangling pointer）
    // 重要性：如果以后 shell_loop 可能被多次调用，这可以防止使用已释放的内存
    g_shell_ctx = NULL;
}

// 显示命令提示符
// 格式：[\home\user\]# 或 [~]#（如果在主目录）
void display_prompt(ShellContext *ctx) {
    char display_path[PATH_MAX];  // 声明临时字符数组，用于存储修改后的路径
    
    // 检查是否在主目录，如果是则显示 ~
    if (ctx->home_dir != NULL && strcmp(ctx->cwd, ctx->home_dir) == 0) {
        strcpy(display_path, "~");
    } else if (ctx->home_dir != NULL && strncmp(ctx->cwd, ctx->home_dir, strlen(ctx->home_dir)) == 0) {
        // 如果当前目录在主目录下，将主目录部分替换为 ~
        snprintf(display_path, sizeof(display_path), "~%s", ctx->cwd + strlen(ctx->home_dir));
    } else {
        strcpy(display_path, ctx->cwd);  // 将当前工作目录复制到 display_path
    }
    
    // 遍历路径字符串，将所有的 / 替换为反斜杠（模仿 Windows 风格）
    for (size_t i = 0; i < strlen(display_path); i++) {  // i 从 0 到路径长度-1
        if (display_path[i] == '/') {  // 如果当前字符是斜杠 /
            display_path[i] = '\\';     // 替换为反斜杠（'\\' 表示单个反斜杠字符）
        }
    }
    
    // 打印提示符到标准输出
    printf("[%s]# ", display_path);  // 格式：[路径]#（末尾有空格）
    fflush(stdout);  // 立即刷新标准输出缓冲区（确保提示符立即显示，不等换行）
}

// 处理 for 循环
// 语法：for i in {1..100}; do command done
// 或：for i in word1 word2 word3; do command done
static int execute_for_loop(const char *line, ShellContext *ctx) {
    // 跳过 "for" 关键字
    const char *p = line;
    while (*p == ' ' || *p == '\t') p++;  // 跳过前导空白
    if (strncmp(p, "for ", 4) != 0) {
        return 0;  // 不是 for 循环
    }
    p += 4;  // 跳过 "for "
    
    // 提取循环变量名
    while (*p == ' ' || *p == '\t') p++;  // 跳过空白
    const char *var_start = p;
    while (*p != '\0' && *p != ' ' && *p != '\t' && *p != ';') p++;
    if (p == var_start) {
        return 0;  // 没有变量名
    }
    size_t var_len = p - var_start;
    char *var_name = malloc(var_len + 1);
    if (var_name == NULL) {
        return -1;
    }
    strncpy(var_name, var_start, var_len);
    var_name[var_len] = '\0';
    
    // 跳过 "in"
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "in ", 3) != 0) {
        free(var_name);
        return 0;  // 不是 for 循环
    }
    p += 3;
    
    // 提取列表（直到分号或 do）
    while (*p == ' ' || *p == '\t') p++;
    const char *list_start = p;
    const char *list_end = NULL;
    
    // 查找分号或 do
    const char *semicolon = strchr(p, ';');
    const char *do_pos = strstr(p, " do");
    if (semicolon != NULL && (do_pos == NULL || semicolon < do_pos)) {
        list_end = semicolon;
        p = semicolon + 1;
    } else if (do_pos != NULL) {
        list_end = do_pos;
        p = do_pos;
    } else {
        free(var_name);
        return 0;  // 没有找到分号或 do
    }
    
    // 提取列表字符串
    size_t list_len = list_end - list_start;
    char *list_str = malloc(list_len + 1);
    if (list_str == NULL) {
        free(var_name);
        return -1;
    }
    strncpy(list_str, list_start, list_len);
    list_str[list_len] = '\0';
    
    // 展开列表（支持大括号展开）
    char **expanded_list = NULL;
    int list_count = 0;
    
    // 检查是否包含大括号
    if (strchr(list_str, '{') != NULL) {
        // 使用 expand_brace 展开（从 executor.c）
        char** expand_brace(const char *arg);
        expanded_list = expand_brace(list_str);
        if (expanded_list != NULL) {
            while (expanded_list[list_count] != NULL) {
                list_count++;
            }
        }
    } else {
        // 简单的空格分隔列表
        // 计算单词数量
        const char *q = list_str;
        int word_count = 0;
        int in_word = 0;
        while (*q != '\0') {
            if (*q == ' ' || *q == '\t') {
                if (in_word) {
                    in_word = 0;
                }
            } else {
                if (!in_word) {
                    word_count++;
                    in_word = 1;
                }
            }
            q++;
        }
        
        if (word_count > 0) {
            expanded_list = malloc((word_count + 1) * sizeof(char*));
            if (expanded_list != NULL) {
                q = list_str;
                int idx = 0;
                in_word = 0;
                const char *word_start = NULL;
                while (*q != '\0') {
                    if (*q == ' ' || *q == '\t') {
                        if (in_word) {
                            size_t word_len = q - word_start;
                            expanded_list[idx] = malloc(word_len + 1);
                            if (expanded_list[idx] != NULL) {
                                strncpy(expanded_list[idx], word_start, word_len);
                                expanded_list[idx][word_len] = '\0';
                                idx++;
                            }
                            in_word = 0;
                        }
                    } else {
                        if (!in_word) {
                            word_start = q;
                            in_word = 1;
                        }
                    }
                    q++;
                }
                if (in_word) {
                    size_t word_len = q - word_start;
                    expanded_list[idx] = malloc(word_len + 1);
                    if (expanded_list[idx] != NULL) {
                        strncpy(expanded_list[idx], word_start, word_len);
                        expanded_list[idx][word_len] = '\0';
                        idx++;
                    }
                }
                expanded_list[idx] = NULL;
                list_count = idx;
            }
        }
    }
    
    free(list_str);
    
    if (expanded_list == NULL || list_count == 0) {
        free(var_name);
        return 0;  // 无法展开列表
    }
    
    // 跳过 "do"
    while (*p == ' ' || *p == '\t') p++;
    if (strncmp(p, "do", 2) != 0) {
        // 释放列表
        for (int i = 0; expanded_list[i] != NULL; i++) {
            free(expanded_list[i]);
        }
        free(expanded_list);
        free(var_name);
        return 0;
    }
    p += 2;
    
    // 提取循环体（直到 done）
    while (*p == ' ' || *p == '\t') p++;
    const char *body_start = p;
    
    // 查找 "done"，但要确保是完整的单词
    const char *done_pos = NULL;
    const char *search = p;
    while ((search = strstr(search, "done")) != NULL) {
        // 检查 "done" 前后是否是单词边界
        if ((search == p || search[-1] == ' ' || search[-1] == '\t' || search[-1] == ';') &&
            (search[4] == '\0' || search[4] == ' ' || search[4] == '\t' || search[4] == ';' || search[4] == '\n')) {
            done_pos = search;
            break;
        }
        search += 4;
    }
    
    if (done_pos == NULL) {
        // 释放列表
        for (int i = 0; expanded_list[i] != NULL; i++) {
            free(expanded_list[i]);
        }
        free(expanded_list);
        free(var_name);
        return 0;
    }
    
    size_t body_len = done_pos - body_start;
    char *body_template = malloc(body_len + 1);
    if (body_template == NULL) {
        // 释放列表
        for (int i = 0; expanded_list[i] != NULL; i++) {
            free(expanded_list[i]);
        }
        free(expanded_list);
        free(var_name);
        return -1;
    }
    strncpy(body_template, body_start, body_len);
    body_template[body_len] = '\0';
    
    // 去除循环体末尾的空白字符和分号
    while (body_len > 0 && (body_template[body_len - 1] == ' ' || 
                            body_template[body_len - 1] == '\t' ||
                            body_template[body_len - 1] == ';')) {
        body_template[--body_len] = '\0';
    }
    
    // 对每个列表值执行循环体
    int last_status = 0;
    for (int i = 0; i < list_count; i++) {
        // 替换循环变量
        // 计算需要的空间：body_template + 变量值的最大长度
        size_t max_var_len = strlen(expanded_list[i]) + 10;  // 变量值长度 + 一些余量
        char *body = malloc(strlen(body_template) + max_var_len + 1);
        if (body == NULL) {
            break;
        }
        
        // 简单的字符串替换：将 $var_name 替换为 expanded_list[i]
        const char *src = body_template;
        char *dst = body;
        while (*src != '\0') {
            if (*src == '$' && strncmp(src + 1, var_name, strlen(var_name)) == 0) {
                char next_char = src[1 + strlen(var_name)];
                // 检查变量名后是否是空白字符、结束符、引号或其他分隔符
                if (next_char == '\0' || next_char == ' ' || next_char == '\t' || 
                    next_char == ';' || next_char == '|' || next_char == '&' ||
                    next_char == '<' || next_char == '>' || next_char == '"' ||
                    next_char == '\'' || next_char == ')' || next_char == '}') {
                    // 找到变量，替换
                    size_t var_value_len = strlen(expanded_list[i]);
                    if (dst + var_value_len >= body + strlen(body_template) + max_var_len) {
                        // 缓冲区不足，重新分配
                        size_t current_len = dst - body;
                        size_t new_size = current_len + var_value_len + 100;
                        char *new_body = realloc(body, new_size);
                        if (new_body == NULL) {
                            break;
                        }
                        body = new_body;
                        dst = body + current_len;
                    }
                    strncpy(dst, expanded_list[i], var_value_len);
                    dst += var_value_len;
                    src += 1 + strlen(var_name);
                } else {
                    // 不是变量，直接复制
                    *dst++ = *src++;
                }
            } else {
                *dst++ = *src++;
            }
        }
        *dst = '\0';
        
        // 执行循环体
        int status = execute_command_line(body, ctx);
        if (status != 0) {
            last_status = status;
        }
        
        free(body);
    }
    
    // 释放资源
    free(body_template);
    for (int i = 0; expanded_list[i] != NULL; i++) {
        free(expanded_list[i]);
    }
    free(expanded_list);
    free(var_name);
    
    return (last_status == 0) ? 1 : -1;  // 1 表示成功处理，-1 表示有错误
}

// 执行命令行（支持 && 和 ||）
int execute_command_line(const char *line, ShellContext *ctx) {
    if (line == NULL || *line == '\0') {
        return 0;
    }
    
    // 检查是否是 for 循环
    int for_status = execute_for_loop(line, ctx);
    if (for_status != 0) {
        ctx->last_exit_status = (for_status == 1) ? 0 : -1;
        return ctx->last_exit_status;
    }
    
    // 简单检查：如果没有 && 或 ||，直接解析并执行
    char *and_check = strstr(line, "&&");
    char *or_check = strstr(line, "||");
    
    if (and_check == NULL && or_check == NULL) {
        // 没有命令链，直接执行
        Command *cmd = parse_command(line);
        if (cmd == NULL) {
            return -1;
        }
        int status = execute_command(cmd, ctx);
        ctx->last_exit_status = status;
        free_command(cmd);
        return status;
    }
    
    // 有命令链，需要分割处理
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        return -1;
    }
    
    int last_status = 0;
    char *current = line_copy;
    
    while (current != NULL && *current != '\0') {
        // 跳过前导空白
        while (*current == ' ' || *current == '\t') current++;
        if (*current == '\0') break;
        
        // 查找下一个 && 或 ||
        char *and_pos = strstr(current, "&&");
        char *or_pos = strstr(current, "||");
        
        char *sep = NULL;
        int sep_type = 0;  // 0=无, 1=&&, 2=||
        
        // 找最近的分隔符
        if (and_pos != NULL && (or_pos == NULL || and_pos < or_pos)) {
            sep = and_pos;
            sep_type = 1;
        } else if (or_pos != NULL) {
            sep = or_pos;
            sep_type = 2;
        }
        
        // 获取下一段命令的起始位置
        char *next = NULL;
        if (sep != NULL) {
            *sep = '\0';
            next = sep + 2;
        }
        
        // 去除当前命令尾部空白
        size_t len = strlen(current);
        while (len > 0 && (current[len-1] == ' ' || current[len-1] == '\t')) {
            current[--len] = '\0';
        }
        
        // 执行当前命令
        if (*current != '\0') {
            Command *cmd = parse_command(current);
            if (cmd != NULL) {
                last_status = execute_command(cmd, ctx);
                ctx->last_exit_status = last_status;
                free_command(cmd);
            } else {
                last_status = -1;
                ctx->last_exit_status = last_status;
            }
        }
        
        // 根据分隔符类型和结果决定是否继续
        if (sep_type == 1 && last_status != 0) {
            // && 且失败，停止执行后续命令
            break;
        } else if (sep_type == 2 && last_status == 0) {
            // || 且成功，停止执行后续命令
            break;
        }
        
        current = next;
    }
    
    free(line_copy);
    return last_status;
}

// 清理 Shell 资源
void cleanup_shell(ShellContext *ctx) {
    // 关闭日志文件
    if (ctx->log_file != NULL) {
        fclose(ctx->log_file);
        ctx->log_file = NULL;
    }
}

// 日志记录函数
// 功能：将错误信息同时输出到标准错误流和日志文件
// 格式：[时间戳] PID=进程ID ERROR: 错误信息
// 参数：
//   ctx - Shell 上下文（包含日志文件指针）
//   format - 格式化字符串（类似 printf）
//   ... - 可变参数
void log_error(ShellContext *ctx, const char *format, ...) {
    if (ctx == NULL) {
        return;
    }
    
    // 获取当前时间
    time_t now = time(NULL);
    char time_str[64];
    struct tm *tm_info = localtime(&now);
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    // 获取当前进程 ID
    pid_t pid = getpid();
    
    // 准备可变参数列表
    va_list args;
    
    // 输出到标准错误流
    fprintf(stderr, "[%s] PID=%d ERROR: ", time_str, pid);
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fprintf(stderr, "\n");
    
    // 输出到日志文件（如果日志文件已打开）
    if (ctx->log_file != NULL) {
        fprintf(ctx->log_file, "[%s] PID=%d ERROR: ", time_str, pid);
        va_start(args, format);
        vfprintf(ctx->log_file, format, args);
        va_end(args);
        fprintf(ctx->log_file, "\n");
        fflush(ctx->log_file);  // 立即刷新，确保日志及时写入
    }
}