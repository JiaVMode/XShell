// 引入头文件
#include "input.h"                                      // 输入处理函数声明
#include "completion.h"                                 // Tab 补全功能
#include "history.h"                                    // 历史记录功能
#include <stdio.h>                                      // 标准输入输出（printf, getchar）
#include <stdlib.h>                                     // 标准库（malloc, free）
#include <string.h>                                     // 字符串处理（strcmp, strcpy, strlen）
#include <unistd.h>                                     // UNIX 标准（read, write）
#include <termios.h>                                    // 终端控制（termios 结构体）

// 特殊按键 ASCII 码定义
#define KEY_TAB 9                                       // Tab 键（水平制表符）
#define KEY_BACKSPACE 127                               // 退格键（DEL）
#define KEY_CTRL_D 4                                    // Ctrl+D（EOT，文件结束）
#define KEY_NEWLINE 10                                  // 换行符（LF，Line Feed）
#define KEY_RETURN 13                                   // 回车键（CR，Carriage Return）
#define KEY_ESC 27                                      // ESC 键（转义序列开始）
#define KEY_CTRL_A 1                                    // Ctrl+A（行首）
#define KEY_CTRL_E 5                                    // Ctrl+E（行尾）
#define KEY_CTRL_C 3                                    // Ctrl+C（取消）
#define KEY_CTRL_L 12                                   // Ctrl+L（清屏）
#define KEY_CTRL_U 21                                   // Ctrl+U（清除行）
#define KEY_CTRL_R 18                                   // Ctrl+R（反向搜索历史）

// ============================================
// 辅助函数：从历史中反向查找包含 query 的最近一条
// ============================================
// 返回：匹配到的历史命令字符串指针（由 history 模块持有），找不到返回 NULL
static const char *history_reverse_search(const char *query) {
    if (query == NULL) {
        return NULL;
    }

    int count = history_count();
    if (count <= 0) {
        return NULL;
    }

    if (query[0] == '\0') {
        // 空查询：返回最近一条
        return history_get(count - 1);
    }

    for (int i = count - 1; i >= 0; i--) {
        const char *item = history_get(i);
        if (item != NULL && strstr(item, query) != NULL) {
            return item;
        }
    }
    return NULL;
}

// ============================================
// 辅助函数：清空当前行并重新显示（带提示符）
// ============================================
// 功能：清除当前行显示的内容，然后重新显示提示符和新内容
// 用途：用于历史浏览、光标移动等场景
// 参数：
//   - buffer: 新的行内容
//   - pos: 当前光标位置
//   - old_len: 旧内容的长度
//   - prompt_callback: 提示符显示回调函数
static void refresh_line(const char *buffer, int pos, int old_len, PromptCallback prompt_callback) {
    // 步骤1：移动光标到行首
    printf("\r");                                       // 回车符，移到行首
    
    // 步骤2：使用 ANSI 转义序列清空从光标到行尾的内容
    // ESC[K 清除从光标到行尾的内容（不换行）
    printf("\033[K");                                   // 清空从光标到行尾
    
    // 步骤3：重新显示提示符
    if (prompt_callback != NULL) {
        prompt_callback();                              // 调用回调显示提示符
    }
    
    // 步骤4：打印新内容
    printf("%s", buffer);
    
    // 步骤5：将光标移到正确位置
    // 如果光标不在末尾，需要向左移动
    int new_len = strlen(buffer);
    if (pos < new_len) {
        // 向左移动到正确位置
        for (int i = 0; i < new_len - pos; i++) {
            printf("\b");                               // 向左移动一格
        }
    }
    
    // 步骤6：刷新输出缓冲区
    fflush(stdout);
}

// ============================================
// 辅助函数：清空当前输入并替换为新内容
// ============================================
// 功能：用于历史浏览时替换当前输入
// 参数：
//   - buffer: 输入缓冲区
//   - size: 缓冲区大小
//   - new_text: 新文本内容
//   - prompt_callback: 提示符显示回调函数
// 返回值：新内容的长度
static int replace_line(char *buffer, size_t size, const char *new_text, PromptCallback prompt_callback) {
    // 步骤1：保存旧内容长度
    int old_len = strlen(buffer);
    
    // 步骤2：复制新内容
    strncpy(buffer, new_text, size - 1);
    buffer[size - 1] = '\0';
    
    // 步骤3：刷新显示（带提示符）
    int new_len = strlen(buffer);
    refresh_line(buffer, new_len, old_len, prompt_callback);
    
    // 步骤4：返回新长度
    return new_len;
}

// 计算多个字符串的公共前缀
// 功能：找出多个字符串共有的前缀部分
// 用途：第一次按 Tab 时，如果有多个匹配但有公共前缀，可以自动补全到公共部分
// 例如：["lab", "laboratory"] -> 公共前缀 "lab"
//      ["file1", "file2"]    -> 公共前缀 "file"
// 参数：
//   - matches: 字符串指针数组
//   - count: 数组长度（字符串数量）
//   - common: 输出缓冲区，存储公共前缀
//   - max_len: 输出缓冲区的最大长度
// 返回：公共前缀的长度（字符数）
static int get_common_prefix(char **matches, int count, char *common, size_t max_len) {
    // 步骤1：参数检查
    if (count == 0 || matches == NULL || matches[0] == NULL) {
        common[0] = '\0';                               // 设置为空字符串
        return 0;                                       // 返回长度 0
    }
    
    // 步骤2：以第一个字符串为基准进行比较
    int prefix_len = 0;                                 // 公共前缀长度计数器
    int first_len = strlen(matches[0]);                 // 第一个字符串的长度
    
    // 步骤3：逐字符比较所有字符串
    for (int i = 0; i < first_len && i < (int)max_len - 1; i++) {
        char ch = matches[0][i];                        // 取第一个字符串的第 i 个字符
        
        // 步骤3.1：检查所有其他字符串在位置 i 是否有相同字符
        int all_match = 1;                              // 是否全部匹配的标志
        for (int j = 1; j < count; j++) {               // 遍历其他字符串
            if (matches[j][i] != ch) {                  // 如果字符不同
                all_match = 0;                          // 设置为不匹配
                break;                                  // 跳出循环
            }
        }
        
        // 步骤3.2：如果所有字符串都匹配，添加到公共前缀
        if (all_match) {
            common[prefix_len++] = ch;                  // 添加字符到公共前缀
        } else {
            break;                                      // 遇到不匹配，停止比较
        }
    }
    
    // 步骤4：添加字符串结束符并返回长度
    common[prefix_len] = '\0';                          // 添加结束符
    return prefix_len;                                  // 返回公共前缀长度
}

// 读取带 Tab 补全功能的用户输入
// 功能：实现类似 bash 的交互式输入，支持 Tab 补全、退格等编辑功能
// 核心特性：
//   1. 原始终端模式（逐字符读取，不等待回车）
//   2. 第一次 Tab：补全公共前缀或响铃
//   3. 第二次 Tab：显示所有匹配选项
//   4. 退格键删除字符
//   5. Ctrl+D 退出
char* read_line_with_completion(char *buffer, size_t size, PromptCallback prompt_callback) {
    // 步骤1：参数有效性检查
    if (buffer == NULL || size == 0) {                  // 检查无效参数
        return NULL;                                    // 返回 NULL 表示失败
    }
    
    // 步骤2：初始化输入缓冲区
    int pos = 0;                                        // 当前输入位置（光标位置）
    buffer[0] = '\0';                                   // 初始化为空字符串
    
    // 步骤3：实现双击 Tab 的状态变量（使用 static 在函数调用间保持状态）
    static int last_was_tab = 0;                        // 上次按键是否为 Tab（0=否，1=是）
    static char last_input[4096];                       // 上次 Tab 时的输入内容
    
    // 步骤3.5：历史浏览状态变量
    int history_index = -1;                             // 当前浏览的历史索引（-1 表示未浏览）
    
    // 步骤4：保存原始终端设置（以便恢复）
    struct termios old_term, new_term;                  // 终端设置结构体
    tcgetattr(STDIN_FILENO, &old_term);                 // 获取当前终端设置
    new_term = old_term;                                // 复制到 new_term
    
    // 步骤5：设置终端为原始模式
    // 原始模式特点：
    //   - ICANON：关闭规范模式（不等待回车，逐字符读取）
    //   - ECHO：关闭自动回显（手动控制字符显示）
    //   - ISIG：关闭信号生成（Ctrl+C 不会发送 SIGINT，可以自己处理）
    new_term.c_lflag &= ~(ICANON | ECHO | ISIG);        // 清除这三个标志位
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);        // 立即应用新设置
    
    // 步骤6：主输入循环
    int ch;                                             // 当前读取的字符
    while (1) {                                         // 无限循环，直到用户按回车或 Ctrl+D
        ch = getchar();                                 // 读取一个字符
        
        // 处理 Ctrl+D 或 EOF
        if (ch == EOF || ch == KEY_CTRL_D) {
            // Ctrl+D（文件结束信号）或真正的 EOF
            if (pos == 0) {                             // 如果输入为空
                tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // 恢复终端设置
                return NULL;                            // 返回 NULL 表示用户想退出
            }
            break;                                      // 如果有输入，结束输入循环
        }
        
        // 处理 Ctrl+C（取消当前输入）
        else if (ch == KEY_CTRL_C) {
            printf("^C\n");                             // 显示 ^C 并换行
            buffer[0] = '\0';                           // 清空缓冲区
            pos = 0;                                    // 重置位置
            history_index = -1;                         // 重置历史索引
            tcsetattr(STDIN_FILENO, TCSANOW, &old_term); // 恢复终端设置
            return buffer;                              // 返回空字符串
        }
        
        // 处理 Ctrl+A（移到行首）
        else if (ch == KEY_CTRL_A) {
            // 将光标移到行首
            while (pos > 0) {
                printf("\b");                           // 向左移动
                pos--;
            }
            fflush(stdout);
            last_was_tab = 0;
        }
        
        // 处理 Ctrl+E（移到行尾）
        else if (ch == KEY_CTRL_E) {
            // 将光标移到行尾
            int len = strlen(buffer);
            while (pos < len) {
                putchar(buffer[pos]);                   // 打印字符并移动光标
                pos++;
            }
            fflush(stdout);
            last_was_tab = 0;
        }
        
        // 处理 Ctrl+L（清屏并重新显示当前行）
        else if (ch == KEY_CTRL_L) {
            // 使用 ANSI 转义序列清屏
            printf("\033[H\033[2J");                    // \033[H 移动光标到左上角, \033[2J 清屏
            fflush(stdout);
            // 重新显示提示符和当前输入
            if (prompt_callback != NULL) {
                prompt_callback();
            }
            printf("%s", buffer);
            // 将光标移到正确位置
            int len = strlen(buffer);
            for (int i = len; i > pos; i--) {
                printf("\b");
            }
            fflush(stdout);
            last_was_tab = 0;
        }
        
        // 处理 Ctrl+U（清除从行首到光标的内容）
        else if (ch == KEY_CTRL_U) {
            if (pos > 0) {
                int old_len = strlen(buffer);
                // 将光标后的内容移到行首
                int chars_to_remove = pos;
                int remaining = old_len - pos;
                memmove(buffer, buffer + pos, remaining + 1);  // +1 包含 '\0'
                pos = 0;
                // 刷新显示
                refresh_line(buffer, pos, old_len, prompt_callback);
            }
            last_was_tab = 0;
        }

        // 处理 Ctrl+R（反向搜索历史）
        else if (ch == KEY_CTRL_R) {
            // 保存当前输入，便于取消搜索时恢复
            char saved_buffer[4096];
            strncpy(saved_buffer, buffer, sizeof(saved_buffer) - 1);
            saved_buffer[sizeof(saved_buffer) - 1] = '\0';
            int saved_pos = pos;

            // 进入搜索模式
            char query[256];
            query[0] = '\0';
            int qlen = 0;

            const char *match = history_reverse_search(query);

            // 交互循环：直到 Enter 接受 / ESC 取消
            while (1) {
                // 刷新整行显示为搜索提示
                printf("\r\033[K");
                printf("(reverse-i-search)`%s`: %s", query, (match != NULL) ? match : "");
                fflush(stdout);

                int sch = getchar();

                // ESC：取消搜索，恢复原输入
                if (sch == KEY_ESC) {
                    int old_len = strlen(buffer);
                    strncpy(buffer, saved_buffer, size - 1);
                    buffer[size - 1] = '\0';
                    pos = (saved_pos <= (int)strlen(buffer)) ? saved_pos : (int)strlen(buffer);
                    refresh_line(buffer, pos, old_len, prompt_callback);
                    break;
                }

                // Ctrl+C：取消搜索并清空输入（与主循环 Ctrl+C 行为一致）
                if (sch == KEY_CTRL_C) {
                    printf("^C\n");
                    buffer[0] = '\0';
                    pos = 0;
                    history_index = -1;
                    // 恢复终端设置并返回空行
                    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);
                    return buffer;
                }

                // Enter：接受匹配（或空）填充到 buffer
                if (sch == KEY_NEWLINE || sch == KEY_RETURN) {
                    int old_len = strlen(buffer);
                    if (match != NULL) {
                        strncpy(buffer, match, size - 1);
                        buffer[size - 1] = '\0';
                    } else {
                        buffer[0] = '\0';
                    }
                    pos = strlen(buffer);
                    history_index = -1;
                    refresh_line(buffer, pos, old_len, prompt_callback);
                    break;
                }

                // Backspace：删除 query 最后一个字符
                if (sch == KEY_BACKSPACE || sch == 8) {
                    if (qlen > 0) {
                        qlen--;
                        query[qlen] = '\0';
                        match = history_reverse_search(query);
                    }
                    continue;
                }

                // 可打印字符：追加到 query
                if (sch >= 32 && sch < 127) {
                    if (qlen < (int)sizeof(query) - 1) {
                        query[qlen++] = (char)sch;
                        query[qlen] = '\0';
                        match = history_reverse_search(query);
                    }
                    continue;
                }

                // 其他控制字符忽略
            }

            last_was_tab = 0;
        }
        
        // 处理转义序列（箭头键等）
        else if (ch == KEY_ESC) {
            // 读取转义序列的后续字符
            int ch2 = getchar();
            if (ch2 == '[') {                           // CSI 序列（Control Sequence Introducer）
                int ch3 = getchar();
                
                // 上箭头：ESC [ A
                if (ch3 == 'A') {
                    const char* prev = history_prev(&history_index);
                    if (prev != NULL) {
                        pos = replace_line(buffer, size, prev, prompt_callback);
                    }
                    last_was_tab = 0;
                }
                
                // 下箭头：ESC [ B
                else if (ch3 == 'B') {
                    const char* next = history_next(&history_index);
                    if (next != NULL) {
                        pos = replace_line(buffer, size, next, prompt_callback);
                    } else {
                        // 已到最新，清空输入
                        pos = replace_line(buffer, size, "", prompt_callback);
                        history_index = -1;
                    }
                    last_was_tab = 0;
                }
                
                // 右箭头：ESC [ C
                else if (ch3 == 'C') {
                    if (pos < (int)strlen(buffer)) {
                        putchar(buffer[pos]);           // 打印字符
                        pos++;                          // 光标右移
                        fflush(stdout);
                    }
                    last_was_tab = 0;
                }
                
                // 左箭头：ESC [ D
                else if (ch3 == 'D') {
                    if (pos > 0) {
                        printf("\b");                   // 光标左移
                        pos--;
                        fflush(stdout);
                    }
                    last_was_tab = 0;
                }
                
                // Home 键：ESC [ H 或 ESC [ 1 ~（移到行首）
                else if (ch3 == 'H') {
                    // 将光标移到行首
                    while (pos > 0) {
                        printf("\b");                   // 向左移动
                        pos--;
                    }
                    fflush(stdout);
                    last_was_tab = 0;
                }
                
                // End 键：ESC [ F 或 ESC [ 4 ~（移到行尾）
                else if (ch3 == 'F') {
                    // 将光标移到行尾
                    int len = strlen(buffer);
                    while (pos < len) {
                        putchar(buffer[pos]);           // 打印字符并移动光标
                        pos++;
                    }
                    fflush(stdout);
                    last_was_tab = 0;
                }
                
                // 处理 ESC [ 数字 ~ 格式的键（Home=1~, End=4~, 等）
                else if (ch3 >= '0' && ch3 <= '9') {
                    int ch4 = getchar();                // 读取 ~
                    if (ch4 == '~') {
                        if (ch3 == '1') {               // Home 键：ESC [ 1 ~
                            while (pos > 0) {
                                printf("\b");
                                pos--;
                            }
                            fflush(stdout);
                        }
                        else if (ch3 == '4') {          // End 键：ESC [ 4 ~
                            int len = strlen(buffer);
                            while (pos < len) {
                                putchar(buffer[pos]);
                                pos++;
                            }
                            fflush(stdout);
                        }
                    }
                    last_was_tab = 0;
                }
            }
        }
        
        // 处理回车键
        else if (ch == KEY_NEWLINE || ch == KEY_RETURN) {
            // 回车键 - 完成输入
            printf("\n");                               // 打印换行符（移动到下一行）
            buffer[pos] = '\0';                         // 添加字符串结束符
            last_was_tab = 0;                           // 重置 Tab 状态
            break;                                      // 跳出循环，返回输入的字符串
        }
        
        // 处理退格键
        else if (ch == KEY_BACKSPACE || ch == 8) {
            // 退格键（ASCII 127 或 8）
            if (pos > 0) {                              // 如果有字符可以删除
                int old_len = strlen(buffer);
                pos--;                                  // 光标位置后退
                
                // 如果光标不在末尾，需要将后面的字符向前移动
                if (pos < old_len - 1) {
                    // 将光标后面的字符向前移动一位
                    for (int i = pos; i < old_len; i++) {
                        buffer[i] = buffer[i + 1];
                    }
                } else {
                    // 光标在末尾，直接截断
                    buffer[pos] = '\0';
                }
                
                // 重新显示整行（使用 refresh_line 确保显示正确）
                int new_len = strlen(buffer);
                refresh_line(buffer, pos, old_len, prompt_callback);
            }
            last_was_tab = 0;                           // 重置 Tab 状态（输入改变了）
        }
        
        // 处理 Tab 键（核心补全逻辑）
        else if (ch == KEY_TAB) {
            // Tab 键 - 执行补全
            buffer[pos] = '\0';                         // 确保字符串以 \0 结尾
            
            // 步骤1：检查是否为双击 Tab
            // 条件：上次按键也是 Tab，且输入内容没有变化
            int is_double_tab = (last_was_tab && strcmp(buffer, last_input) == 0);
            
            // 步骤2：保存当前输入状态（用于下次判断是否为双击）
            strcpy(last_input, buffer);                 // 保存当前输入
            last_was_tab = 1;                           // 标记本次按键为 Tab
            
            // 步骤3：获取补全建议（使用智能补全）
            char **matches = NULL;                      // 匹配项数组指针
            int count = get_smart_completions(buffer, pos, &matches); // 调用智能补全函数
            
            // 情况1：没有匹配项
            if (count == 0) {
                // 没有匹配项 - 响铃提示用户
                printf("\a");                           // \a 是响铃字符（BEL）
                fflush(stdout);                         // 刷新输出
            }
            
            // 情况2：只有一个匹配 - 直接补全
            else if (count == 1) {
                // 只有一个匹配 - 自动补全
                char prefix[4096];                      // 目录前缀
                char partial[4096];                     // 部分文件名
                extract_path_to_complete(buffer, pos, prefix, partial); // 提取路径部分
                
                // 计算需要补全的部分（去掉已输入的部分）
                int partial_len = strlen(partial);      // 已输入的长度
                const char *to_add = matches[0] + partial_len; // 需要添加的部分
                int add_len = strlen(to_add);           // 需要添加的长度
                
                // 检查缓冲区空间是否足够
                if (pos + add_len < (int)size - 1) {    // 确保不溢出
                    // 添加补全部分到缓冲区
                    strcpy(buffer + pos, to_add);       // 追加到 buffer 末尾
                    pos += add_len;                     // 更新光标位置
                    
                    // 在终端上显示补全的部分
                    printf("%s", to_add);               // 打印补全内容
                    fflush(stdout);                     // 刷新输出
                    
                    // 更新 last_input（因为内容变了）
                    strcpy(last_input, buffer);         // 保存新的输入内容
                }
                
                free_completions(matches, count);       // 释放匹配项内存
            }
            
            // 情况3：多个匹配
            else {
                // 有多个匹配项
                
                // 第一次 Tab：尝试补全公共前缀
                if (!is_double_tab) {
                    // 不是双击 Tab，尝试补全公共前缀
                    char prefix[4096];                  // 目录前缀
                    char partial[4096];                 // 部分文件名
                    extract_path_to_complete(buffer, pos, prefix, partial);
                    
                    // 计算所有匹配项的公共前缀
                    char common[4096];                  // 公共前缀缓冲区
                    get_common_prefix(matches, count, common, sizeof(common));
                    
                    int partial_len = strlen(partial);  // 已输入的长度
                    int common_len = strlen(common);    // 公共前缀长度
                    
                    if (common_len > partial_len) {
                        // 有更长的公共前缀可以补全
                        const char *to_add = common + partial_len; // 需要添加的部分
                        int add_len = strlen(to_add);   // 添加的长度
                        
                        if (pos + add_len < (int)size - 1) { // 检查缓冲区空间
                            strcpy(buffer + pos, to_add); // 追加到 buffer
                            pos += add_len;             // 更新光标位置
                            printf("%s", to_add);       // 显示补全内容
                            fflush(stdout);             // 刷新输出
                            strcpy(last_input, buffer); // 更新 last_input
                        }
                    } else {
                        // 没有更多公共前缀可以补全
                        // 响铃提示用户再按一次 Tab 查看所有选项
                        printf("\a");                   // 响铃
                        fflush(stdout);                 // 刷新输出
                    }
                    
                    free_completions(matches, count);   // 释放内存
                }
                
                // 第二次 Tab：显示所有匹配选项
                else {
                    // 是双击 Tab，显示所有匹配项
                    printf("\n");                       // 换行（开始显示列表）
                    
                    // 打印所有匹配项（每行显示 5 个）
                    for (int i = 0; i < count; i++) {
                        printf("%s  ", matches[i]);     // 打印匹配项，两个空格分隔
                        if ((i + 1) % 5 == 0) {         // 每显示 5 个换行
                            printf("\n");               // 换行
                        }
                    }
                    if (count % 5 != 0) {               // 如果最后一行不满 5 个
                        printf("\n");                   // 补充换行
                    }
                    
                    // 重新显示提示符和当前输入
                    if (prompt_callback != NULL) {      // 如果有提示符回调函数
                        prompt_callback();              // 调用回调显示提示符
                    }
                    printf("%s", buffer);               // 重新显示当前输入
                    fflush(stdout);                     // 刷新输出
                    
                    free_completions(matches, count);   // 释放内存
                    last_was_tab = 0;                   // 重置状态（避免第三次 Tab 再次显示）
                }
            }
        }
        
        // 处理可打印字符
        else if (ch >= 32 && ch < 127) {
            // 可打印字符（ASCII 32-126，即空格到 ~）
            int old_len = strlen(buffer);
            if (pos < (int)size - 1) {                  // 检查缓冲区是否还有空间
                // 如果光标不在末尾，需要将后面的字符向后移动
                if (pos < old_len) {
                    // 将光标位置及后面的字符向后移动一位
                    // 注意：需要移动包括 '\0' 在内的所有字符
                    // 从后往前移动，避免覆盖
                    for (int i = old_len; i >= pos; i--) {
                        if (i + 1 < (int)size) {  // 确保不越界
                            buffer[i + 1] = buffer[i];
                        }
                    }
                    // 在光标位置插入新字符
                    buffer[pos] = ch;
                    pos++;                              // 光标位置前进
                    // 确保字符串以 \0 结尾（移动时已经保留了，但再次确认）
                    if (pos < (int)size) {
                        // 不需要设置，因为移动时已经保留了 '\0'
                    }
                    
                    // 光标在中间位置，需要重新显示整行
                    // 确保缓冲区以 \0 结尾
                    if (pos < (int)size) {
                        buffer[pos] = '\0';
                    } else {
                        buffer[size - 1] = '\0';
                    }
                    int new_len = strlen(buffer);
                    refresh_line(buffer, pos, old_len, prompt_callback);
                } else {
                    // 光标在末尾，直接追加（不需要刷新整行，直接打印即可）
                    buffer[pos] = ch;
                    pos++;                              // 光标位置前进
                    if (pos < (int)size) {
                        buffer[pos] = '\0';             // 确保字符串以 \0 结尾
                    }
                    // 直接打印字符，不需要刷新整行
                    printf("%c", ch);
                    fflush(stdout);
                }
            }
            last_was_tab = 0;                           // 重置 Tab 状态（输入改变了）
        }
        // 忽略其他控制字符（如方向键等）
    }
    
    // 步骤7：恢复终端设置
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);        // 恢复为原始终端设置
    
    // 步骤8：返回读取的字符串
    return buffer;                                      // 返回 buffer 指针
}
