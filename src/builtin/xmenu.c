/*
 * xmenu.c - 交互式菜单系统
 * 
 * 功能：显示交互式菜单，支持键盘导航
 * 用法：xmenu [选项]
 * 
 * 选项：
 *   -f <文件>  从配置文件加载菜单
 *   --help     显示帮助信息
 * 
 * 注意：简化实现，支持基本菜单功能
 */

#include "builtin.h"
#include "utils.h"
#include "parser.h"
#include "executor.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define MAX_MENU_ITEMS 50
#define MAX_ITEM_LENGTH 256

// 菜单项结构
typedef struct {
    char label[MAX_ITEM_LENGTH];
    char command[MAX_ITEM_LENGTH];
} MenuItem;

// 菜单结构
typedef struct {
    char title[MAX_ITEM_LENGTH];
    MenuItem items[MAX_MENU_ITEMS];
    int item_count;
} Menu;

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项]\n", cmd_name);
    printf("功能: 显示交互式菜单，支持键盘导航\n");
    printf("选项:\n");
    printf("  -f <文件>      从配置文件加载菜单（简化实现，暂不支持）\n");
    printf("  -h, --help     显示此帮助信息\n");
    printf("操作:\n");
    printf("  ↑/↓           上下移动选择\n");
    printf("  Enter          执行选中的菜单项\n");
    printf("  q/Q            退出菜单\n");
    printf("示例:\n");
    printf("  %s              # 显示默认菜单\n", cmd_name);
}

// 保存终端设置
static struct termios saved_termios;

// 设置终端为原始模式
static void set_raw_mode(void) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    saved_termios = term;
    
    term.c_lflag &= ~(ICANON | ECHO);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 恢复终端设置
static void restore_terminal(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &saved_termios);
}

// 清屏
static void clear_screen(void) {
    printf("\033[2J\033[H");
    fflush(stdout);
}

// 移动光标到指定位置（暂未使用，保留以备将来扩展）
// static void move_cursor(int row, int col) {
//     printf("\033[%d;%dH", row, col);
//     fflush(stdout);
// }

// 显示菜单
static void display_menu(const Menu *menu, int selected) {
    clear_screen();
    
    // 显示标题
    printf("%s%s%s\n\n", set_color("bold"), menu->title, reset_color());
    
    // 显示菜单项
    for (int i = 0; i < menu->item_count; i++) {
        if (i == selected) {
            // 选中的项：高亮显示
            printf("  %s> %s%s\n", set_color("green"), menu->items[i].label, reset_color());
        } else {
            // 未选中的项：普通显示
            printf("    %s\n", menu->items[i].label);
        }
    }
    
    printf("\n%s使用方向键选择，Enter 执行，q 退出%s\n", 
           set_color("yellow"), reset_color());
    
    fflush(stdout);
}

// 读取键盘输入（处理方向键）
static int read_key(void) {
    char ch;
    if (read(STDIN_FILENO, &ch, 1) != 1) {
        return -1;
    }
    
    // 检查是否是转义序列（方向键）
    if (ch == '\033') {
        char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return ch;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return ch;
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return 'U';  // 上箭头
                case 'B': return 'D';  // 下箭头
                case 'C': return 'R';  // 右箭头
                case 'D': return 'L';  // 左箭头
            }
        }
    }
    
    return ch;
}

// 执行菜单项
static void execute_menu_item(const MenuItem *item, ShellContext *ctx) {
    if (item->command[0] == '\0') {
        return;
    }
    
    // 解析并执行命令
    Command *cmd = parse_command(item->command);
    if (cmd != NULL) {
        execute_command(cmd, ctx);
        free_command(cmd);
    }
}

// 创建默认菜单
static void create_default_menu(Menu *menu) {
    strcpy(menu->title, "XShell 主菜单");
    menu->item_count = 0;
    
    // 添加菜单项
    strcpy(menu->items[menu->item_count].label, "1. 显示当前目录");
    strcpy(menu->items[menu->item_count].command, "xpwd");
    menu->item_count++;
    
    strcpy(menu->items[menu->item_count].label, "2. 列出文件");
    strcpy(menu->items[menu->item_count].command, "xls");
    menu->item_count++;
    
    strcpy(menu->items[menu->item_count].label, "3. 显示历史记录");
    strcpy(menu->items[menu->item_count].command, "xhistory");
    menu->item_count++;
    
    strcpy(menu->items[menu->item_count].label, "4. 显示帮助");
    strcpy(menu->items[menu->item_count].command, "xhelp");
    menu->item_count++;
    
    strcpy(menu->items[menu->item_count].label, "5. 清屏");
    strcpy(menu->items[menu->item_count].command, "xclear");
    menu->item_count++;
    
    strcpy(menu->items[menu->item_count].label, "6. 退出 Shell");
    strcpy(menu->items[menu->item_count].command, "quit");
    menu->item_count++;
}

// xmenu 命令实现
int cmd_xmenu(Command *cmd, ShellContext *ctx) {
    // 解析参数
    if (cmd->arg_count >= 2) {
        if (strcmp(cmd->args[1], "--help") == 0 || strcmp(cmd->args[1], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[1], "-f") == 0) {
            XSHELL_LOG_ERROR(ctx, "xmenu: 错误: 从文件加载菜单功能暂未实现\n");
            return -1;
        }
    }
    
    // 创建菜单
    Menu menu;
    create_default_menu(&menu);
    
    // 设置原始模式
    set_raw_mode();
    
    // 显示菜单并处理输入
    int selected = 0;
    int running = 1;
    
    while (running) {
        display_menu(&menu, selected);
        
        int key = read_key();
        
        switch (key) {
            case 'U':  // 上箭头
                if (selected > 0) {
                    selected--;
                }
                break;
            case 'D':  // 下箭头
                if (selected < menu.item_count - 1) {
                    selected++;
                }
                break;
            case '\n':  // Enter
            case '\r':
                // 执行选中的菜单项
                execute_menu_item(&menu.items[selected], ctx);
                // 等待用户按键继续
                printf("\n按任意键继续...");
                fflush(stdout);
                read_key();
                break;
            case 'q':
            case 'Q':
                running = 0;
                break;
            case '\033':  // ESC
                running = 0;
                break;
        }
        
        // 如果 Shell 已退出，退出菜单
        if (!ctx->running) {
            running = 0;
        }
    }
    
    // 恢复终端设置
    restore_terminal();
    clear_screen();
    
    return 0;
}

