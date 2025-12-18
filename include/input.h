// 头文件保护：防止重复包含
#ifndef INPUT_H                                         // 如果 INPUT_H 未定义
#define INPUT_H                                         // 则定义 INPUT_H

#include <stddef.h>                                     // size_t 类型定义

// 输入处理模块
// 功能说明：实现带 Tab 补全功能的用户输入处理
// 核心特性：
//   1. 逐字符读取（不等待回车）
//   2. Tab 键触发路径补全
//   3. 支持退格键编辑
//   4. 支持 Ctrl+D 退出

// 提示符显示回调函数类型
// 用途：在显示补全选项后，重新显示命令提示符
// 使用场景：当用户按两次 Tab 显示所有匹配项后，需要重新显示提示符
// 例如：
//   [\home]# xcd l<Tab><Tab>
//   lab/  lost+found/          <- 显示匹配项
//   [\home]# xcd l              <- 回调函数显示这一行
typedef void (*PromptCallback)(void);

// 读取带 Tab 补全功能的用户输入
// 功能：替代标准的 fgets()，提供更强大的交互式输入功能
// 工作原理：
//   1. 设置终端为原始模式（逐字符读取）
//   2. 捕获特殊按键（Tab, 退格, Ctrl+D, 回车）
//   3. Tab 键触发路径补全逻辑
//   4. 回车时恢复终端设置并返回
// 参数：
//   - buffer: 存储用户输入的缓冲区（由调用者分配）
//   - size: 缓冲区大小（字节数）
//   - prompt_callback: 显示提示符的回调函数（可为 NULL）
//                      当需要重新显示提示符时会调用此函数
// 返回值：
//   - 成功：返回 buffer 指针（指向输入的字符串）
//   - EOF/Ctrl+D：返回 NULL
// 注意事项：
//   1. 此函数会修改终端设置（进入原始模式）
//   2. 返回前会自动恢复终端设置
//   3. buffer 必须足够大以容纳用户输入
//   4. 返回的字符串不包含换行符
char* read_line_with_completion(char *buffer, size_t size, PromptCallback prompt_callback);

#endif // INPUT_H                                       // 头文件保护结束
