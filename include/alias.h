/*
 * alias.h - 命令别名管理
 * 
 * 功能：管理Shell命令别名
 * 
 * 别名允许用户为常用命令创建简短的替代名称
 * 例如：alias ll="xls -lah"
 */

#ifndef ALIAS_H
#define ALIAS_H

// 最大别名数量
#define MAX_ALIASES 100

// 别名结构体
typedef struct {
    char name[64];      // 别名名称
    char value[256];    // 别名对应的命令
} Alias;

// 别名初始化
// 返回：0=成功，-1=失败
int alias_init(void);

// 添加或更新别名
// 参数：
//   name  - 别名名称
//   value - 别名对应的命令
// 返回：0=成功，-1=失败
int alias_set(const char *name, const char *value);

// 获取别名对应的命令
// 参数：
//   name - 别名名称
// 返回：别名对应的命令，如果不存在返回NULL
const char* alias_get(const char *name);

// 删除别名
// 参数：
//   name - 别名名称
// 返回：0=成功，-1=失败（别名不存在）
int alias_remove(const char *name);

// 列出所有别名
// 参数：无
// 返回：无
void alias_list(void);

// 获取别名数量
// 返回：当前别名数量
int alias_count(void);

// 清理别名系统
// 释放所有资源
void alias_cleanup(void);

#endif // ALIAS_H




