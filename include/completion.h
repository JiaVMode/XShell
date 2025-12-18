// 头文件保护：防止重复包含
#ifndef COMPLETION_H                                    // 如果 COMPLETION_H 未定义
#define COMPLETION_H                                    // 则定义 COMPLETION_H

#include <stddef.h>                                     // size_t 类型定义

// Tab 补全功能模块
// 功能说明：实现类似 bash 的 Tab 键自动补全路径功能
// 使用场景：用户在输入命令时按 Tab 键，自动补全文件/目录名

// Tab 补全核心函数
// 功能：根据输入的部分路径，查找匹配的文件/目录并返回补全建议
// 工作原理：
//   1. 解析用户当前输入，提取需要补全的路径部分
//   2. 打开对应目录，遍历所有文件/目录
//   3. 匹配前缀，收集所有符合条件的项
//   4. 返回匹配数组供调用者选择
// 参数：
//   - input: 当前输入的完整字符串（如 "xcd /home/la"）
//   - cursor_pos: 光标位置（在 input 中的索引）
//   - matches: 输出参数，存储所有匹配项的数组（调用者负责使用 free_completions 释放）
// 返回值：匹配项的数量（0 表示无匹配）
// 注意：调用者必须调用 free_completions() 释放 matches 的内存
int get_path_completions(const char *input, int cursor_pos, char ***matches);

// 释放补全匹配项数组
// 功能：释放 get_path_completions() 分配的所有内存
// 参数：
//   - matches: 匹配项数组（字符串指针数组）
//   - count: 数组长度（匹配项数量）
// 注意：此函数会：
//   1. 遍历数组，释放每个字符串
//   2. 释放数组本身
void free_completions(char **matches, int count);

// 从输入中提取需要补全的路径部分
// 功能：解析用户输入，分离出目录前缀和待补全的文件名
// 例如：
//   输入 "xcd /home/lab/L" -> prefix="/home/lab/", partial="L"
//   输入 "xcd LJ"          -> prefix="", partial="LJ"
// 参数：
//   - input: 完整输入字符串
//   - cursor_pos: 光标位置
//   - prefix: 输出参数，存储路径前缀（目录部分，如 "/home/lab/"）
//   - partial: 输出参数，存储部分文件名（需要补全的部分，如 "L"）
// 注意：prefix 和 partial 的缓冲区由调用者分配和管理
void extract_path_to_complete(const char *input, int cursor_pos, 
                               char *prefix, char *partial);

// ===== 增强补全功能 =====

// 补全类型枚举
typedef enum {
    COMPLETION_TYPE_COMMAND,      // 命令名补全
    COMPLETION_TYPE_OPTION,       // 选项补全（--xxx）
    COMPLETION_TYPE_PATH,         // 路径补全（默认）
    COMPLETION_TYPE_DIR_ONLY,     // 只补全目录
    COMPLETION_TYPE_FILE_ONLY     // 只补全文件
} CompletionType;

// 获取补全类型（根据上下文判断）
// 功能：分析用户输入，判断应该进行哪种类型的补全
// 参数：
//   - input: 完整输入字符串
//   - cursor_pos: 光标位置
// 返回：补全类型
CompletionType get_completion_type(const char *input, int cursor_pos);

// 获取命令名补全
// 功能：根据部分命令名，查找匹配的内置命令
// 参数：
//   - partial: 部分命令名（如 "xls"）
//   - matches: 输出参数，存储匹配的命令名数组
// 返回：匹配的命令数量
int get_command_completions(const char *partial, char ***matches);

// 获取选项补全
// 功能：根据命令名和部分选项，查找匹配的选项
// 参数：
//   - cmd_name: 命令名（如 "xls"）
//   - partial: 部分选项（如 "--"）
//   - matches: 输出参数，存储匹配的选项数组
// 返回：匹配的选项数量
int get_option_completions(const char *cmd_name, const char *partial, char ***matches);

// 获取路径补全（增强版，支持文件类型过滤）
// 功能：根据上下文获取路径补全，支持目录/文件过滤
// 参数：
//   - input: 完整输入字符串
//   - cursor_pos: 光标位置
//   - completion_type: 补全类型（目录/文件/全部）
//   - matches: 输出参数，存储匹配项数组
// 返回：匹配项数量
int get_enhanced_path_completions(const char *input, int cursor_pos, 
                                   CompletionType completion_type, char ***matches);

// 智能补全（主入口函数）
// 功能：根据上下文自动选择补全类型并执行补全
// 参数：
//   - input: 完整输入字符串
//   - cursor_pos: 光标位置
//   - matches: 输出参数，存储匹配项数组
// 返回：匹配项数量
int get_smart_completions(const char *input, int cursor_pos, char ***matches);

#endif // COMPLETION_H                                 // 头文件保护结束
