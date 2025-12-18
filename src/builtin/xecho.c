// ============================================
// xecho 命令实现文件
// 功能：模拟 Ubuntu echo 命令，输出字符串到标准输出
// 对应系统命令：echo
// ============================================

// 引入自定义头文件
#include "builtin.h"                        // 内置命令函数声明
#include "utils.h"                          // 工具函数（彩色输出）

// 引入标准库
#include <stdio.h>                          // 标准输入输出（printf, putchar）
#include <stdlib.h>                         // 标准库函数（strtol）
#include <string.h>                         // 字符串处理（strcmp）
#include <ctype.h>                          // 字符判断（isdigit）

// ============================================
// 辅助函数：解释转义字符
// ============================================
// 功能：将字符串中的转义序列（如 \n, \t）转换为实际字符
// 参数：
//   - str: 输入字符串（包含转义序列）
//   - stop_output: 输出参数，如果遇到 \c 则设置为 1（停止后续输出）
// 返回：无（直接输出到 stdout）
//
// 支持的转义序列：
//   \\  - 反斜杠
//   \a  - 响铃（BEL，ASCII 7）
//   \b  - 退格（Backspace）
//   \c  - 停止输出（不输出后续内容，包括换行符）
//   \e  - 转义字符（ESC，ASCII 27）
//   \f  - 换页（Form Feed）
//   \n  - 换行（New Line）
//   \r  - 回车（Carriage Return）
//   \t  - 水平制表符（Tab）
//   \v  - 垂直制表符（Vertical Tab）
//   \0nnn  - 八进制值（1-3位数字，如 \0101 = 'A'）
//   \xHH   - 十六进制值（1-2位数字，如 \x41 = 'A'）
// ============================================

static void print_with_escapes(const char *str, int *stop_output) {
    // 遍历字符串中的每个字符
    for (int i = 0; str[i] != '\0'; i++) {
        // 如果当前字符不是反斜杠，直接输出
        if (str[i] != '\\') {
            putchar(str[i]);                // 输出普通字符
            continue;                       // 继续下一个字符
        }

        // 如果是反斜杠，检查下一个字符（转义序列）
        i++;                                // 移动到反斜杠后的字符
        
        // 如果反斜杠是字符串的最后一个字符，直接输出反斜杠
        if (str[i] == '\0') {
            putchar('\\');                  // 输出反斜杠本身
            break;                          // 字符串结束
        }

        // 根据转义字符类型进行处理
        switch (str[i]) {
            case '\\':                      // \\ → 反斜杠
                putchar('\\');
                break;
            case 'a':                       // \a → 响铃（ASCII 7）
                putchar('\a');
                break;
            case 'b':                       // \b → 退格
                putchar('\b');
                break;
            case 'c':                       // \c → 停止输出
                *stop_output = 1;           // 设置停止标志
                return;                     // 立即返回，不再输出任何内容
            case 'e':                       // \e → ESC（ASCII 27）
                putchar('\033');            // 八进制 033 = 十进制 27
                break;
            case 'f':                       // \f → 换页
                putchar('\f');
                break;
            case 'n':                       // \n → 换行
                putchar('\n');
                break;
            case 'r':                       // \r → 回车
                putchar('\r');
                break;
            case 't':                       // \t → 水平制表符
                putchar('\t');
                break;
            case 'v':                       // \v → 垂直制表符
                putchar('\v');
                break;
            
            // 八进制转义序列：\0nnn（1-3位八进制数字）
            case '0': {
                // 解析八进制数值（最多3位）
                // 注意：\0 后面的数字才是八进制数字，所以从 i+1 开始读取
                int octal_value = 0;        // 八进制值累加器
                int j = i + 1;              // 从 '0' 后面的位置开始
                int digits_read = 0;        // 已读取的位数
                
                // 最多读取3位八进制数字（0-7）
                while (digits_read < 3 && str[j] >= '0' && str[j] <= '7') {
                    octal_value = octal_value * 8 + (str[j] - '0');
                    j++;
                    digits_read++;
                }
                
                // 如果至少读取了1位数字，输出转换后的字符
                if (digits_read > 0) {
                    putchar(octal_value & 0xFF);
                    i = j - 1;              // 更新索引（-1 是因为循环会 i++）
                } else {
                    // 如果没有读取到数字，输出 \0（空字符）
                    putchar('\0');
                }
                break;
            }
            
            // 十六进制转义序列：\xHH（1-2位十六进制数字）
            case 'x': {
                // 解析十六进制数值（最多2位）
                int hex_value = 0;          // 十六进制值累加器
                int j = i + 1;              // 跳过 'x'，指向第一位数字
                
                // 最多读取2位十六进制数字（0-9, A-F, a-f）
                while (j < i + 3 && isxdigit((unsigned char)str[j])) {
                    // 将十六进制字符转换为数值
                    if (str[j] >= '0' && str[j] <= '9') {
                        hex_value = hex_value * 16 + (str[j] - '0');
                    } else if (str[j] >= 'a' && str[j] <= 'f') {
                        hex_value = hex_value * 16 + (str[j] - 'a' + 10);
                    } else if (str[j] >= 'A' && str[j] <= 'F') {
                        hex_value = hex_value * 16 + (str[j] - 'A' + 10);
                    }
                    j++;
                }
                
                // 如果没有有效的十六进制数字，输出 \x 字面量
                if (j == i + 1) {
                    putchar('\\');          // 输出反斜杠
                    putchar('x');           // 输出 x
                } else {
                    // 输出转换后的字符
                    putchar(hex_value & 0xFF);
                    i = j - 1;              // 更新索引
                }
                break;
            }
            
            // 未识别的转义序列：输出反斜杠和字符本身
            default:
                putchar('\\');              // 输出反斜杠
                putchar(str[i]);            // 输出原字符
                break;
        }
    }
}

// ============================================
// xecho 命令实现函数
// ============================================
// 命令名称：xecho
// 对应系统命令：echo
// 功能：输出字符串到标准输出
// 用法：xecho [选项] [字符串...]
//
// 选项：
//   -n      不输出换行符（默认会在末尾输出 \n）
//   -e      启用转义字符解释（\n, \t, \\, 等）
//   -E      禁用转义字符解释（默认行为）
//
// 示例：
//   xecho Hello World          → 输出: Hello World\n
//   xecho -n Hello World       → 输出: Hello World（无换行）
//   xecho -e "Line1\nLine2"    → 输出: Line1 换行 Line2
//   xecho -e "Tab\there"       → 输出: Tab 制表符 here
//   xecho -ne "No\nnewline"    → 输出: No 换行 newline（无末尾换行）
//
// 返回值：
//   0  - 成功执行
//  -1  - 执行失败
// ============================================

int cmd_xecho(Command *cmd, ShellContext *ctx) {
    // 未使用参数标记（避免编译器警告）
    (void)ctx;                              // ctx 在 xecho 命令中未使用

    // 步骤0：参数有效性检查
    if (cmd == NULL || cmd->args == NULL) { // 命令对象或参数数组为空
        return -1;                          // 返回失败状态
    }

    // 步骤0.5：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xecho - 输出字符串到标准输出\n\n");
        printf("用法:\n");
        printf("  xecho [选项] [字符串...] [--help]\n\n");
        printf("说明:\n");
        printf("  输出字符串到标准输出。\n");
        printf("  Echo - 回显字符串。\n\n");
        printf("选项:\n");
        printf("  -n        不输出换行符（默认会在末尾输出换行）\n");
        printf("  -e        启用转义字符解释\n");
        printf("  -E        禁用转义字符解释（默认）\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("转义序列（需要 -e 选项）:\n");
        printf("  \\\\        反斜杠\n");
        printf("  \\a        响铃（BEL）\n");
        printf("  \\b        退格\n");
        printf("  \\c        停止输出（包括换行符）\n");
        printf("  \\e        ESC 字符\n");
        printf("  \\f        换页\n");
        printf("  \\n        换行\n");
        printf("  \\r        回车\n");
        printf("  \\t        水平制表符\n");
        printf("  \\v        垂直制表符\n");
        printf("  \\0nnn     八进制值（1-3 位）\n");
        printf("  \\xHH      十六进制值（1-2 位）\n\n");
        printf("示例:\n");
        printf("  xecho Hello World            # 基本输出\n");
        printf("  xecho -n Hello               # 不换行\n");
        printf("  xecho -e \"Line1\\nLine2\"     # 多行\n");
        printf("  xecho -e \"Tab\\there\"        # 制表符\n");
        printf("  xecho -ne \"No\\nnewline\"     # 组合选项\n");
        printf("  xecho -e \"\\x48\\x65\\x6c\\x6c\\x6f\"  # 十六进制（Hello）\n");
        printf("  xecho -c red \"Error message\"  # 红色输出\n");
        printf("  xecho -c green \"Success\"     # 绿色输出\n\n");
        printf("对应系统命令: echo\n");
        return 0;
    }

    // 步骤1：解析选项
    int no_newline = 0;                     // 是否不输出换行符（-n）
    int interpret_escapes = 0;              // 是否解释转义字符（-e）
    const char *color = NULL;                // 颜色选项（-c）
    int start_index = 1;                    // 开始输出的参数索引

    // 遍历所有选项参数（以 - 开头且在参数开始位置的）
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *arg = cmd->args[i];     // 当前参数
        
        // 如果参数不以 - 开头，说明选项解析结束
        if (arg[0] != '-' || arg[1] == '\0') {
            start_index = i;                // 记录第一个非选项参数的位置
            break;
        }
        
        // 检查 -c 选项（需要参数）
        if (strcmp(arg, "-c") == 0) {
            if (i + 1 < cmd->arg_count) {
                color = cmd->args[i + 1];
                i++;  // 跳过颜色参数
                start_index = i + 1;
                continue;
            }
        }
        
        // 解析选项字符（支持组合选项，如 -ne）
        for (int j = 1; arg[j] != '\0'; j++) {
            switch (arg[j]) {
                case 'n':                   // -n：不输出换行符
                    no_newline = 1;
                    break;
                case 'e':                   // -e：启用转义字符解释
                    interpret_escapes = 1;
                    break;
                case 'E':                   // -E：禁用转义字符解释（默认）
                    interpret_escapes = 0;
                    break;
                default:
                    // 未知选项：当作普通字符串处理
                    // 设置起始索引并结束选项解析
                    start_index = i;
                    goto end_option_parsing; // 跳出双层循环
            }
        }
        
        start_index = i + 1;                // 更新起始索引
    }
    
end_option_parsing:
    
    // 如果指定了颜色，输出颜色代码
    if (color != NULL) {
        printf("%s", set_color(color));
    }

    // 步骤3：输出所有参数
    int stop_output = 0;                    // 是否停止输出（遇到 \c 时）
    
    for (int i = start_index; i < cmd->arg_count && !stop_output; i++) {
        // 根据选项决定输出方式
        if (interpret_escapes) {
            // 启用转义字符解释
            print_with_escapes(cmd->args[i], &stop_output);
        } else {
            // 不解释转义字符，直接输出
            printf("%s", cmd->args[i]);
        }
        
        // 如果不是最后一个参数且未停止输出，输出空格分隔符
        if (i < cmd->arg_count - 1 && !stop_output) {
            putchar(' ');
        }
    }

    // 步骤4：根据选项决定是否输出换行符
    // 如果没有 -n 选项且未遇到 \c，则输出换行符
    if (!no_newline && !stop_output) {
        putchar('\n');
    }
    
    // 如果指定了颜色，重置颜色
    if (color != NULL) {
        printf("%s", reset_color());
    }
    
    // 步骤5：如果使用了 \c 停止输出，确保输出缓冲区被刷新
    // 这样即使没有换行符，输出也会立即显示
    if (stop_output) {
        fflush(stdout);
    }

    // 步骤6：返回成功状态
    return 0;
}

// ============================================
// 功能说明和技术要点
// ============================================

// 1. **选项解析**：
//    - 支持 -n 选项：不输出换行符
//    - 支持 -e 选项：启用转义字符解释
//    - 支持 -E 选项：禁用转义字符解释（默认行为）
//    - 支持组合选项：-ne, -en, -nE 等
//    - 选项必须在字符串参数之前
//
// 2. **转义字符解释**（-e 选项）：
//    - \\  - 反斜杠
//    - \a  - 响铃（BEL）
//    - \b  - 退格
//    - \c  - 停止输出（包括换行符）
//    - \e  - ESC 字符
//    - \f  - 换页
//    - \n  - 换行
//    - \r  - 回车
//    - \t  - 水平制表符
//    - \v  - 垂直制表符
//    - \0nnn - 八进制值（如 \0101 = 'A'）
//    - \xHH - 十六进制值（如 \x41 = 'A'）
//
// 3. **与 Ubuntu echo 对比**：
//    - ✅ 输出字符串
//    - ✅ 支持 -n 选项
//    - ✅ 支持 -e 选项（解释转义字符）
//    - ✅ 支持 -E 选项（不解释转义字符）
//    - ✅ 支持组合选项（-ne）
//    - ✅ 支持所有标准转义序列
//    - ✅ 支持八进制和十六进制转义
//    - ✅ 支持 \c 停止输出
//
// 4. **使用示例**：
//    - xecho -e "Line1\nLine2"        → 两行输出
//    - xecho -e "Tab\there"           → 制表符分隔
//    - xecho -e "Hello\c World"       → 只输出 Hello
//    - xecho -e "\x48\x65\x6C\x6C\x6F" → 输出 Hello（十六进制）
//    - xecho -e "\0110\0145\0154\0154\0157" → 输出 Hello（八进制）
//    - xecho -ne "Progress: 50%\r"    → 无换行，可用于进度条
//
// 5. **性能优化**：
//    - 使用 putchar 输出单个字符（高效）
//    - 避免动态内存分配
//    - O(n) 时间复杂度，n 为参数总字符数
//
// 6. **错误处理**：
//    - 无效的八进制/十六进制序列会按字面量输出
//    - 未识别的转义序列输出反斜杠和原字符
//
// ============================================
