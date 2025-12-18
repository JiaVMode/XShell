/*
 * alias.c - 命令别名管理实现
 * 
 * 功能：管理Shell命令别名
 */

#include "alias.h"
#include <stdio.h>
#include <string.h>

// 别名表（静态数组）
static Alias aliases[MAX_ALIASES];
static int alias_count_value = 0;

// 初始化别名系统
int alias_init(void) {
    alias_count_value = 0;
    memset(aliases, 0, sizeof(aliases));
    return 0;
}

// 添加或更新别名
int alias_set(const char *name, const char *value) {
    if (name == NULL || value == NULL) {
        return -1;
    }
    
    // 检查名称和值的长度
    if (strlen(name) >= sizeof(aliases[0].name) ||
        strlen(value) >= sizeof(aliases[0].value)) {
        return -1;
    }
    
    // 检查是否已存在，如果存在则更新
    for (int i = 0; i < alias_count_value; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            strncpy(aliases[i].value, value, sizeof(aliases[i].value) - 1);
            aliases[i].value[sizeof(aliases[i].value) - 1] = '\0';
            return 0;
        }
    }
    
    // 添加新别名
    if (alias_count_value >= MAX_ALIASES) {
        return -1;
    }
    
    strncpy(aliases[alias_count_value].name, name, sizeof(aliases[alias_count_value].name) - 1);
    aliases[alias_count_value].name[sizeof(aliases[alias_count_value].name) - 1] = '\0';
    
    strncpy(aliases[alias_count_value].value, value, sizeof(aliases[alias_count_value].value) - 1);
    aliases[alias_count_value].value[sizeof(aliases[alias_count_value].value) - 1] = '\0';
    
    alias_count_value++;
    return 0;
}

// 获取别名对应的命令
const char* alias_get(const char *name) {
    if (name == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < alias_count_value; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            return aliases[i].value;
        }
    }
    
    return NULL;
}

// 删除别名
int alias_remove(const char *name) {
    if (name == NULL) {
        return -1;
    }
    
    // 查找别名
    for (int i = 0; i < alias_count_value; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            // 找到了，将后面的别名前移
            for (int j = i; j < alias_count_value - 1; j++) {
                aliases[j] = aliases[j + 1];
            }
            alias_count_value--;
            // 清空最后一个位置
            memset(&aliases[alias_count_value], 0, sizeof(Alias));
            return 0;
        }
    }
    
    // 未找到
    return -1;
}

// 列出所有别名
void alias_list(void) {
    if (alias_count_value == 0) {
        // 没有别名
        return;
    }
    
    for (int i = 0; i < alias_count_value; i++) {
        printf("alias %s='%s'\n", aliases[i].name, aliases[i].value);
    }
}

// 获取别名数量
int alias_count(void) {
    return alias_count_value;
}

// 清理别名系统
void alias_cleanup(void) {
    alias_count_value = 0;
    memset(aliases, 0, sizeof(aliases));
}




