/**
 * @file score.c
 * @brief 游戏分数管理系统 - 保存和排行榜
 */

#define _POSIX_C_SOURCE 200809L
#include "xgame.h"
#include "xui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <pwd.h>

#define MAX_SCORES 10
#define SCORE_DIR ".xshell_scores"

typedef struct {
    char name[32];
    int score;
    time_t timestamp;
} ScoreEntry;

// 获取分数文件路径（保存在当前目录，和历史文件相同位置）
static void get_score_path(const char *game, char *path, size_t size) {
    char cwd[512];
    if (getcwd(cwd, sizeof(cwd))) {
        snprintf(path, size, "%s/.xshell_%s_scores", cwd, game);
    } else {
        snprintf(path, size, ".xshell_%s_scores", game);
    }
}

// 加载分数
int load_scores(const char *game, ScoreEntry *scores, int max_count) {
    char path[512];
    get_score_path(game, path, sizeof(path));
    
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    
    int count = 0;
    char line[256];
    while (count < max_count && fgets(line, sizeof(line), f)) {
        ScoreEntry *e = &scores[count];
        if (sscanf(line, "%31[^,],%d,%ld", e->name, &e->score, &e->timestamp) == 3) {
            count++;
        }
    }
    fclose(f);
    return count;
}

// 保存分数
void save_scores(const char *game, ScoreEntry *scores, int count) {
    char path[512];
    get_score_path(game, path, sizeof(path));
    
    FILE *f = fopen(path, "w");
    if (!f) return;
    
    for (int i = 0; i < count && i < MAX_SCORES; i++) {
        fprintf(f, "%s,%d,%ld\n", scores[i].name, scores[i].score, scores[i].timestamp);
    }
    fclose(f);
}

// 添加新分数（相同名字只保留最高分）
int add_score(const char *game, const char *name, int score) {
    ScoreEntry scores[MAX_SCORES + 1];
    int count = load_scores(game, scores, MAX_SCORES);
    
    // 检查是否已存在同名玩家
    int existing_idx = -1;
    for (int i = 0; i < count; i++) {
        if (strcmp(scores[i].name, name) == 0) {
            existing_idx = i;
            break;
        }
    }
    
    if (existing_idx >= 0) {
        // 同名玩家已存在，只有新分数更高时才更新
        if (score > scores[existing_idx].score) {
            scores[existing_idx].score = score;
            scores[existing_idx].timestamp = time(NULL);
        } else {
            // 新分数不够高，不更新
            // 查找当前排名并返回
            for (int i = 0; i < count; i++) {
                if (strcmp(scores[i].name, name) == 0) {
                    return i + 1;
                }
            }
            return 0;
        }
    } else {
        // 新玩家，添加记录
        ScoreEntry *new_entry = &scores[count];
        strncpy(new_entry->name, name, 31);
        new_entry->name[31] = '\0';
        new_entry->score = score;
        new_entry->timestamp = time(NULL);
        count++;
    }
    
    // 排序（降序）
    for (int i = 0; i < count - 1; i++) {
        for (int j = i + 1; j < count; j++) {
            if (scores[j].score > scores[i].score) {
                ScoreEntry tmp = scores[i];
                scores[i] = scores[j];
                scores[j] = tmp;
            }
        }
    }
    
    // 只保留前 MAX_SCORES 个
    if (count > MAX_SCORES) count = MAX_SCORES;
    
    save_scores(game, scores, count);
    
    // 返回排名（1-based）
    for (int i = 0; i < count; i++) {
        if (strcmp(scores[i].name, name) == 0) {
            return i + 1;
        }
    }
    return 0;
}

// 获取最高分
int get_high_score(const char *game) {
    ScoreEntry scores[1];
    if (load_scores(game, scores, 1) > 0) {
        return scores[0].score;
    }
    return 0;
}

// 显示排行榜
void show_leaderboard(const char *game, const char *title) {
    ScoreEntry scores[MAX_SCORES];
    int count = load_scores(game, scores, MAX_SCORES);
    
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    int box_w = 40;
    int box_h = count + 6;
    int ox = (term_w - box_w) / 2;
    int oy = (term_h - box_h) / 2;
    
    // 边框
    xui_term_set_fg256(39);
    xui_term_move_to(oy, ox);
    printf("╔");
    for (int i = 0; i < box_w - 2; i++) printf("═");
    printf("╗");
    
    for (int y = 1; y < box_h - 1; y++) {
        xui_term_move_to(oy + y, ox);
        printf("║");
        xui_term_move_to(oy + y, ox + box_w - 1);
        printf("║");
    }
    
    xui_term_move_to(oy + box_h - 1, ox);
    printf("╚");
    for (int i = 0; i < box_w - 2; i++) printf("═");
    printf("╝");
    xui_term_reset_style();
    
    // 标题
    xui_term_move_to(oy + 1, ox + (box_w - (int)strlen(title)) / 2);
    xui_term_set_fg256(220);
    xui_term_set_bold();
    printf("%s", title);
    xui_term_reset_style();
    
    // 表头
    xui_term_move_to(oy + 3, ox + 2);
    xui_term_set_fg256(244);
    printf("排名  玩家          分数");
    xui_term_reset_style();
    
    // 分数列表
    for (int i = 0; i < count; i++) {
        xui_term_move_to(oy + 4 + i, ox + 2);
        
        if (i == 0) xui_term_set_fg256(220);      // 金
        else if (i == 1) xui_term_set_fg256(250); // 银
        else if (i == 2) xui_term_set_fg256(208); // 铜
        else xui_term_set_fg256(252);
        
        printf("%2d.   %-12s  %6d", i + 1, scores[i].name, scores[i].score);
        xui_term_reset_style();
    }
    
    if (count == 0) {
        xui_term_move_to(oy + 4, ox + (box_w - 10) / 2);
        xui_term_set_fg256(244);
        printf("暂无记录");
        xui_term_reset_style();
    }
    
    fflush(stdout);
}

// 输入玩家名称
void input_player_name(char *name, int max_len) {
    int term_w, term_h;
    xui_term_get_size(&term_h, &term_w);
    
    int ox = (term_w - 30) / 2;
    int oy = term_h / 2;
    
    xui_term_move_to(oy, ox);
    xui_term_set_fg256(220);
    printf("输入你的名字: ");
    xui_term_reset_style();
    xui_term_show_cursor();
    fflush(stdout);
    
    // 手动读取字符
    int pos = 0;
    name[0] = '\0';
    
    while (pos < max_len - 1) {
        int ch = xui_term_read_key();
        
        if (ch == '\n' || ch == '\r' || ch == XUI_KEY_ENTER) {
            break;
        } else if (ch == 127 || ch == 8) { // Backspace
            if (pos > 0) {
                pos--;
                name[pos] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
        } else if (ch == 27) { // ESC - 使用默认名称
            pos = 0;
            break;
        } else if (ch >= 32 && ch < 127) { // 可打印字符
            name[pos++] = ch;
            name[pos] = '\0';
            printf("%c", ch);
            fflush(stdout);
        }
    }
    
    if (name[0] == '\0') {
        strcpy(name, "Player");
    }
    
    xui_term_hide_cursor();
}
