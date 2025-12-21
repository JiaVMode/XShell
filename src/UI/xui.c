/**
 * @file xui.c
 * @brief XShell UI 主逻辑 - 菜单渲染和事件处理
 */

#define _POSIX_C_SOURCE 200809L

#include "xui.h"
#include "executor.h"
#include "xgame.h"
#include "xweb.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>



// ==================== 菜单定义 ====================

static const XUIMenuItem g_menu_items[] = {
    {"执行命令",     "📝", NULL,        39,  true},
    {"系统监控",     "💻", NULL,        39,  true},
    {"网页浏览",     "🌐", NULL,        112, true},
    {"文件列表",     "📁", "xls -la",   142, false},
    {"贪吃蛇",       "🐍", NULL,        208, true},
    {"历史记录",     "📜", "xhistory",  220, false},
    {"俄罗斯方块",   "🎮", NULL,        141, true},
    {"计算器",       "🧮", NULL,        75,  true},
    {"2048",         "🎲", NULL,        208, true},
    {"退出 UI",      "🚪", NULL,        244, true},
};

static const size_t g_menu_count = sizeof(g_menu_items) / sizeof(g_menu_items[0]);

// ==================== UI 状态 ====================

static XUIState g_state;

// ==================== 辅助函数 ====================

static void sleep_ms(long ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000L;
    nanosleep(&ts, NULL);
}

static void wait_for_enter(void) {
    // 确保输出缓冲区刷新
    fflush(stdout);
    
    fputs("\n\033[2m按 Enter 键返回 UI...\033[0m", stdout);
    fflush(stdout);
    
    // 简单地等待回车键
    int c;
    do {
        c = getchar();
    } while (c != '\n' && c != '\r' && c != EOF);
    
    fputs("\n", stdout);
}



// ==================== 绘制 UI ====================

static void draw_card_background(int top, int left, int height, int width) {
    // 不填充背景，使用终端默认背景
    // 这样在深色和浅色主题下都能良好显示
    (void)top;
    (void)left;
    (void)height;
    (void)width;
    // 只清除卡片区域即可
}

static void draw_ui(void) {
    int rows = g_state.term_rows;
    int cols = g_state.term_cols;
    
    // 计算卡片尺寸和位置
    int card_width = (cols >= 80) ? 70 : (cols - 6);
    if (card_width < 50) card_width = 50;
    int card_height = 18;
    int card_left = (cols - card_width) / 2;
    int card_top = (rows - card_height) / 2;
    if (card_top < 2) card_top = 2;
    if (card_left < 1) card_left = 1;
    
    // 填充背景
    xui_fill_background(rows, cols);
    
    // 绘制卡片背景
    draw_card_background(card_top, card_left, card_height, card_width);
    
    // 绘制边框
    xui_draw_box(card_top, card_left, card_height, card_width, g_state.support_unicode);
    
    // 绘制渐变标题栏
    xui_draw_title_bar(card_top, card_left + 1, card_width - 2, 
                       "  XShell UI 控制面板  v1.0  ");
    
    // 绘制副标题
    xui_term_move_to(card_top + 2, card_left + 2);
    if (xui_term_supports_256color()) {
        xui_term_set_fg256(XUI_COLOR_DIM);
    } else {
        xui_term_set_dim();
    }
    fputs("上下键选择  Enter执行  0-9快捷键  q退出", stdout);
    xui_term_reset_style();
    
    // 绘制分隔线
    xui_draw_separator(card_top + 3, card_left, card_width, g_state.support_unicode);
    
    // 绘制菜单项 (双列布局)
    int items_per_col = 5;
    int col1_left = card_left + 3;
    int col2_left = card_left + card_width / 2 + 1;
    int menu_top = card_top + 5;
    
    for (size_t i = 0; i < g_menu_count; i++) {
        int col = (int)(i / (size_t)items_per_col);
        int row_offset = (int)(i % (size_t)items_per_col);
        int x = (col == 0) ? col1_left : col2_left;
        int y = menu_top + row_offset;
        
        xui_draw_menu_item(y, x, &g_menu_items[i], i == g_state.selected, (int)i);
    }
    
    // 绘制底部分隔线
    xui_draw_separator(card_top + card_height - 3, card_left, card_width, g_state.support_unicode);
    
    // 绘制状态栏
    xui_draw_status_bar(card_top + card_height - 2, card_left, card_width,
                        "提示: 设置 TERM=xterm-256color 获得最佳效果");
    
    fflush(stdout);
}

// ==================== 处理命令 ====================

static int handle_menu_action(ShellContext *ctx, size_t index) {
    if (index >= g_menu_count) {
        return 0;
    }
    
    const XUIMenuItem *item = &g_menu_items[index];
    
    // 特殊处理项
    if (item->is_special) {
        if (strcmp(item->label, "退出 UI") == 0) {
            g_state.running = false;
            return 0;
        }
        if (strcmp(item->label, "执行命令") == 0) {
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_leave();
            }
            xui_term_restore();
            xui_term_show_cursor();
            
            // 重置终端
            fputs("\033[0m\033[H\033[2J", stdout);
            printf("\n\033[1;36m=== 命令模式 ===\033[0m\n");
            printf("输入命令执行，输入 'exit' 返回 UI\n\n");
            fflush(stdout);
            
            // 循环执行命令直到用户输入 exit
            char cmd_line[1024];
            while (1) {
                printf("\033[1;32mxshell>\033[0m ");
                fflush(stdout);
                
                if (!fgets(cmd_line, sizeof(cmd_line), stdin)) {
                    break;
                }
                cmd_line[strcspn(cmd_line, "\r\n")] = '\0';
                
                // 检查是否退出
                if (cmd_line[0] == '\0') {
                    continue;  // 空输入，继续
                }
                if (strcmp(cmd_line, "exit") == 0 || strcmp(cmd_line, "quit") == 0 || strcmp(cmd_line, "q") == 0) {
                    printf("返回 UI\n");
                    break;
                }
                
                // 解析并执行命令
                Command *cmd = parse_command(cmd_line);
                if (cmd != NULL) {
                    execute_command(cmd, ctx);
                    free_command(cmd);
                }
                printf("\n");
            }
            
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "贪吃蛇") == 0) {
            // 启动贪吃蛇游戏
            xgame_snake();
            
            // 游戏退出后恢复 UI 状态
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "俄罗斯方块") == 0) {
            // 启动俄罗斯方块游戏
            xgame_tetris();
            
            // 游戏退出后恢复 UI 状态
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "2048") == 0) {
            xgame_2048();
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "系统监控") == 0) {
            xsysmon();
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "计算器") == 0) {
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_leave();
            }
            xui_term_restore();
            xui_term_show_cursor();
            
            // 重置终端
            fputs("\033[0m\033[H\033[2J", stdout);
            printf("\n\033[1;36m=== 计算器模式 ===\033[0m\n");
            printf("输入数学表达式，输入 'exit' 退出\n\n");
            fflush(stdout);
            
            // 循环计算直到用户输入 exit
            char expr[256];
            while (1) {
                printf("\033[1;33mcalc>\033[0m ");
                fflush(stdout);
                
                if (!fgets(expr, sizeof(expr), stdin)) {
                    break;
                }
                expr[strcspn(expr, "\r\n")] = '\0';
                
                // 检查是否退出
                if (expr[0] == '\0') {
                    continue;  // 空输入，继续
                }
                if (strcmp(expr, "exit") == 0 || strcmp(expr, "quit") == 0 || strcmp(expr, "q") == 0) {
                    printf("退出计算器\n");
                    break;
                }
                
                // 构建 xcalc 命令并执行
                char cmd_line[300];
                snprintf(cmd_line, sizeof(cmd_line), "xcalc %s", expr);
                
                Command *cmd = parse_command(cmd_line);
                if (cmd != NULL) {
                    execute_command(cmd, ctx);
                    free_command(cmd);
                }
            }
            
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        if (strcmp(item->label, "网页浏览") == 0) {
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_leave();
            }
            xui_term_restore();
            xui_term_show_cursor();
            
            // 重置终端
            fputs("\033[0m\033[H\033[2J", stdout);
            fflush(stdout);
            
            // 调用网页浏览器
            xweb_browser(ctx);
            
            xui_term_init();
            if (g_state.use_alt_screen) {
                xui_term_alt_screen_enter();
            }
            xui_term_hide_cursor();
            return 0;
        }
        return 0;
    }
    
    // 普通命令
    if (item->command != NULL) {
        // 先恢复终端到正常模式
        xui_term_restore();
        xui_term_show_cursor();
        
        // 离开备用屏幕（如果有）
        if (g_state.use_alt_screen) {
            xui_term_alt_screen_leave();
        }
        
        // 完全重置终端状态
        fputs("\033[0m", stdout);  // 重置所有属性
        fputs("\033[H\033[2J", stdout);  // 清屏并移到左上角
        fflush(stdout);
        
        printf("\n\033[1;36m>> %s\033[0m\n\n", item->command);
        fflush(stdout);
        
        // 解析并执行命令
        Command *cmd = parse_command(item->command);
        int status = 0;
        if (cmd != NULL) {
            status = execute_command(cmd, ctx);
            free_command(cmd);
        }
        
        // 等待用户按回车
        wait_for_enter();
        
        // 重新进入 UI 模式
        xui_term_init();
        if (g_state.use_alt_screen) {
            xui_term_alt_screen_enter();
        }
        xui_term_hide_cursor();
        
        return status;
    }
    
    return 0;
}

// ==================== 主循环 ====================

int xui_run(ShellContext *ctx) {
    // 检查 TTY
    if (!isatty(STDIN_FILENO)) {
        fprintf(stderr, "xui: 需要终端环境\n");
        return 1;
    }
    
    // 初始化状态
    memset(&g_state, 0, sizeof(g_state));
    g_state.running = true;
    g_state.selected = 0;
    g_state.menu_count = g_menu_count;
    g_state.support_256color = xui_term_supports_256color();
    g_state.support_unicode = xui_term_supports_unicode();
    g_state.use_alt_screen = xui_term_supports_alt_screen();
    
    // 进入原始模式
    if (!xui_term_init()) {
        perror("xui");
        return 1;
    }
    
    // 进入备用屏幕
    if (g_state.use_alt_screen) {
        xui_term_alt_screen_enter();
    }
    xui_term_hide_cursor();
    
    // 主循环
    while (g_state.running) {
        // 获取终端尺寸
        xui_term_get_size(&g_state.term_rows, &g_state.term_cols);
        
        // 绘制 UI
        draw_ui();
        
        // 读取按键
        int key = xui_term_read_key();
        
        switch (key) {
            case 'q':
            case 'Q':
            case XUI_KEY_ESC:
                g_state.running = false;
                break;
                
            case XUI_KEY_UP:
                if (g_state.selected > 0) {
                    g_state.selected--;
                }
                break;
                
            case XUI_KEY_DOWN:
                if (g_state.selected + 1 < g_state.menu_count) {
                    g_state.selected++;
                }
                break;
                
            case XUI_KEY_ENTER:
                handle_menu_action(ctx, g_state.selected);
                break;
                
            default:
                // 数字快捷键 0-9
                if (key >= '0' && key <= '9') {
                    size_t idx = (size_t)(key - '0');
                    if (idx < g_state.menu_count) {
                        g_state.selected = idx;
                        handle_menu_action(ctx, idx);
                    }
                }
                break;
        }
    }
    
    // 清理终端状态
    if (g_state.use_alt_screen) {
        xui_term_alt_screen_leave();
    }
    xui_term_restore();
    
    // 强制重置终端到正常状态
    system("stty sane");
    fputs("\033[?25h", stdout);  // 显示光标
    
    // 显示退出消息
    printf("\n\033[1;44;97m  感谢使用 XShell UI  \033[0m\n\n");
    fflush(stdout);
    
    return 0;
}

// ==================== 内置命令入口 ====================

int cmd_xui(Command *cmd, ShellContext *ctx) {
    (void)cmd;  // 忽略命令参数
    return xui_run(ctx);
}
