/**
 * @file game2048.c
 * @brief 2048 小游戏实现
 */

#define _POSIX_C_SOURCE 200809L
#include "xgame.h"
#include "xui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/select.h>

#define GRID_SIZE 4
#define CELL_WIDTH 8

// 方块颜色
static int get_tile_color(int value) {
    switch (value) {
        case 2:    return 255;  // 白色
        case 4:    return 230;  // 米色
        case 8:    return 215;  // 橙色
        case 16:   return 209;  // 深橙
        case 32:   return 203;  // 红橙
        case 64:   return 196;  // 红色
        case 128:  return 226;  // 黄色
        case 256:  return 220;  // 金色
        case 512:  return 214;  // 橙黄
        case 1024: return 208;  // 深橙
        case 2048: return 46;   // 绿色
        default:   return 51;   // 青色
    }
}

typedef struct {
    int grid[GRID_SIZE][GRID_SIZE];
    int score;
    int best;
    int game_over;
    int won;
    int quit_requested;
} Game2048;

static int offset_x, offset_y;

static int wait_key_2048(int timeout_ms) {
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = timeout_ms * 1000;
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0) {
        return xui_term_read_key();
    }
    return 0;
}

// 添加随机方块
static void add_random_tile(Game2048 *game) {
    int empty[GRID_SIZE * GRID_SIZE][2];
    int count = 0;
    
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (game->grid[y][x] == 0) {
                empty[count][0] = y;
                empty[count][1] = x;
                count++;
            }
        }
    }
    
    if (count > 0) {
        int idx = rand() % count;
        game->grid[empty[idx][0]][empty[idx][1]] = (rand() % 10 < 9) ? 2 : 4;
    }
}

// 检查是否可以移动
static int can_move(Game2048 *game) {
    for (int y = 0; y < GRID_SIZE; y++) {
        for (int x = 0; x < GRID_SIZE; x++) {
            if (game->grid[y][x] == 0) return 1;
            if (x < GRID_SIZE - 1 && game->grid[y][x] == game->grid[y][x + 1]) return 1;
            if (y < GRID_SIZE - 1 && game->grid[y][x] == game->grid[y + 1][x]) return 1;
        }
    }
    return 0;
}

// 向左移动一行
static int move_line_left(int *line, int *score) {
    int moved = 0;
    int temp[GRID_SIZE] = {0};
    int pos = 0;
    
    // 先压缩
    for (int i = 0; i < GRID_SIZE; i++) {
        if (line[i] != 0) {
            temp[pos++] = line[i];
        }
    }
    
    // 合并相同的
    for (int i = 0; i < GRID_SIZE - 1; i++) {
        if (temp[i] != 0 && temp[i] == temp[i + 1]) {
            temp[i] *= 2;
            *score += temp[i];
            temp[i + 1] = 0;
        }
    }
    
    // 再压缩
    pos = 0;
    for (int i = 0; i < GRID_SIZE; i++) {
        if (temp[i] != 0) {
            if (line[pos] != temp[i]) moved = 1;
            line[pos++] = temp[i];
        }
    }
    while (pos < GRID_SIZE) {
        if (line[pos] != 0) moved = 1;
        line[pos++] = 0;
    }
    
    return moved;
}

// 移动
static int move(Game2048 *game, int dir) {
    int moved = 0;
    int line[GRID_SIZE];
    
    // 0=左, 1=上, 2=右, 3=下
    if (dir == 0) { // 左
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) line[x] = game->grid[y][x];
            if (move_line_left(line, &game->score)) moved = 1;
            for (int x = 0; x < GRID_SIZE; x++) game->grid[y][x] = line[x];
        }
    } else if (dir == 1) { // 上
        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) line[y] = game->grid[y][x];
            if (move_line_left(line, &game->score)) moved = 1;
            for (int y = 0; y < GRID_SIZE; y++) game->grid[y][x] = line[y];
        }
    } else if (dir == 2) { // 右
        for (int y = 0; y < GRID_SIZE; y++) {
            for (int x = 0; x < GRID_SIZE; x++) line[GRID_SIZE - 1 - x] = game->grid[y][x];
            if (move_line_left(line, &game->score)) moved = 1;
            for (int x = 0; x < GRID_SIZE; x++) game->grid[y][x] = line[GRID_SIZE - 1 - x];
        }
    } else if (dir == 3) { // 下
        for (int x = 0; x < GRID_SIZE; x++) {
            for (int y = 0; y < GRID_SIZE; y++) line[GRID_SIZE - 1 - y] = game->grid[y][x];
            if (move_line_left(line, &game->score)) moved = 1;
            for (int y = 0; y < GRID_SIZE; y++) game->grid[y][x] = line[GRID_SIZE - 1 - y];
        }
    }
    
    return moved;
}

// 绘制游戏
static void draw_2048(Game2048 *game) {
    int board_w = GRID_SIZE * (CELL_WIDTH + 1) + 1;
    
    // 标题
    xui_term_move_to(offset_y, offset_x + board_w / 2 - 2);
    xui_term_set_fg256(220);
    xui_term_set_bold();
    printf("2048");
    xui_term_reset_style();
    
    // 分数
    xui_term_move_to(offset_y + 1, offset_x);
    xui_term_set_fg256(46);
    printf("分数: %-6d", game->score);
    xui_term_move_to(offset_y + 1, offset_x + 14);
    xui_term_set_fg256(208);
    printf("最高: %-6d", game->best);
    xui_term_reset_style();
    
    // 绘制网格
    int grid_y = offset_y + 3;
    xui_term_set_fg256(245);
    
    for (int y = 0; y <= GRID_SIZE; y++) {
        xui_term_move_to(grid_y + y * 2, offset_x);
        for (int x = 0; x <= GRID_SIZE; x++) {
            if (y == 0) {
                printf(x == 0 ? "┌" : (x == GRID_SIZE ? "┐" : "┬"));
            } else if (y == GRID_SIZE) {
                printf(x == 0 ? "└" : (x == GRID_SIZE ? "┘" : "┴"));
            } else {
                printf(x == 0 ? "├" : (x == GRID_SIZE ? "┤" : "┼"));
            }
            if (x < GRID_SIZE) {
                for (int i = 0; i < CELL_WIDTH; i++) printf("─");
            }
        }
    }
    
    // 绘制方块
    for (int y = 0; y < GRID_SIZE; y++) {
        xui_term_move_to(grid_y + y * 2 + 1, offset_x);
        for (int x = 0; x < GRID_SIZE; x++) {
            xui_term_set_fg256(245);
            printf("│");
            int val = game->grid[y][x];
            if (val > 0) {
                xui_term_set_fg256(get_tile_color(val));
                xui_term_set_bold();
                printf("%*d", CELL_WIDTH, val);
            } else {
                printf("%*s", CELL_WIDTH, "");
            }
            xui_term_reset_style();
        }
        xui_term_set_fg256(245);
        printf("│");
        xui_term_reset_style();
    }
    
    // 控制说明
    int help_y = grid_y + GRID_SIZE * 2 + 2;
    xui_term_move_to(help_y, offset_x);
    xui_term_set_fg256(244);
    printf("WASD/方向键移动  R重开  Q退出");
    xui_term_reset_style();
    
    // 检查胜利
    if (game->won) {
        xui_term_move_to(grid_y + GRID_SIZE, offset_x + board_w / 2 - 4);
        xui_term_set_fg256(46);
        xui_term_set_bold();
        printf("  YOU WIN!  ");
        xui_term_reset_style();
    }
    
    // 检查失败
    if (game->game_over) {
        xui_term_move_to(grid_y + GRID_SIZE, offset_x + board_w / 2 - 5);
        xui_term_set_fg256(196);
        xui_term_set_bold();
        printf(" GAME OVER! ");
        xui_term_reset_style();
    }
    
    fflush(stdout);
}

// 初始化游戏
static void init_game(Game2048 *game) {
    memset(game->grid, 0, sizeof(game->grid));
    game->score = 0;
    game->game_over = 0;
    game->won = 0;
    add_random_tile(game);
    add_random_tile(game);
}

// 主游戏函数
void xgame_2048(void) {
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    int required_w = GRID_SIZE * (CELL_WIDTH + 1) + 10;
    int required_h = GRID_SIZE * 2 + 10;
    if (term_w < required_w || term_h < required_h) {
        printf("\n终端窗口太小！需要至少 %d x %d\n", required_w, required_h);
        printf("按任意键返回...");
        fflush(stdout);
        wait_key_2048(10000);
        return;
    }
    
    offset_x = (term_w - GRID_SIZE * (CELL_WIDTH + 1) - 1) / 2;
    offset_y = (term_h - GRID_SIZE * 2 - 8) / 2;
    
    srand(time(NULL));
    xui_term_alt_screen_enter();
    xui_term_init();
    xui_term_hide_cursor();
    xui_term_clear();
    
    Game2048 game = {0};
    game.best = get_high_score("2048");
    init_game(&game);
    
    while (1) {
        draw_2048(&game);
        
        int key = wait_key_2048(100);
        if (key == 'q' || key == 'Q') break;
        
        if (key == 'r' || key == 'R') {
            if (game.score > game.best) game.best = game.score;
            init_game(&game);
            xui_term_clear();
            continue;
        }
        
        // 方向移动
        int dir = -1;
        if (key == 'a' || key == 'A' || key == XUI_KEY_LEFT) dir = 0;
        else if (key == 'w' || key == 'W' || key == XUI_KEY_UP) dir = 1;
        else if (key == 'd' || key == 'D' || key == XUI_KEY_RIGHT) dir = 2;
        else if (key == 's' || key == 'S' || key == XUI_KEY_DOWN) dir = 3;
        
        if (dir >= 0 && !game.game_over) {
            if (move(&game, dir)) {
                add_random_tile(&game);
                
                // 检查是否达到 2048
                for (int y = 0; y < GRID_SIZE && !game.won; y++) {
                    for (int x = 0; x < GRID_SIZE && !game.won; x++) {
                        if (game.grid[y][x] >= 2048) game.won = 1;
                    }
                }
                
                // 检查是否游戏结束
                if (!can_move(&game)) {
                    game.game_over = 1;
                }
                
                if (game.score > game.best) game.best = game.score;
            }
        }
        
        if (game.game_over) {
            // 游戏结束，显示排行榜
            wait_key_2048(1000);
            
            if (game.score > 0) {
                xui_term_clear();
                
                char name[32];
                input_player_name(name, sizeof(name));
                
                int rank = add_score("2048", name, game.score);
                
                xui_term_clear();
                show_leaderboard("2048", "🎲 2048 排行榜 🎲");
                
                int board_w = GRID_SIZE * (CELL_WIDTH + 1) + 1;
                xui_term_move_to(offset_y + GRID_SIZE * 2 + 6, offset_x + board_w / 2 - 10);
                if (rank > 0 && rank <= 3) {
                    xui_term_set_fg256(220);
                    printf("恭喜！你排名第 %d 名！", rank);
                }
                xui_term_reset_style();
                
                xui_term_move_to(offset_y + GRID_SIZE * 2 + 8, offset_x + board_w / 2 - 6);
                xui_term_set_fg256(244);
                printf("按任意键返回");
                xui_term_reset_style();
                fflush(stdout);
                wait_key_2048(60000);
            }
            break;
        }
    }
    
    xui_term_alt_screen_leave();
    xui_term_restore();
    system("stty sane");
    fputs("\033[?25h", stdout);  // 显示光标
    printf("\n");
}

// 命令行入口
int cmd_x2048(struct Command *cmd, struct ShellContext *ctx) {
    (void)cmd;
    (void)ctx;
    
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("x2048 - 2048 数字游戏\n\n");
        printf("用法:\n");
        printf("  x2048          启动游戏\n");
        printf("  x2048 --help   显示帮助信息\n\n");
        printf("游戏控制:\n");
        printf("  WASD / 方向键  移动方块\n");
        printf("  R             重新开始\n");
        printf("  Q             退出\n\n");
        printf("规则: 合并相同数字，达到 2048 获胜！\n\n");
        return 0;
    }
    
    xgame_2048();
    return 0;
}
