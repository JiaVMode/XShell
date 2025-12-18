// 头文件保护：防止重复包含
#ifndef HISTORY_H
#define HISTORY_H

// 历史记录最大条数
#define MAX_HISTORY 1000

// ============================================
// 历史记录模块
// ============================================
// 功能：记录和管理用户输入的命令历史
// 用途：
//   1. 记录每条执行的命令
//   2. 显示历史命令列表（xhistory）
//   3. 持久化存储历史（保存到文件）
// ============================================

// 函数声明

// 初始化历史记录系统
// 参数：无
// 返回值：0-成功，-1-失败
int history_init(void);

// 添加命令到历史记录
// 参数：
//   line: 要添加的命令行字符串
// 返回值：无
void history_add(const char *line);

// 显示所有历史记录
// 参数：无
// 返回值：无
void history_show(void);

// 获取历史记录数量
// 参数：无
// 返回值：历史记录条数
int history_count(void);

// 获取指定索引的历史记录
// 参数：
//   index: 历史记录索引（0 为最旧，count-1 为最新）
// 返回值：历史记录字符串，如果索引无效返回 NULL
const char* history_get(int index);

// 获取上一条历史记录（用于上箭头）
// 参数：
//   current_index: 指向当前索引的指针，会被更新
// 返回值：历史记录字符串，如果已到最旧返回 NULL
const char* history_prev(int *current_index);

// 获取下一条历史记录（用于下箭头）
// 参数：
//   current_index: 指向当前索引的指针，会被更新
// 返回值：历史记录字符串，如果已到最新返回 NULL
const char* history_next(int *current_index);

// 保存历史记录到文件
// 参数：无
// 返回值：0-成功，-1-失败
int history_save(void);

// 从文件加载历史记录
// 参数：无
// 返回值：0-成功，-1-失败
int history_load(void);

// 清理历史记录系统
// 参数：无
// 返回值：无
void history_cleanup(void);

#endif // HISTORY_H

