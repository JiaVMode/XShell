/**
 * @file xui_widgets.c
 * @brief UI 组件库 - 绘制卡片、边框、菜单项等
 */

#define _POSIX_C_SOURCE 200809L

#include "xui.h"
#include <stdio.h>
#include <string.h>

// ==================== 边框字符集 ====================

typedef struct {
    const char *tl, *tr, *bl, *br;  // 四角
    const char *h, *v;              // 水平/垂直线
    const char *lt, *rt;            // 左/右 T 形
    const char *hl;                 // 水平线（用于分隔符）
} BoxChars;

static BoxChars box_ascii = {"+", "+", "+", "+", "-", "|", "+", "+", "-"};
static BoxChars box_unicode = {"╭", "╮", "╰", "╯", "─", "│", "├", "┤", "─"};
static BoxChars box_double = {"╔", "╗", "╚", "╝", "═", "║", "╠", "╣", "═"};

static BoxChars* get_box_chars(bool use_unicode) {
    if (!use_unicode) {
        return &box_ascii;
    }
    // 可以通过环境变量选择双线边框
    const char *style = getenv("XUI_BORDER");
    if (style && strcmp(style, "double") == 0) {
        return &box_double;
    }
    return &box_unicode;
}

// ==================== 填充背景 ====================

void xui_fill_background(int rows, int cols) {
    // 只清屏，使用终端默认背景
    // 这样在深色和浅色主题下都能良好显示
    (void)rows;
    (void)cols;
    xui_term_clear();
}

// ==================== 绘制边框 ====================

void xui_draw_box(int top, int left, int height, int width, bool use_unicode) {
    BoxChars *bc = get_box_chars(use_unicode);
    
    int bottom = top + height - 1;
    int right = left + width - 1;
    
    // 设置边框颜色
    if (xui_term_supports_256color()) {
        xui_term_set_fg256(XUI_COLOR_BORDER);
    }
    
    // 顶边
    xui_term_move_to(top, left);
    fputs(bc->tl, stdout);
    for (int c = left + 1; c < right; c++) {
        fputs(bc->h, stdout);
    }
    fputs(bc->tr, stdout);
    
    // 侧边
    for (int r = top + 1; r < bottom; r++) {
        xui_term_move_to(r, left);
        fputs(bc->v, stdout);
        xui_term_move_to(r, right);
        fputs(bc->v, stdout);
    }
    
    // 底边
    xui_term_move_to(bottom, left);
    fputs(bc->bl, stdout);
    for (int c = left + 1; c < right; c++) {
        fputs(bc->h, stdout);
    }
    fputs(bc->br, stdout);
    
    xui_term_reset_style();
}

// ==================== 渐变标题栏 ====================

void xui_draw_title_bar(int row, int left, int width, const char *title) {
    xui_term_move_to(row, left);
    
    if (xui_term_supports_256color()) {
        // 使用蓝色背景，白色加粗文字
        xui_term_set_bg256(39);   // 蓝色背景
        xui_term_set_fg256(255);  // 白色文字
        xui_term_set_bold();
        
        // 先用空格填充整行背景
        for (int i = 0; i < width; i++) {
            fputc(' ', stdout);
        }
        
        // 计算标题居中位置（简化处理，不考虑中文宽度）
        int title_display_len = (int)strlen(title) / 2;  // 大致估算显示宽度
        int start_pos = (width - title_display_len) / 2;
        if (start_pos < 0) start_pos = 0;
        
        // 移动到标题位置并打印
        xui_term_move_to(row, left + start_pos);
        xui_term_set_bg256(39);
        xui_term_set_fg256(255);
        xui_term_set_bold();
        fputs(title, stdout);
    } else {
        // 降级: 使用简单的蓝色背景
        fputs("\033[1;44;97m", stdout);
        
        // 填充背景
        for (int i = 0; i < width; i++) {
            fputc(' ', stdout);
        }
        
        // 打印标题
        xui_term_move_to(row, left);
        fputs("\033[1;44;97m", stdout);
        fputs(title, stdout);
    }
    
    xui_term_reset_style();
}

// ==================== 绘制菜单项 ====================

void xui_draw_menu_item(int row, int col, const XUIMenuItem *item, bool selected, int index) {
    xui_term_move_to(row, col);
    
    // 显示数字快捷键
    if (xui_term_supports_256color()) {
        if (selected) {
            // 选中项: 高亮背景
            xui_term_set_bg256(XUI_COLOR_HIGHLIGHT);
            xui_term_set_fg256(255);
            xui_term_set_bold();
            printf(" [%d] ", index);
        } else {
            // 未选中项: 彩色数字
            xui_term_set_fg256(220);  // 黄色数字
            xui_term_set_bold();
            printf(" [%d] ", index);
            xui_term_reset_style();
            // 设置菜单项颜色
            xui_term_set_fg256(item->accent_color >= 0 ? item->accent_color : 252);
        }
    } else {
        // 无256色时使用基本ANSI
        if (selected) {
            fputs("\033[7m", stdout);  // 反色
        }
        printf(" [%d] ", index);
    }
    
    // 选中指示器
    if (selected) {
        fputs("> ", stdout);
    } else {
        fputs("  ", stdout);
    }
    
    // 输出图标和标签
    if (item->icon) {
        fputs(item->icon, stdout);
        fputs(" ", stdout);
    }
    
    fputs(item->label, stdout);
    
    // 填充到固定宽度 (用于保持背景色一致)
    int printed = 7 + (item->icon ? 3 : 0) + (int)strlen(item->label);
    int target_width = 26;
    for (int i = printed; i < target_width; i++) {
        fputc(' ', stdout);
    }
    
    xui_term_reset_style();
}

// ==================== 分隔线 ====================

void xui_draw_separator(int row, int left, int width, bool use_unicode) {
    BoxChars *bc = get_box_chars(use_unicode);
    
    xui_term_move_to(row, left);
    
    if (xui_term_supports_256color()) {
        xui_term_set_fg256(XUI_COLOR_BORDER);
    }
    
    fputs(bc->lt, stdout);
    for (int c = 1; c < width - 1; c++) {
        fputs(bc->hl, stdout);
    }
    fputs(bc->rt, stdout);
    
    xui_term_reset_style();
}

// ==================== 状态栏 ====================

void xui_draw_status_bar(int row, int left, int width, const char *text) {
    xui_term_move_to(row, left + 2);
    
    if (xui_term_supports_256color()) {
        xui_term_set_fg256(XUI_COLOR_DIM);
    } else {
        xui_term_set_dim();
    }
    
    fputs("💡 ", stdout);
    
    // 截断过长的文本
    int max_len = width - 8;
    int text_len = (int)strlen(text);
    if (text_len > max_len) {
        fwrite(text, 1, (size_t)max_len - 3, stdout);
        fputs("...", stdout);
    } else {
        fputs(text, stdout);
    }
    
    xui_term_reset_style();
}
