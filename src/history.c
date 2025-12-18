// 特性测试宏：启用 POSIX 标准函数（strdup）
#define _POSIX_C_SOURCE 200809L     // POSIX.1-2008 标准

// 引入自定义头文件
#include "history.h"                // 历史记录模块声明

// 引入标准库
#include <stdio.h>                  // 标准输入输出（printf, fopen, fclose）
#include <stdlib.h>                 // 标准库（malloc, free）
#include <string.h>                 // 字符串处理（strdup, strcmp, strlen）
#include <unistd.h>                 // UNIX 标准函数（getenv）

// ============================================
// 历史记录数据结构
// ============================================
static char *history[MAX_HISTORY];  // 历史记录数组（存储命令字符串指针）
static int history_count_value = 0; // 当前历史记录数量
static char history_file[256];      // 历史文件路径

// ============================================
// 初始化历史记录系统
// ============================================
int history_init(void) {
    // 步骤1：构建历史文件路径（项目启动目录的绝对路径 + .xshell_history）
    // 说明：保存启动时的绝对路径，避免因 cd 命令导致路径变化
    char cwd[256];                              // 当前工作目录缓冲区
    if (getcwd(cwd, sizeof(cwd)) != NULL) {     // 获取当前工作目录
        snprintf(history_file, sizeof(history_file), "%s/.xshell_history", cwd);
    } else {                                    // 获取失败，使用相对路径
        snprintf(history_file, sizeof(history_file), ".xshell_history");
    }

    // 步骤2：初始化历史记录数组
    for (int i = 0; i < MAX_HISTORY; i++) {
        history[i] = NULL;                      // 初始化为 NULL
    }

    // 步骤3：从文件加载历史记录
    history_load();                             // 加载之前保存的历史

    return 0;                                   // 返回成功
}

// ============================================
// 添加命令到历史记录
// ============================================
void history_add(const char *line) {
    // 步骤1：参数检查
    if (line == NULL || strlen(line) == 0) {    // 空命令不记录
        return;
    }

    // 步骤2：跳过只包含空白字符的命令
    int has_content = 0;
    for (size_t i = 0; i < strlen(line); i++) {
        if (line[i] != ' ' && line[i] != '\t' && line[i] != '\n') {
            has_content = 1;
            break;
        }
    }
    if (!has_content) {                         // 只有空白字符，不记录
        return;
    }

    // 步骤3：去掉末尾的换行符
    char *line_copy = strdup(line);             // 复制字符串
    if (line_copy == NULL) {                    // 内存分配失败
        return;
    }
    size_t len = strlen(line_copy);
    if (len > 0 && line_copy[len - 1] == '\n') {
        line_copy[len - 1] = '\0';              // 去掉换行符
    }

    // 步骤4：检查是否与上一条命令重复
    if (history_count_value > 0 && 
        strcmp(history[history_count_value - 1], line_copy) == 0) {
        free(line_copy);                        // 重复命令不记录
        return;
    }

    // 步骤5：添加到历史记录
    if (history_count_value < MAX_HISTORY) {    // 未满
        history[history_count_value] = line_copy;
        history_count_value++;
    } else {                                    // 已满，删除最旧的
        free(history[0]);                       // 释放最旧的记录
        // 所有记录向前移动一位
        for (int i = 0; i < MAX_HISTORY - 1; i++) {
            history[i] = history[i + 1];
        }
        history[MAX_HISTORY - 1] = line_copy;   // 添加新记录到末尾
    }
}

// ============================================
// 显示所有历史记录
// ============================================
void history_show(void) {
    // 遍历所有历史记录并打印
    for (int i = 0; i < history_count_value; i++) {
        printf("%5d  %s\n", i + 1, history[i]); // 打印序号和命令
    }
}

// ============================================
// 获取历史记录数量
// ============================================
int history_count(void) {
    return history_count_value;                 // 返回当前记录数量
}

// ============================================
// 获取指定索引的历史记录
// ============================================
const char* history_get(int index) {
    // 参数检查：索引是否有效
    if (index < 0 || index >= history_count_value) {
        return NULL;                            // 无效索引
    }
    return history[index];                      // 返回指定索引的历史记录
}

// ============================================
// 获取上一条历史记录（用于上箭头）
// ============================================
const char* history_prev(int *current_index) {
    // 步骤1：参数检查
    if (current_index == NULL) {
        return NULL;
    }
    
    // 步骤2：初始化索引（第一次调用时）
    if (*current_index == -1) {                 // 尚未开始浏览
        *current_index = history_count_value;   // 从最新位置开始
    }
    
    // 步骤3：向上移动（更旧的命令）
    if (*current_index > 0) {                   // 还有更旧的命令
        (*current_index)--;                     // 索引减1
        return history[*current_index];         // 返回历史记录
    }
    
    // 步骤4：已到最旧，无法继续向上
    return NULL;
}

// ============================================
// 获取下一条历史记录（用于下箭头）
// ============================================
const char* history_next(int *current_index) {
    // 步骤1：参数检查
    if (current_index == NULL || *current_index == -1) {
        return NULL;                            // 无效状态
    }
    
    // 步骤2：向下移动（更新的命令）
    if (*current_index < history_count_value - 1) {  // 还有更新的命令
        (*current_index)++;                     // 索引加1
        return history[*current_index];         // 返回历史记录
    }
    
    // 步骤3：已到最新，返回空字符串（清空输入）
    *current_index = history_count_value;       // 设置为"最新"位置
    return "";                                  // 返回空字符串
}

// ============================================
// 保存历史记录到文件
// ============================================
int history_save(void) {
    // 步骤1：打开历史文件（写模式）
    FILE *fp = fopen(history_file, "w");
    if (fp == NULL) {                           // 打开失败
        return -1;                              // 返回失败（不是致命错误）
    }

    // 步骤2：写入所有历史记录
    for (int i = 0; i < history_count_value; i++) {
        fprintf(fp, "%s\n", history[i]);        // 每条命令一行
    }

    // 步骤3：关闭文件
    fclose(fp);
    return 0;                                   // 返回成功
}

// ============================================
// 从文件加载历史记录
// ============================================
int history_load(void) {
    // 步骤1：打开历史文件（读模式）
    FILE *fp = fopen(history_file, "r");
    if (fp == NULL) {                           // 文件不存在或无法打开
        return 0;                               // 不是错误（首次运行）
    }

    // 步骤2：逐行读取历史记录
    char line[1024];                            // 行缓冲区
    while (fgets(line, sizeof(line), fp) != NULL && history_count_value < MAX_HISTORY) {
        // 去掉换行符
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        // 添加到历史记录（不检查重复，因为是从文件加载）
        if (strlen(line) > 0) {
            history[history_count_value] = strdup(line);
            if (history[history_count_value] != NULL) {
                history_count_value++;
            }
        }
    }

    // 步骤3：关闭文件
    fclose(fp);
    return 0;                                   // 返回成功
}

// ============================================
// 清理历史记录系统
// ============================================
void history_cleanup(void) {
    // 步骤1：保存历史记录到文件
    history_save();

    // 步骤2：释放所有历史记录内存
    for (int i = 0; i < history_count_value; i++) {
        if (history[i] != NULL) {
            free(history[i]);                   // 释放字符串内存
            history[i] = NULL;
        }
    }

    // 步骤3：重置计数
    history_count_value = 0;
}

