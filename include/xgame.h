#ifndef XGAME_H
#define XGAME_H

struct Command;
struct ShellContext;

// 贪吃蛇
void xgame_snake(void);
int cmd_xsnake(struct Command *cmd, struct ShellContext *ctx);

// 俄罗斯方块
void xgame_tetris(void);
int cmd_xtetris(struct Command *cmd, struct ShellContext *ctx);

// 2048
void xgame_2048(void);
int cmd_x2048(struct Command *cmd, struct ShellContext *ctx);

// 分数系统
int get_high_score(const char *game);
int add_score(const char *game, const char *name, int score);
void show_leaderboard(const char *game, const char *title);
void input_player_name(char *name, int max_len);

#endif // XGAME_H
