/**
 * @file snake.c
 * @brief 贪吃蛇小游戏实现 - 优化版
 */

#define _POSIX_C_SOURCE 200809L
#include "xgame.h"
#include "xui.h"
#include "xshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

// 游戏设置
#define GAME_WIDTH 25
#define GAME_HEIGHT 20
#define SNAKE_SPEED_MS 200  // 速度（毫秒，数值越大越慢）

// 显示字符
#define SNAKE_HEAD "●"
#define SNAKE_BODY "○"
#define FOOD_CHAR "★"
#define BORDER_H "═"
#define BORDER_V "║"
#define BORDER_TL "╔"
#define BORDER_TR "╗"
#define BORDER_BL "╚"
#define BORDER_BR "╝"

typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    Point *body;
    int length;
    int capacity;
    int dx;
    int dy;
} Snake;

static int offset_x, offset_y;

// 等待按键输入（带超时）
static int wait_key(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;

    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv);
    if (ret > 0) {
        return xui_term_read_key();
    }
    return 0;
}

// 绘制游戏边框
static void draw_game_border(void) {
    xui_term_set_fg256(39); // 蓝色边框
    xui_term_set_bold();
    
    // 上边框
    xui_term_move_to(offset_y, offset_x);
    printf("%s", BORDER_TL);
    for (int i = 0; i < GAME_WIDTH * 2; i++) printf("%s", BORDER_H);
    printf("%s", BORDER_TR);
    
    // 侧边框
    for (int i = 1; i <= GAME_HEIGHT; i++) {
        xui_term_move_to(offset_y + i, offset_x);
        printf("%s", BORDER_V);
        xui_term_move_to(offset_y + i, offset_x + GAME_WIDTH * 2 + 1);
        printf("%s", BORDER_V);
    }
    
    // 下边框
    xui_term_move_to(offset_y + GAME_HEIGHT + 1, offset_x);
    printf("%s", BORDER_BL);
    for (int i = 0; i < GAME_WIDTH * 2; i++) printf("%s", BORDER_H);
    printf("%s", BORDER_BR);
    
    xui_term_reset_style();
}

// 绘制标题栏
static void draw_title(int score, int high_score) {
    int board_w = GAME_WIDTH * 2;
    
    // 标题居中
    xui_term_move_to(offset_y - 2, offset_x + board_w / 2 - 6);
    xui_term_set_fg256(220); // 金色
    xui_term_set_bold();
    printf("🐍  贪吃蛇  🐍");
    xui_term_reset_style();
    
    // 分数显示（居中）
    xui_term_move_to(offset_y - 1, offset_x + board_w / 2 - 12);
    xui_term_set_fg256(46); // 青色
    printf("分数:%3d", score);
    
    xui_term_move_to(offset_y - 1, offset_x + board_w / 2 + 2);
    xui_term_set_fg256(208); // 橙色
    printf("最高:%3d", high_score);
    
    xui_term_reset_style();
}

// 绘制帮助信息
static void draw_help(void) {
    int help_y = offset_y + GAME_HEIGHT + 3;
    
    xui_term_move_to(help_y, offset_x);
    xui_term_set_fg256(244); // 灰色
    xui_term_set_dim();
    printf("控制: ");
    xui_term_reset_style();
    xui_term_set_fg256(75);
    printf("WASD");
    xui_term_set_fg256(244);
    printf(" / ");
    xui_term_set_fg256(75);
    printf("方向键");
    xui_term_set_fg256(244);
    printf("  |  ");
    xui_term_set_fg256(75);
    printf("P");
    xui_term_set_fg256(244);
    printf(":暂停  ");
    xui_term_set_fg256(75);
    printf("Q");
    xui_term_set_fg256(244);
    printf(":退出");
    xui_term_reset_style();
}

// 随机生成食物
static void spawn_food(Point *food, Snake *snake) {
    int valid = 0;
    while (!valid) {
        food->x = rand() % GAME_WIDTH;
        food->y = rand() % GAME_HEIGHT;
        valid = 1;
        
        for (int i = 0; i < snake->length; i++) {
            if (food->x == snake->body[i].x && food->y == snake->body[i].y) {
                valid = 0;
                break;
            }
        }
    }
}

// 主游戏函数
void xgame_snake(void) {
    // 获取终端尺寸
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    // 检查终端大小
    int required_w = GAME_WIDTH * 2 + 4;
    int required_h = GAME_HEIGHT + 8;
    if (term_w < required_w || term_h < required_h) {
        printf("\n终端窗口太小！\n");
        printf("需要至少 %d x %d 的窗口大小\n", required_w, required_h);
        printf("当前: %d x %d\n\n", term_w, term_h);
        printf("按任意键返回...");
        fflush(stdout);
        wait_key(10000);
        return;
    }
    
    // 计算居中偏移
    offset_x = (term_w - GAME_WIDTH * 2 - 2) / 2;
    offset_y = (term_h - GAME_HEIGHT - 6) / 2 + 3;
    
    // 初始化
    srand(time(NULL));
    xui_term_alt_screen_enter();
    xui_term_init();
    xui_term_hide_cursor();
    xui_term_clear();
    
    // 初始化蛇
    Snake snake;
    snake.capacity = GAME_WIDTH * GAME_HEIGHT;
    snake.body = malloc(sizeof(Point) * snake.capacity);
    snake.length = 3;
    snake.dx = 1;
    snake.dy = 0;
    
    int start_x = GAME_WIDTH / 2;
    int start_y = GAME_HEIGHT / 2;
    for (int i = 0; i < snake.length; i++) {
        snake.body[i].x = start_x - i;
        snake.body[i].y = start_y;
    }
    
    Point food;
    spawn_food(&food, &snake);
    
    int score = 0;
    int high_score = get_high_score("snake");
    int game_over = 0;
    
    // 绘制静态元素
    draw_game_border();
    draw_help();
    
    while (!game_over) {
        // 绘制标题（包含分数）
        draw_title(score, high_score);
        
        // 绘制食物
        xui_term_move_to(offset_y + 1 + food.y, offset_x + 1 + food.x * 2);
        xui_term_set_fg256(196); // 红色
        printf("%s", FOOD_CHAR);
        xui_term_reset_style();
        
        // 绘制蛇
        for (int i = 0; i < snake.length; i++) {
            xui_term_move_to(offset_y + 1 + snake.body[i].y, 
                           offset_x + 1 + snake.body[i].x * 2);
            if (i == 0) {
                xui_term_set_fg256(82); // 绿色蛇头
                xui_term_set_bold();
                printf("%s", SNAKE_HEAD);
            } else {
                xui_term_set_fg256(46); // 青绿色蛇身
                printf("%s", SNAKE_BODY);
            }
        }
        xui_term_reset_style();
        fflush(stdout);
        
        // 输入处理
        int key = wait_key(SNAKE_SPEED_MS);
        
        // 暂停
        if (key == 'p' || key == 'P') {
            xui_term_move_to(offset_y + GAME_HEIGHT/2 + 1, 
                           offset_x + GAME_WIDTH - 2);
            xui_term_set_fg256(226);
            xui_term_set_bold();
            printf(" PAUSED ");
            xui_term_reset_style();
            fflush(stdout);
            
            while (1) {
                int k = wait_key(100);
                if (k == 'p' || k == 'P') break;
                if (k == 'q' || k == 'Q') { game_over = 1; break; }
            }
            
            // 清除 PAUSED 文字
            xui_term_move_to(offset_y + GAME_HEIGHT/2 + 1, 
                           offset_x + GAME_WIDTH - 2);
            printf("        ");
            if (game_over) break;
        }
        
        if (key == 'q' || key == 'Q') {
            game_over = 1;
            break;
        }
        
        // 方向控制
        if ((key == 'w' || key == 'W' || key == XUI_KEY_UP) && snake.dy != 1) {
            snake.dx = 0; snake.dy = -1;
        }
        else if ((key == 's' || key == 'S' || key == XUI_KEY_DOWN) && snake.dy != -1) {
            snake.dx = 0; snake.dy = 1;
        }
        else if ((key == 'a' || key == 'A' || key == XUI_KEY_LEFT) && snake.dx != 1) {
            snake.dx = -1; snake.dy = 0;
        }
        else if ((key == 'd' || key == 'D' || key == XUI_KEY_RIGHT) && snake.dx != -1) {
            snake.dx = 1; snake.dy = 0;
        }
        
        // 移动蛇
        Point new_head = {
            snake.body[0].x + snake.dx,
            snake.body[0].y + snake.dy
        };
        
        // 碰撞检测 - 墙壁
        if (new_head.x < 0 || new_head.x >= GAME_WIDTH ||
            new_head.y < 0 || new_head.y >= GAME_HEIGHT) {
            game_over = 1;
            break;
        }
        
        // 碰撞检测 - 自身
        for (int i = 0; i < snake.length; i++) {
            if (new_head.x == snake.body[i].x && 
                new_head.y == snake.body[i].y) {
                game_over = 1;
                break;
            }
        }
        if (game_over) break;
        
        // 检测是否吃到食物
        int ate = (new_head.x == food.x && new_head.y == food.y);
        
        Point tail_pos = snake.body[snake.length - 1];
        
        if (ate) {
            score += 10;
            if (score > high_score) high_score = score;
            if (snake.length < snake.capacity) {
                snake.length++;
            }
            spawn_food(&food, &snake);
        } else {
            // 清除旧尾巴
            xui_term_move_to(offset_y + 1 + tail_pos.y, 
                           offset_x + 1 + tail_pos.x * 2);
            printf("  ");
        }
        
        // 移动蛇身
        for (int i = snake.length - 1; i > 0; i--) {
            snake.body[i] = snake.body[i - 1];
        }
        snake.body[0] = new_head;
    }
    
    // 游戏结束
    int board_w = GAME_WIDTH * 2;  // 实际显示宽度
    
    xui_term_move_to(offset_y + GAME_HEIGHT/2, 
                   offset_x + board_w/2 - 5);
    xui_term_set_fg256(196);
    xui_term_set_bold();
    printf(" GAME OVER! ");
    
    xui_term_move_to(offset_y + GAME_HEIGHT/2 + 2, 
                   offset_x + board_w/2 - 6);
    xui_term_set_fg256(220);
    printf("最终得分: %d", score);
    xui_term_reset_style();
    fflush(stdout);
    
    // 检查是否进入排行榜
    int high = get_high_score("snake");
    if (score > 0) {
        wait_key(1000);
        xui_term_clear();
        
        // 输入名字
        char name[32];
        input_player_name(name, sizeof(name));
        
        // 保存分数
        int rank = add_score("snake", name, score);
        
        // 显示排行榜
        xui_term_clear();
        show_leaderboard("snake", "🐍 贪吃蛇排行榜 🐍");
        
        xui_term_move_to(offset_y + GAME_HEIGHT + 2, offset_x + board_w/2 - 10);
        if (rank > 0 && rank <= 3) {
            xui_term_set_fg256(220);
            printf("恭喜！你排名第 %d 名！", rank);
        }
        xui_term_reset_style();
        
        xui_term_move_to(offset_y + GAME_HEIGHT + 4, offset_x + board_w/2 - 6);
        xui_term_set_fg256(244);
        printf("按任意键返回");
        xui_term_reset_style();
        fflush(stdout);
        wait_key(60000);
    } else {
        xui_term_move_to(offset_y + GAME_HEIGHT/2 + 4, 
                       offset_x + board_w/2 - 6);
        xui_term_set_fg256(244);
        printf("按任意键返回");
        xui_term_reset_style();
        fflush(stdout);
        wait_key(60000);
    }
    
    free(snake.body);
    xui_term_alt_screen_leave();
    xui_term_restore();
    system("stty sane");
    fputs("\033[?25h", stdout);  // 显示光标
    printf("\n");
}

// 命令行入口
int cmd_xsnake(struct Command *cmd, struct ShellContext *ctx) {
    (void)cmd;
    (void)ctx;
    
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xsnake - 贪吃蛇游戏\n\n");
        printf("用法:\n");
        printf("  xsnake           启动游戏\n");
        printf("  xsnake --help    显示帮助信息\n\n");
        printf("游戏控制:\n");
        printf("  WASD / 方向键    移动\n");
        printf("  P               暂停\n");
        printf("  Q               退出\n\n");
        return 0;
    }
    
    // 启动游戏
    xgame_snake();
    return 0;
}
