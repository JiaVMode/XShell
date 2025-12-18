// 定义 POSIX 标准版本，启用strdup等函数
#define _POSIX_C_SOURCE 200809L

// 引入头文件
#include "parser.h"                                 // Command 结构体定义，函数声明
#include "utils.h"                                  // 工具函数（trim,is_empty_line等）
#include <stdio.h>                                  // 标准输入输出（perror）
#include <stdlib.h>                                 // 内存管理
#include <string.h>                                 // 字符串处理（strlen、strdup、strtok）
#include <ctype.h>                                  // 字符类型（isspace等）
#include <unistd.h>                                 // getenv

// 常量定义
#define MAX_TOKENS 256                               // 单条命令最大token数量（防止数组越界）

// 检查是否是重定向符号
static int is_redirect(const char *token) {
    return (strcmp(token, ">") == 0 || 
            strcmp(token, ">>") == 0 || 
            strcmp(token, "2>") == 0 ||
            strcmp(token, "<") == 0);
}

// 检查是否是管道符号
static int is_pipe(const char *token) {
    return strcmp(token, "|") == 0;
}

// 将重定向符号转换为类型
static RedirectType get_redirect_type(const char *token) {
    if (strcmp(token, ">") == 0) return REDIRECT_OUT;
    if (strcmp(token, ">>") == 0) return REDIRECT_APPEND;
    if (strcmp(token, "2>") == 0) return REDIRECT_ERR;
    if (strcmp(token, "<") == 0) return REDIRECT_IN;
    return REDIRECT_NONE;
}

// 跳过空白字符
static char* skip_whitespace(char *p) {
    while (*p == ' ' || *p == '\t') {
        p++;
    }
    return p;
}

// 变量展开函数
static char* expand_variables(const char *input) {
    if (input == NULL) {
        return NULL;
    }
    
    size_t input_len = strlen(input);
    size_t output_capacity = input_len * 2; // 初始容量
    char *output = malloc(output_capacity);
    if (output == NULL) {
        return NULL;
    }
    
    size_t output_pos = 0;
    const char *p = input;
    
    // 检查是否是 VAR="value" 格式，如果是，去除引号
    char *equals = strchr(input, '=');
    if (equals != NULL) {
        // 检查等号后是否有引号包围
        const char *after_equals = equals + 1;
        size_t value_len = strlen(after_equals);
        if (value_len >= 2 && 
            ((after_equals[0] == '"' && after_equals[value_len-1] == '"') ||
             (after_equals[0] == '\'' && after_equals[value_len-1] == '\''))) {
            // 创建去掉引号的版本
            size_t new_len = input_len - 2; // 去掉两个引号
            char *unquoted_input = malloc(new_len + 1);
            if (unquoted_input != NULL) {
                // 复制 VAR= 部分
                size_t prefix_len = equals - input + 1;
                strncpy(unquoted_input, input, prefix_len);
                // 复制去掉引号的值部分
                strncpy(unquoted_input + prefix_len, after_equals + 1, value_len - 2);
                unquoted_input[new_len] = '\0';
                
                // 递归处理去掉引号的版本
                char *result = expand_variables(unquoted_input);
                free(unquoted_input);
                free(output);
                return result;
            }
        }
    }
    
    while (*p != '\0') {
        if (*p == '$' && (isalnum(p[1]) || p[1] == '_')) {
            // 找到变量名
            p++; // 跳过 $
            const char *var_start = p;
            while (isalnum(*p) || *p == '_') {
                p++;
            }
            
            // 提取变量名
            size_t var_len = p - var_start;
            char *var_name = malloc(var_len + 1);
            if (var_name == NULL) {
                free(output);
                return NULL;
            }
            strncpy(var_name, var_start, var_len);
            var_name[var_len] = '\0';
            
            // 获取环境变量值
            const char *var_value = getenv(var_name);
            if (var_value == NULL) {
                var_value = ""; // 未定义的变量展开为空字符串
            }
            
            // Debug: 记录变量展开信息
            // fprintf(stderr, "DEBUG: Expanding $%s to '%s'\n", var_name, var_value);
            
            // 确保输出缓冲区足够大
            size_t var_value_len = strlen(var_value);
            while (output_pos + var_value_len >= output_capacity) {
                output_capacity *= 2;
                char *new_output = realloc(output, output_capacity);
                if (new_output == NULL) {
                    free(var_name);
                    free(output);
                    return NULL;
                }
                output = new_output;
            }
            
            // 复制变量值到输出
            strcpy(output + output_pos, var_value);
            output_pos += var_value_len;
            
            free(var_name);
        } else if (*p == '~' && (p == input || p[-1] == ':' || p[-1] == '=' || isspace(p[-1]) || p[-1] == '"' || p[-1] == '\'')) {
            // 波浪号展开（在字符串开头或在 : = 空格后）
            const char *home_dir = getenv("HOME");
            if (home_dir == NULL) {
                home_dir = "/tmp"; // 默认目录
            }
            
            // 确保输出缓冲区足够大
            size_t home_len = strlen(home_dir);
            while (output_pos + home_len >= output_capacity) {
                output_capacity *= 2;
                char *new_output = realloc(output, output_capacity);
                if (new_output == NULL) {
                    free(output);
                    return NULL;
                }
                output = new_output;
            }
            
            // 复制主目录路径
            strcpy(output + output_pos, home_dir);
            output_pos += home_len;
            p++; // 跳过 ~
        } else {
            // 普通字符，直接复制
            if (output_pos + 1 >= output_capacity) {
                output_capacity *= 2;
                char *new_output = realloc(output, output_capacity);
                if (new_output == NULL) {
                    free(output);
                    return NULL;
                }
                output = new_output;
            }
            output[output_pos++] = *p++;
        }
    }
    
    output[output_pos] = '\0';
    return output;
}

// 解析单个命令（不包含管道）
static Command* parse_single_command(char *line_start, char *line_end) {
    if (line_start >= line_end) {
        return NULL;
    }
    
    // 分配Command对象
    Command *cmd = (Command*)calloc(1, sizeof(Command));
    if (cmd == NULL) {
        perror("calloc");
        return NULL;
    }
    
    // 初始化
    cmd->redirect_type = REDIRECT_NONE;
    cmd->redirect_file = NULL;
    cmd->pipe_next = NULL;
    
    // 初始化多重定向字段
    cmd->stdout_file = NULL;
    cmd->stderr_file = NULL;
    cmd->stdin_file = NULL;
    cmd->stdout_append = 0;
    cmd->stderr_append = 0;
    cmd->args = (char**)calloc(MAX_TOKENS, sizeof(char*));
    if (cmd->args == NULL) {
        perror("calloc");
        free(cmd);
        return NULL;
    }
    
    cmd->arg_count = 0;
    
    // 复制字符串
    size_t len = line_end - line_start;
    char *line = (char*)malloc(len + 1);
    if (line == NULL) {
        perror("malloc");
        free(cmd->args);
        free(cmd);
        return NULL;
    }
    strncpy(line, line_start, len);
    line[len] = '\0';
    
    // 手动解析（支持多字符重定向符号）
    char *p = line;
    p = skip_whitespace(p);
    
    int expecting_redirect_file = 0;
    RedirectType current_redirect = REDIRECT_NONE;
    
    while (*p != '\0' && cmd->arg_count < MAX_TOKENS - 1) {
        p = skip_whitespace(p);
        if (*p == '\0') break;
        
        // 检查重定向符号
        if (strncmp(p, "2>>", 3) == 0) {
            // 错误追加（虽然不常用，但支持）
            current_redirect = REDIRECT_ERR;
            expecting_redirect_file = 1;
            p += 3;
            continue;
        } else if (strncmp(p, "2>", 2) == 0) {
            current_redirect = REDIRECT_ERR;
            expecting_redirect_file = 1;
            p += 2;
            continue;
        } else if (strncmp(p, ">>", 2) == 0) {
            current_redirect = REDIRECT_APPEND;
            expecting_redirect_file = 1;
            p += 2;
            continue;
        } else if (*p == '>') {
            current_redirect = REDIRECT_OUT;
            expecting_redirect_file = 1;
            p += 1;
            continue;
        } else if (*p == '<') {
            current_redirect = REDIRECT_IN;
            expecting_redirect_file = 1;
            p += 1;
            continue;
        }
        
        // 提取token（支持引号）
        char *token_start = p;
        char quote_char = 0;
        
        // 检查是否以引号开始
        if (*p == '"' || *p == '\'') {
            quote_char = *p;
            p++; // 跳过开始引号
            token_start = p; // 引号内容的开始
            
            // 找到匹配的结束引号
            while (*p != '\0' && *p != quote_char) {
                p++;
            }
            
            if (*p == quote_char) {
                // 找到了结束引号，不包含在 token 中
                // p 指向结束引号，token_len 将是引号内容的长度
            } else {
                // 没有找到结束引号，将整个剩余部分作为 token
            }
        } else {
            // 普通 token，按空格和特殊字符分割
            while (*p != '\0' && *p != ' ' && *p != '\t' && 
                   *p != '>' && *p != '<' && *p != '|') {
                // 如果遇到引号，检查是否是 VAR="value" 格式
                if (*p == '"' || *p == '\'') {
                    // 检查是否在等号后面，如果是，则包含引号内容
                    char *equals = strchr(token_start, '=');
                    if (equals != NULL && p > equals) {
                        // 这是 VAR="value" 格式，包含引号内容
                        quote_char = *p;
                        p++; // 跳过开始引号
                        
                        // 找到结束引号，包含在同一个token中
                        while (*p != '\0' && *p != quote_char) {
                            p++;
                        }
                        
                        if (*p == quote_char) {
                            p++; // 跳过结束引号
                        }
                    } else {
                        // 普通情况，遇到引号就停止
                        break;
                    }
                } else {
                    // 检查是否是"2>"的一部分
                    if (*p == '2' && (p[1] == '>' || (p[1] == '>' && p[2] == '>'))) {
                        break;
                    }
                    p++;
                }
            }
        }
        
        if (p > token_start || quote_char != 0) {
            size_t token_len = p - token_start;
            char *token = (char*)malloc(token_len + 1);
            if (token == NULL) {
                perror("malloc");
                free(line);
                free_command(cmd);
                return NULL;
            }
            strncpy(token, token_start, token_len);
            token[token_len] = '\0';
            
            // 如果是引号 token，跳过结束引号
            if (quote_char != 0 && *p == quote_char) {
                p++;
            }
            
            // 展开变量
            char *expanded_token = expand_variables(token);
            if (expanded_token != NULL) {
                free(token);
                token = expanded_token;
            }
            
            if (expecting_redirect_file) {
                // 这是重定向文件名，根据类型设置相应字段
                switch (current_redirect) {
                    case REDIRECT_OUT:
                        cmd->stdout_file = token;
                        cmd->stdout_append = 0;
                        break;
                    case REDIRECT_APPEND:
                        cmd->stdout_file = token;
                        cmd->stdout_append = 1;
                        break;
                    case REDIRECT_ERR:
                        cmd->stderr_file = token;
                        cmd->stderr_append = 0;
                        break;
                    case REDIRECT_IN:
                        cmd->stdin_file = token;
                        break;
                    default:
                        free(token);
                        break;
                }
                
                // 兼容性：设置第一个重定向为主重定向
                if (cmd->redirect_type == REDIRECT_NONE) {
                    cmd->redirect_type = current_redirect;
                    cmd->redirect_file = token;
                }
                
                expecting_redirect_file = 0;
            } else {
                // 这是命令参数
                cmd->args[cmd->arg_count] = token;
                cmd->arg_count++;
            }
        }
    }
    
    free(line);
    
    // 检查是否有未完成的重定向
    if (expecting_redirect_file) {
        fprintf(stderr, "parse error: missing redirect file\n");
        free_command(cmd);
        return NULL;
    }
    
    // 设置命令名
    if (cmd->arg_count > 0) {
        cmd->name = cmd->args[0];
    } else {
        cmd->name = NULL;
    }
    
    // 参数数组以NULL结尾
    cmd->args[cmd->arg_count] = NULL;
    
    // Debug: 打印解析结果
    // fprintf(stderr, "DEBUG: Parsed command: '%s', args: %d, redirect: %d, file: '%s'\n", 
    //         cmd->name ? cmd->name : "(null)", 
    //         cmd->arg_count, 
    //         cmd->redirect_type, 
    //         cmd->redirect_file ? cmd->redirect_file : "(null)");
    
    return cmd;
}

// 命令解析函数
// 功能：将用户输入的命令行字符解析为 Command 结构体
// 支持重定向和管道
Command* parse_command(const char *line) {
    // 步骤1：参数检查
    if (line == NULL || strlen(line) == 0) {
        return NULL;
    }
    
    // 步骤2：处理注释并复制命令行字符串
    char *line_copy = strdup(line);
    if (line_copy == NULL) {
        perror("strdup");
        return NULL;
    }
    
    // 移除注释：找到第一个不在引号内的 # 字符，截断字符串
    char *comment_pos = NULL;
    int in_single_quote = 0;
    int in_double_quote = 0;
    
    for (char *p = line_copy; *p != '\0'; p++) {
        if (*p == '\'' && !in_double_quote) {
            in_single_quote = !in_single_quote;
        } else if (*p == '"' && !in_single_quote) {
            in_double_quote = !in_double_quote;
        } else if (*p == '#' && !in_single_quote && !in_double_quote) {
            comment_pos = p;
            break;
        }
    }
    
    // 如果找到注释，截断字符串
    if (comment_pos != NULL) {
        *comment_pos = '\0';
    }
    
    // 如果处理注释后字符串为空或只有空白，返回NULL
    char *trimmed = line_copy;
    while (*trimmed == ' ' || *trimmed == '\t') {
        trimmed++;
    }
    if (*trimmed == '\0') {
        free(line_copy);
        return NULL;
    }
    
    // 步骤3：查找管道符号，分割命令链
    char *pipe_pos = strchr(line_copy, '|');
    
    if (pipe_pos == NULL) {
        // 没有管道，解析单个命令
        char *end = line_copy + strlen(line_copy);
        Command *cmd = parse_single_command(line_copy, end);
        free(line_copy);
        return cmd;
    } else {
        // 有管道，需要解析命令链
        Command *first_cmd = NULL;
        Command *prev_cmd = NULL;
        char *start = line_copy;
        
        while (start != NULL) {
            // 找到下一个管道符号或字符串结尾
            char *pipe = strchr(start, '|');
            char *end = (pipe != NULL) ? pipe : (start + strlen(start));
            
            // 解析当前命令
            Command *cmd = parse_single_command(start, end);
            if (cmd == NULL) {
                // 解析失败，释放已解析的命令
                if (first_cmd != NULL) {
                    free_command(first_cmd);
                }
                free(line_copy);
                return NULL;
            }
            
            // 链接命令
            if (first_cmd == NULL) {
                first_cmd = cmd;
            } else {
                prev_cmd->pipe_next = cmd;
            }
            prev_cmd = cmd;
            
            // 移动到下一个命令
            if (pipe != NULL) {
                start = pipe + 1;
                // 跳过空格
                while (*start == ' ' || *start == '\t') {
                    start++;
                }
                if (*start == '\0') {
                    break;
                }
            } else {
                break;
            }
        }
        
        free(line_copy);
        return first_cmd;
    }
}

// 内存释放函数
// 功能：释放 Command 对象及其内部所有动态分配的内存
// 注意：必须按照"从内到外" 的释放顺序，避免内存泄漏
void free_command(Command *cmd) {
    if (cmd == NULL) {
        return;
    }
    
    // 递归释放管道中的下一个命令
    if (cmd->pipe_next != NULL) {
        free_command(cmd->pipe_next);
    }
    
    // 释放重定向文件名
    if (cmd->redirect_file != NULL) {
        free(cmd->redirect_file);
    }
    
    // 释放多重定向文件名
    if (cmd->stdout_file != NULL && cmd->stdout_file != cmd->redirect_file) {
        free(cmd->stdout_file);
    }
    if (cmd->stderr_file != NULL && cmd->stderr_file != cmd->redirect_file) {
        free(cmd->stderr_file);
    }
    if (cmd->stdin_file != NULL && cmd->stdin_file != cmd->redirect_file) {
        free(cmd->stdin_file);
    }
    
    // 释放参数数组中的每个字符串
    if (cmd->args != NULL) {
        for (int i = 0; i < cmd->arg_count; i++) {
            if (cmd->args[i] != NULL) {
                free(cmd->args[i]);
            }
        }
        free(cmd->args);
    }
    
    // 释放Command本身
    free(cmd);
}
