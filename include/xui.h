/**
 * @file xui.h
 * @brief XShell UI 系统头文件
 * 
 * 提供现代化的终端 UI 界面，支持：
 * - 256色和 TrueColor 渐变
 * - Unicode 边框字符
 * - 键盘导航和快捷键
 * - 动态窗口尺寸适配
 */

#ifndef XUI_H
#define XUI_H

#include "xshell.h"
#include "parser.h"
#include <stdbool.h>
#include <stddef.h>
// ==================== 按键定义 ====================
#define XUI_KEY_UP      1000
#define XUI_KEY_DOWN    1001
#define XUI_KEY_LEFT    1002
#define XUI_KEY_RIGHT   1003
#define XUI_KEY_ENTER   1004
#define XUI_KEY_ESC     1005

// ==================== 颜色定义 ====================

// 主题色 (256色模式)
#define XUI_COLOR_BG          236   // 深灰背景
#define XUI_COLOR_CARD_BG     255   // 卡片白色背景
#define XUI_COLOR_BORDER      245   // 边框灰色
#define XUI_COLOR_TITLE_FG    16    // 标题文字黑色
#define XUI_COLOR_HIGHLIGHT   75    // 高亮蓝色
#define XUI_COLOR_ACCENT1     39    // 蓝色强调
#define XUI_COLOR_ACCENT2     112   // 绿色强调
#define XUI_COLOR_ACCENT3     208   // 橙色强调
#define XUI_COLOR_ACCENT4     141   // 紫色强调
#define XUI_COLOR_DIM         244   // 暗淡灰色

// ==================== 菜单项结构 ====================

typedef struct {
    const char *label;      // 显示文本
    const char *icon;       // emoji 图标
    const char *command;    // 执行的命令 (NULL 表示特殊处理)
    int accent_color;       // 强调色 (256色代码)
    bool is_special;        // 是否需要特殊处理
} XUIMenuItem;

// ==================== UI 状态结构 ====================

typedef struct {
    int term_rows;          // 终端行数
    int term_cols;          // 终端列数
    size_t selected;        // 当前选中项
    size_t menu_count;      // 菜单项数量
    bool running;           // UI 是否运行中
    bool use_alt_screen;    // 是否使用备用屏幕
    bool support_256color;  // 是否支持 256 色
    bool support_unicode;   // 是否支持 Unicode
} XUIState;

// ==================== 终端控制函数 (xui_term.c) ====================

/**
 * @brief 初始化终端原始模式
 * @return true 成功, false 失败
 */
bool xui_term_init(void);

/**
 * @brief 恢复终端正常模式
 */
void xui_term_restore(void);

/**
 * @brief 获取终端尺寸
 */
void xui_term_get_size(int *rows, int *cols);

/**
 * @brief 检测终端能力
 */
bool xui_term_supports_256color(void);
bool xui_term_supports_unicode(void);
bool xui_term_supports_alt_screen(void);

/**
 * @brief 进入/退出备用屏幕
 */
void xui_term_alt_screen_enter(void);
void xui_term_alt_screen_leave(void);

/**
 * @brief 清屏和光标控制
 */
void xui_term_clear(void);
void xui_term_move_to(int row, int col);
void xui_term_hide_cursor(void);
void xui_term_show_cursor(void);

/**
 * @brief 颜色输出
 */
void xui_term_reset_style(void);
void xui_term_set_fg256(int color);
void xui_term_set_bg256(int color);
void xui_term_set_bold(void);
void xui_term_set_dim(void);

/**
 * @brief 读取按键
 * @return 按键字符，-1 表示错误
 */
int xui_term_read_key(void);

// ==================== UI 组件函数 (xui_widgets.c) ====================

/**
 * @brief 绘制圆角边框
 */
void xui_draw_box(int top, int left, int height, int width, bool use_unicode);

/**
 * @brief 绘制渐变标题栏
 */
void xui_draw_title_bar(int row, int left, int width, const char *title);

/**
 * @brief 绘制菜单项
 */
void xui_draw_menu_item(int row, int col, const XUIMenuItem *item, bool selected, int index);

/**
 * @brief 绘制状态栏
 */
void xui_draw_status_bar(int row, int left, int width, const char *text);

/**
 * @brief 绘制分隔线
 */
void xui_draw_separator(int row, int left, int width, bool use_unicode);

/**
 * @brief 填充背景
 */
void xui_fill_background(int rows, int cols);

// ==================== 主 UI 函数 (xui.c) ====================

/**
 * @brief 运行 UI 主循环
 * @param ctx Shell 上下文
 * @return 退出状态码
 */
int xui_run(ShellContext *ctx);

/**
 * @brief xui 内置命令入口
 * @param cmd 命令结构
 * @param ctx Shell 上下文
 * @return 退出状态码
 */
int cmd_xui(Command *cmd, ShellContext *ctx);

#endif // XUI_H
