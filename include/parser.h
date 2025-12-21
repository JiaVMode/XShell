// 头文件保护，防止重复包含
#ifndef PARSER_H                                //如果PARSER_H 未定义
#define PARSER_H                                // 则定义PARSER_H

// 重定向类型
typedef enum {
    REDIRECT_NONE = 0,      // 无重定向
    REDIRECT_OUT,           // > 覆盖输出
    REDIRECT_APPEND,        // >> 追加输出
    REDIRECT_ERR,           // 2> 错误输出
    REDIRECT_IN             // < 输入重定向
} RedirectType;

// 命令结构体：存储解析后的单个命令信息
typedef struct Command
{
    char *name;                                 // 命令名称（指向 args[0]，不单独分配内存
                                                // 例如："ls", "xpwd","cat"等
                        
    char **args;                                // 参数数组（字符串指针数组）
                                                // 包含命令名和所有参数，以NULL结尾
                                                // 例如：{"ls", "-l", "/home", NULL}
                                                // 这种格式可以直接传递给execv系列函数

    int arg_count;                              // 参数数量（不包含末尾的NULL）
                                                // 例如：对于"ls -l /home", arg_count = 2
    
    // 重定向信息（支持多个重定向）
    RedirectType redirect_type;                 // 主要重定向类型（保持兼容性）
    char *redirect_file;                        // 主要重定向文件名（保持兼容性）
    
    // 多重定向支持
    char *stdout_file;                          // 标准输出重定向文件 (>)
    char *stderr_file;                          // 错误输出重定向文件 (2>)
    char *stdin_file;                           // 输入重定向文件 (<)
    int stdout_append;                          // 标准输出是否追加模式 (>>)
    int stderr_append;                          // 错误输出是否追加模式 (2>>)
    
    // 管道信息
    struct Command *pipe_next;                  // 管道中的下一个命令（NULL表示没有管道）
    
    // 命令链信息（&& 和 ||）
    struct Command *chain_next;                 // 命令链中的下一个命令（NULL表示没有链）
    int chain_type;                             // 链类型：0=无, 1=&&, 2=||
    
    // 后台执行
    int background;                             // 是否后台执行（以 & 结尾）
} Command;

//  函数声明
// 命令解析和内存管理函数

// 解析命令行字符串，返回Command 结构体指针
// 参数：line - 用户输入的命令行字符串
// 返回：成功返回Command* 指针，失败返回NULL
// 注意：调用者负责调用 free_command() 释放返回的内存
Command* parse_command(const char *line);

// 释放 Command 结构体及其内部动态分配的所有内存
// 参数：cmd - 需要释放的Command指针
//注意：会释放 cmd->args 中的所有字符串以及args数组本省
void free_command(Command *cmd);

# endif                                         // PARSER_H 头文件保护结束