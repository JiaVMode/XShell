/**
 * @file xui_term.c
 * @brief 终端控制函数
 */

#define _POSIX_C_SOURCE 200809L

#include "xui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <sys/select.h>

// 保存原始终端设置
static struct termios g_orig_termios;
static bool g_raw_mode = false;

// ==================== 终端模式控制 ====================

bool xui_term_init(void) {
    if (!isatty(STDIN_FILENO)) {
        return false;
    }
    
    if (tcgetattr(STDIN_FILENO, &g_orig_termios) < 0) {
        return false;
    }
    
    struct termios raw = g_orig_termios;
    // 关闭回显和规范模式
    raw.c_lflag &= (tcflag_t)~(ECHO | ICANON | ISIG);
    // 关闭输入处理
    raw.c_iflag &= (tcflag_t)~(IXON | ICRNL | BRKINT | INPCK | ISTRIP);
    // 设置字符大小
    raw.c_cflag |= (CS8);
    // 关闭输出处理
    raw.c_oflag &= (tcflag_t)~(OPOST);
    // 设置读取参数
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) < 0) {
        return false;
    }
    
    g_raw_mode = true;
    return true;
}

void xui_term_restore(void) {
    if (g_raw_mode) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_orig_termios);
        g_raw_mode = false;
    }
}

// ==================== 终端尺寸 ====================

void xui_term_get_size(int *rows, int *cols) {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_row > 0 && ws.ws_col > 0) {
        *rows = (int)ws.ws_row;
        *cols = (int)ws.ws_col;
    } else {
        *rows = 24;
        *cols = 80;
    }
}

// ==================== 能力检测 ====================

bool xui_term_supports_256color(void) {
    const char *term = getenv("TERM");
    if (!term) return false;
    return (strstr(term, "256color") != NULL || 
            strstr(term, "truecolor") != NULL ||
            strstr(term, "24bit") != NULL);
}

bool xui_term_supports_unicode(void) {
    const char *vars[] = {getenv("LC_ALL"), getenv("LC_CTYPE"), getenv("LANG")};
    for (size_t i = 0; i < sizeof(vars) / sizeof(vars[0]); i++) {
        const char *v = vars[i];
        if (!v) continue;
        if (strstr(v, "UTF-8") || strstr(v, "utf8") || strstr(v, "utf-8")) {
            return true;
        }
    }
    return false;
}

bool xui_term_supports_alt_screen(void) {
    const char *term = getenv("TERM");
    if (!term || strcmp(term, "dumb") == 0) {
        return false;
    }
    return true;
}

// ==================== 备用屏幕 ====================

void xui_term_alt_screen_enter(void) {
    fputs("\033[?1049h", stdout);  // 进入备用屏幕
    fflush(stdout);
}

void xui_term_alt_screen_leave(void) {
    fputs("\033[?1049l", stdout);  // 离开备用屏幕
    fflush(stdout);
}

// ==================== 屏幕控制 ====================

void xui_term_clear(void) {
    fputs("\033[2J\033[H", stdout);
    fflush(stdout);
}

void xui_term_move_to(int row, int col) {
    printf("\033[%d;%dH", row, col);
}

void xui_term_hide_cursor(void) {
    fputs("\033[?25l", stdout);
}

void xui_term_show_cursor(void) {
    fputs("\033[?25h", stdout);
}

// ==================== 颜色和样式 ====================

void xui_term_reset_style(void) {
    fputs("\033[0m", stdout);
}

void xui_term_set_fg256(int color) {
    printf("\033[38;5;%dm", color);
}

void xui_term_set_bg256(int color) {
    printf("\033[48;5;%dm", color);
}

void xui_term_set_bold(void) {
    fputs("\033[1m", stdout);
}

void xui_term_set_dim(void) {
    fputs("\033[2m", stdout);
}

// ==================== 输入处理 ====================

// 特殊按键定义


int xui_term_read_key(void) {
    unsigned char c;
    if (read(STDIN_FILENO, &c, 1) != 1) {
        return -1;
    }
    
    // 处理 Enter 键
    if (c == '\r' || c == '\n') {
        return XUI_KEY_ENTER;
    }
    
    // 处理转义序列
    if (c == 0x1b) {
        // 等待可能的后续字符
        struct timeval tv = {.tv_sec = 0, .tv_usec = 50000};  // 50ms
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        
        if (select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) <= 0) {
            return XUI_KEY_ESC;  // 单独的 ESC 键
        }
        
        unsigned char seq[3];
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return XUI_KEY_ESC;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return XUI_KEY_ESC;
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return XUI_KEY_UP;
                case 'B': return XUI_KEY_DOWN;
                case 'C': return XUI_KEY_RIGHT;
                case 'D': return XUI_KEY_LEFT;
            }
        }
        return XUI_KEY_ESC;
    }
    
    return (int)c;
}
