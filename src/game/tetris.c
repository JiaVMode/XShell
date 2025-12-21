/**
 * @file tetris.c
 * @brief 俄罗斯方块小游戏实现
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

// 游戏设置
#define BOARD_WIDTH 10
#define BOARD_HEIGHT 20
#define BLOCK_CHAR "█"
#define EMPTY_CHAR " "

// 方块类型
#define PIECE_I 0
#define PIECE_O 1
#define PIECE_T 2
#define PIECE_S 3
#define PIECE_Z 4
#define PIECE_J 5
#define PIECE_L 6
#define NUM_PIECES 7

// 方块颜色 (256色)
static const int piece_colors[NUM_PIECES] = {
    51,   // I - 青色
    226,  // O - 黄色
    129,  // T - 紫色
    46,   // S - 绿色
    196,  // Z - 红色
    21,   // J - 蓝色
    208   // L - 橙色
};

// 方块形状 (4x4 矩阵，4种旋转状态)
static const int pieces[NUM_PIECES][4][4][4] = {
    // I
    {
        {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}},
        {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}}
    },
    // O
    {
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}}
    },
    // T
    {
        {{0,1,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    // S
    {
        {{0,1,1,0},{1,1,0,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,0,0,0},{0,1,1,0},{1,1,0,0},{0,0,0,0}},
        {{1,0,0,0},{1,1,0,0},{0,1,0,0},{0,0,0,0}}
    },
    // Z
    {
        {{1,1,0,0},{0,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,0,1,0},{0,1,1,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,1,0,0},{1,1,0,0},{1,0,0,0},{0,0,0,0}}
    },
    // J
    {
        {{1,0,0,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,1,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{0,0,1,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{1,1,0,0},{0,0,0,0}}
    },
    // L
    {
        {{0,0,1,0},{1,1,1,0},{0,0,0,0},{0,0,0,0}},
        {{0,1,0,0},{0,1,0,0},{0,1,1,0},{0,0,0,0}},
        {{0,0,0,0},{1,1,1,0},{1,0,0,0},{0,0,0,0}},
        {{1,1,0,0},{0,1,0,0},{0,1,0,0},{0,0,0,0}}
    }
};

typedef struct {
    int board[BOARD_HEIGHT][BOARD_WIDTH];
    int current_piece;
    int current_rotation;
    int piece_x;
    int piece_y;
    int next_piece;
    int score;
    int lines;
    int level;
    int game_over;
} TetrisGame;

static int offset_x, offset_y;

// 等待按键输入
static int tetris_wait_key(int timeout_ms) {
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

// 检查方块是否可以放置
static int can_place(TetrisGame *game, int piece, int rotation, int x, int y) {
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (pieces[piece][rotation][py][px]) {
                int bx = x + px;
                int by = y + py;
                if (bx < 0 || bx >= BOARD_WIDTH || by >= BOARD_HEIGHT) {
                    return 0;
                }
                if (by >= 0 && game->board[by][bx]) {
                    return 0;
                }
            }
        }
    }
    return 1;
}

// 固定方块到棋盘
static void lock_piece(TetrisGame *game) {
    int piece = game->current_piece;
    int rot = game->current_rotation;
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (pieces[piece][rot][py][px]) {
                int bx = game->piece_x + px;
                int by = game->piece_y + py;
                if (by >= 0 && by < BOARD_HEIGHT && bx >= 0 && bx < BOARD_WIDTH) {
                    game->board[by][bx] = piece + 1;
                }
            }
        }
    }
}

// 清除完成的行
static int clear_lines(TetrisGame *game) {
    int cleared = 0;
    for (int y = BOARD_HEIGHT - 1; y >= 0; y--) {
        int full = 1;
        for (int x = 0; x < BOARD_WIDTH; x++) {
            if (!game->board[y][x]) {
                full = 0;
                break;
            }
        }
        if (full) {
            cleared++;
            for (int cy = y; cy > 0; cy--) {
                for (int x = 0; x < BOARD_WIDTH; x++) {
                    game->board[cy][x] = game->board[cy - 1][x];
                }
            }
            for (int x = 0; x < BOARD_WIDTH; x++) {
                game->board[0][x] = 0;
            }
            y++; // 重新检查当前行
        }
    }
    return cleared;
}

// 生成新方块
static void spawn_piece(TetrisGame *game) {
    game->current_piece = game->next_piece;
    game->next_piece = rand() % NUM_PIECES;
    game->current_rotation = 0;
    game->piece_x = BOARD_WIDTH / 2 - 2;
    game->piece_y = 0;
    
    if (!can_place(game, game->current_piece, game->current_rotation, 
                   game->piece_x, game->piece_y)) {
        game->game_over = 1;
    }
}

// 绘制游戏界面
static void draw_game(TetrisGame *game) {
    // 绘制边框
    xui_term_set_fg256(245);
    for (int y = 0; y <= BOARD_HEIGHT + 1; y++) {
        xui_term_move_to(offset_y + y, offset_x);
        if (y == 0) {
            printf("╔");
            for (int x = 0; x < BOARD_WIDTH * 2; x++) printf("═");
            printf("╗");
        } else if (y == BOARD_HEIGHT + 1) {
            printf("╚");
            for (int x = 0; x < BOARD_WIDTH * 2; x++) printf("═");
            printf("╝");
        } else {
            printf("║");
            xui_term_move_to(offset_y + y, offset_x + BOARD_WIDTH * 2 + 1);
            printf("║");
        }
    }
    xui_term_reset_style();
    
    // 绘制棋盘
    for (int y = 0; y < BOARD_HEIGHT; y++) {
        xui_term_move_to(offset_y + y + 1, offset_x + 1);
        for (int x = 0; x < BOARD_WIDTH; x++) {
            int cell = game->board[y][x];
            if (cell) {
                xui_term_set_fg256(piece_colors[cell - 1]);
                printf("%s%s", BLOCK_CHAR, BLOCK_CHAR);
            } else {
                printf("  ");
            }
        }
    }
    
    // 绘制当前方块
    int piece = game->current_piece;
    int rot = game->current_rotation;
    xui_term_set_fg256(piece_colors[piece]);
    for (int py = 0; py < 4; py++) {
        for (int px = 0; px < 4; px++) {
            if (pieces[piece][rot][py][px]) {
                int screen_y = game->piece_y + py;
                int screen_x = game->piece_x + px;
                if (screen_y >= 0 && screen_y < BOARD_HEIGHT) {
                    xui_term_move_to(offset_y + screen_y + 1, 
                                   offset_x + 1 + screen_x * 2);
                    printf("%s%s", BLOCK_CHAR, BLOCK_CHAR);
                }
            }
        }
    }
    xui_term_reset_style();
    
    // 绘制信息面板
    int info_x = offset_x + BOARD_WIDTH * 2 + 5;
    
    xui_term_move_to(offset_y + 1, info_x);
    xui_term_set_fg256(220);
    xui_term_set_bold();
    printf("俄罗斯方块");
    xui_term_reset_style();
    
    xui_term_move_to(offset_y + 3, info_x);
    xui_term_set_fg256(46);
    printf("分数: %d", game->score);
    
    xui_term_move_to(offset_y + 4, info_x);
    printf("行数: %d", game->lines);
    
    xui_term_move_to(offset_y + 5, info_x);
    printf("等级: %d", game->level);
    
    // 绘制下一个方块
    xui_term_move_to(offset_y + 7, info_x);
    xui_term_set_fg256(244);
    printf("下一个:");
    xui_term_reset_style();
    
    xui_term_set_fg256(piece_colors[game->next_piece]);
    for (int py = 0; py < 4; py++) {
        xui_term_move_to(offset_y + 8 + py, info_x);
        for (int px = 0; px < 4; px++) {
            if (pieces[game->next_piece][0][py][px]) {
                printf("%s%s", BLOCK_CHAR, BLOCK_CHAR);
            } else {
                printf("  ");
            }
        }
    }
    xui_term_reset_style();
    
    // 绘制控制说明
    xui_term_move_to(offset_y + 14, info_x);
    xui_term_set_fg256(244);
    printf("A/D: 移动");
    xui_term_move_to(offset_y + 15, info_x);
    printf("W: 旋转");
    xui_term_move_to(offset_y + 16, info_x);
    printf("S: 加速");
    xui_term_move_to(offset_y + 17, info_x);
    printf("空格: 落下");
    xui_term_move_to(offset_y + 18, info_x);
    printf("P: 暂停");
    xui_term_move_to(offset_y + 19, info_x);
    printf("Q: 退出");
    xui_term_reset_style();
    
    fflush(stdout);
}

// 主游戏函数
void xgame_tetris(void) {
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    int required_w = BOARD_WIDTH * 2 + 20;
    int required_h = BOARD_HEIGHT + 4;
    if (term_w < required_w || term_h < required_h) {
        printf("\n终端窗口太小！需要至少 %d x %d\n", required_w, required_h);
        printf("按任意键返回...");
        fflush(stdout);
        tetris_wait_key(10000);
        return;
    }
    
    offset_x = (term_w - BOARD_WIDTH * 2 - 18) / 2;
    offset_y = (term_h - BOARD_HEIGHT - 2) / 2;
    
    srand(time(NULL));
    xui_term_alt_screen_enter();
    xui_term_init();
    xui_term_hide_cursor();
    xui_term_clear();
    
    TetrisGame game = {0};
    game.next_piece = rand() % NUM_PIECES;
    spawn_piece(&game);
    game.level = 1;
    
    int drop_interval = 500; // 毫秒
    int drop_timer = 0;
    int tick = 50; // 每帧 50ms
    
    while (!game.game_over) {
        draw_game(&game);
        
        int key = tetris_wait_key(tick);
        
        if (key == 'q' || key == 'Q') break;
        
        if (key == 'p' || key == 'P') {
            xui_term_move_to(offset_y + BOARD_HEIGHT / 2, offset_x + BOARD_WIDTH - 3);
            xui_term_set_fg256(226);
            xui_term_set_bold();
            printf(" PAUSED ");
            xui_term_reset_style();
            fflush(stdout);
            while (1) {
                int k = tetris_wait_key(100);
                if (k == 'p' || k == 'P') break;
                if (k == 'q' || k == 'Q') { game.game_over = 1; break; }
            }
            xui_term_clear();
            continue;
        }
        
        // 移动
        if ((key == 'a' || key == 'A' || key == XUI_KEY_LEFT) &&
            can_place(&game, game.current_piece, game.current_rotation, 
                     game.piece_x - 1, game.piece_y)) {
            game.piece_x--;
        }
        if ((key == 'd' || key == 'D' || key == XUI_KEY_RIGHT) &&
            can_place(&game, game.current_piece, game.current_rotation, 
                     game.piece_x + 1, game.piece_y)) {
            game.piece_x++;
        }
        
        // 旋转
        if ((key == 'w' || key == 'W' || key == XUI_KEY_UP)) {
            int new_rot = (game.current_rotation + 1) % 4;
            if (can_place(&game, game.current_piece, new_rot, 
                         game.piece_x, game.piece_y)) {
                game.current_rotation = new_rot;
            }
        }
        
        // 软降
        if (key == 's' || key == 'S' || key == XUI_KEY_DOWN) {
            if (can_place(&game, game.current_piece, game.current_rotation, 
                         game.piece_x, game.piece_y + 1)) {
                game.piece_y++;
                game.score += 1;
            }
        }
        
        // 硬降
        if (key == ' ') {
            while (can_place(&game, game.current_piece, game.current_rotation, 
                           game.piece_x, game.piece_y + 1)) {
                game.piece_y++;
                game.score += 2;
            }
            lock_piece(&game);
            int cleared = clear_lines(&game);
            if (cleared > 0) {
                int points[] = {0, 100, 300, 500, 800};
                game.score += points[cleared] * game.level;
                game.lines += cleared;
                game.level = game.lines / 10 + 1;
                drop_interval = 500 - (game.level - 1) * 40;
                if (drop_interval < 100) drop_interval = 100;
            }
            spawn_piece(&game);
            drop_timer = 0;
            continue;
        }
        
        // 自动下落
        drop_timer += tick;
        if (drop_timer >= drop_interval) {
            drop_timer = 0;
            if (can_place(&game, game.current_piece, game.current_rotation, 
                         game.piece_x, game.piece_y + 1)) {
                game.piece_y++;
            } else {
                lock_piece(&game);
                int cleared = clear_lines(&game);
                if (cleared > 0) {
                    int points[] = {0, 100, 300, 500, 800};
                    game.score += points[cleared] * game.level;
                    game.lines += cleared;
                    game.level = game.lines / 10 + 1;
                    drop_interval = 500 - (game.level - 1) * 40;
                    if (drop_interval < 100) drop_interval = 100;
                }
                spawn_piece(&game);
            }
        }
    }
    
    // 游戏结束
    xui_term_move_to(offset_y + BOARD_HEIGHT / 2 - 1, offset_x + BOARD_WIDTH - 5);
    xui_term_set_fg256(196);
    xui_term_set_bold();
    printf(" GAME OVER! ");
    xui_term_move_to(offset_y + BOARD_HEIGHT / 2 + 1, offset_x + BOARD_WIDTH - 6);
    xui_term_set_fg256(220);
    printf("最终得分: %d", game.score);
    xui_term_reset_style();
    fflush(stdout);
    
    if (game.score > 0) {
        tetris_wait_key(1000);
        xui_term_clear();
        
        char name[32];
        input_player_name(name, sizeof(name));
        
        int rank = add_score("tetris", name, game.score);
        
        xui_term_clear();
        show_leaderboard("tetris", "🎮 俄罗斯方块排行榜 🎮");
        
        xui_term_move_to(offset_y + BOARD_HEIGHT + 2, offset_x + BOARD_WIDTH - 8);
        if (rank > 0 && rank <= 3) {
            xui_term_set_fg256(220);
            printf("恭喜！你排名第 %d 名！", rank);
        }
        xui_term_reset_style();
        
        xui_term_move_to(offset_y + BOARD_HEIGHT + 4, offset_x + BOARD_WIDTH - 4);
        xui_term_set_fg256(244);
        printf("按任意键返回");
        xui_term_reset_style();
        fflush(stdout);
        tetris_wait_key(60000);
    } else {
        xui_term_move_to(offset_y + BOARD_HEIGHT / 2 + 3, offset_x + BOARD_WIDTH - 7);
        xui_term_set_fg256(244);
        printf("按任意键返回");
        xui_term_reset_style();
        fflush(stdout);
        tetris_wait_key(60000);
    }
    
    xui_term_alt_screen_leave();
    xui_term_restore();
    system("stty sane");
    fputs("\033[?25h", stdout);  // 显示光标
    printf("\n");
}

// 命令行入口
int cmd_xtetris(struct Command *cmd, struct ShellContext *ctx) {
    (void)cmd;
    (void)ctx;
    
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xtetris - 俄罗斯方块游戏\n\n");
        printf("用法:\n");
        printf("  xtetris          启动游戏\n");
        printf("  xtetris --help   显示帮助信息\n\n");
        printf("游戏控制:\n");
        printf("  A/D / 方向键     左右移动\n");
        printf("  W / 上           旋转\n");
        printf("  S / 下           加速下落\n");
        printf("  空格             立即落下\n");
        printf("  P               暂停\n");
        printf("  Q               退出\n\n");
        return 0;
    }
    
    xgame_tetris();
    return 0;
}
