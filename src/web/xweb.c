/**
 * @file xweb.c
 * @brief XShell 终端网页浏览器实现
 * 
 * 使用 curl 或 wget 获取网页内容并显示
 */

#define _POSIX_C_SOURCE 200809L

#include "xweb.h"
#include "executor.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// ==================== 网页获取函数 ====================

int xweb_fetch(const char *url) {
    if (!url || url[0] == '\0') {
        fprintf(stderr, "xweb: URL 不能为空\n");
        return -1;
    }
    
    char cmd[8192];
    
        // 优先尝试文本浏览器 (效果最好)
    snprintf(cmd, sizeof(cmd), "which lynx > /dev/null 2>&1");
    if (system(cmd) == 0) {
        printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
        snprintf(cmd, sizeof(cmd), "lynx -dump -useragent='Mozilla/5.0' '%s' | head -100", url);
        int ret = system(cmd);
        printf("\n");
        return ret;
    }
    
    snprintf(cmd, sizeof(cmd), "which w3m > /dev/null 2>&1");
    if (system(cmd) == 0) {
        printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
        snprintf(cmd, sizeof(cmd), "w3m -dump '%s' | head -100", url);
        int ret = system(cmd);
        printf("\n");
        return ret;
    }
    
    snprintf(cmd, sizeof(cmd), "which links > /dev/null 2>&1");
    if (system(cmd) == 0) {
        printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
        snprintf(cmd, sizeof(cmd), "links -dump '%s' | head -100", url);
        int ret = system(cmd);
        printf("\n");
        return ret;
    }


    
    // 降级方案：使用 Python3 进行智能正文提取
    // 1. 尝试定位核心区域 (Bing: b_results, Baidu: content_left)
    // 2. 块级元素换行，改善排版
    // 3. 深度清理
    
    char filter[4096];
    
    // 尝试 Python3 (智能提取 - 线性逻辑修复版)
    snprintf(cmd, sizeof(cmd), "which python3 > /dev/null 2>&1");
    if (system(cmd) == 0) {
        strcpy(filter, "python3 -c \"import sys, re, html; c=sys.stdin.read(); "
               "m=re.search(r'<ol id=\\\"b_results\\\"[^>]*>(.*)</ol>', c, re.S|re.I); "
               "m=re.search(r'<div id=\\\"content_left\\\"[^>]*>(.*)</div>', c, re.S|re.I) if not m else m; "
               "m=re.search(r'<main[^>]*>(.*)</main>', c, re.S|re.I) if not m else m; "
               "m=re.search(r'<body[^>]*>(.*)</body>', c, re.S|re.I) if not m else m; "
               "c=m.group(1) if m else c; "
               "c=re.sub(r'<(script|style|noscript)[^>]*>.*?</\\1>', '', c, flags=re.S|re.I); "
               "c=re.sub(r'<!--.*?-->', '', c, flags=re.S); "
               "c=re.sub(r'</(div|p|li|h[1-6]|tr|br)>', '\\n', c, flags=re.I); "
               "c=re.sub(r'<[^>]+>', ' ', c); "
               "c=html.unescape(c); "
               "print('\\n'.join([l.strip() for l in c.splitlines() if l.strip()]))\"");
    } else {
        // 尝试 Perl
        snprintf(cmd, sizeof(cmd), "which perl > /dev/null 2>&1");
        if (system(cmd) == 0) {
            strcpy(filter, "perl -MHTML::Entities -0777 -pe 's/<(script|style|noscript)[^>]*>.*?<\\/\\1>//gis; s/<!--.*?-->//gs; s/<\\/(div|p|li|h[1-6]|tr|br)>/\\n/gi; s/<[^>]*>/ /g; decode_entities($_); s/^\\s+|\\s+$//gm; s/\\n\\s*\\n/\\n/g'");
        } else {
            // 最后降级到 sed
            strcpy(filter, "sed -e 's/<[^>]*>//g' -e '/^$/d'");
        }
    }

    // 检查是否安装了 curl
    snprintf(cmd, sizeof(cmd), "which curl > /dev/null 2>&1");
    if (system(cmd) == 0) {
        printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
        // 使用更完善的 header 模拟浏览器
        snprintf(cmd, sizeof(cmd), "curl -sL --compressed -A 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36' -H 'Accept-Language: zh-CN,zh;q=0.9,en;q=0.8' -H 'Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/webp,*/*;q=0.8' '%s' 2>/dev/null | %s | head -100", url, filter);
        int ret = system(cmd);
        printf("\n");
        return ret;
    }
    
    // 检查是否安装了 wget
    snprintf(cmd, sizeof(cmd), "which wget > /dev/null 2>&1");
    if (system(cmd) == 0) {
        printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
        snprintf(cmd, sizeof(cmd), "wget -qO- --header='Accept-Language: zh-CN,zh;q=0.9' --user-agent='Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36' '%s' 2>/dev/null | %s | head -100", url, filter);
        int ret = system(cmd);
        printf("\n");
        return ret;
    }
    
    // 没有可用工具
    fprintf(stderr, "xweb: 需要安装 curl 或 wget\n");
    fprintf(stderr, "  sudo apt install curl    # Ubuntu/Debian\n");
    fprintf(stderr, "  sudo yum install curl    # CentOS/RHEL\n");
    return -1;
}

// ==================== 交互式浏览器 ====================

int xweb_browser(ShellContext *ctx) {
    (void)ctx;
    
    printf("\n\033[1;36m=== 网页浏览器 ===\033[0m\n");
    printf("输入 URL 获取网页内容，输入 'exit' 退出\n");
    printf("常用命令:\n");
    printf("  http://example.com   - 获取网页内容\n");
    printf("  bing <关键词>        - Bing 搜索\n");
    printf("  sogou <关键词>       - 搜狗搜索 (暂替百度)\n");
    printf("  news                 - 科技新闻 (Hacker News)\n");
    printf("  cheat <命令>         - 命令速查 (cheat.sh)\n");
    printf("\n");
    
    char input[1024];
    
    while (1) {
        printf("\033[1;34mweb>\033[0m ");
        fflush(stdout);
        
        if (!fgets(input, sizeof(input), stdin)) {
            break;
        }
        input[strcspn(input, "\r\n")] = '\0';
        
        if (input[0] == '\0') {
            continue;
        }
        
        // 去除首尾空格
        char *cmd_start = input;
        while (*cmd_start && (*cmd_start == ' ' || *cmd_start == '\t')) {
            cmd_start++;
        }
        
        if (*cmd_start == '\0') continue;
        
        char *cmd_end = cmd_start + strlen(cmd_start) - 1;
        while (cmd_end > cmd_start && (*cmd_end == ' ' || *cmd_end == '\t' || *cmd_end == '\n' || *cmd_end == '\r')) {
            *cmd_end = '\0';
            cmd_end--;
        }
        
        // 退出命令
        if (strcmp(cmd_start, "exit") == 0 || strcmp(cmd_start, "quit") == 0 || strcmp(cmd_start, "q") == 0) {
            printf("退出网页浏览器\n");
            break;
        }
        
        // 帮助
        if (strcmp(cmd_start, "help") == 0 || strcmp(cmd_start, "?") == 0) {
            printf("\n可用命令:\n");
            printf("  <URL>        - 获取网页内容\n");
            printf("  bing <关键词>   - Bing 搜索\n");
            printf("  sogou <关键词>  - 搜狗搜索\n");
            printf("  news         - Hacker News 科技新闻\n");
            printf("  cheat <命令>    - Linux 命令速查表\n");
            printf("  weather      - 天气信息\n");
            printf("  exit         - 退出\n\n");
            continue;
        }

        // 天气查询
        if (strcmp(cmd_start, "weather") == 0) {
            printf("\n\033[1;33m正在获取天气信息...\033[0m\n");
            system("curl -s 'wttr.in?format=3' 2>/dev/null && echo");
            printf("\n");
            continue;
        }
        
        // 新闻 (Hacker News)
        if (strcmp(cmd_start, "news") == 0) {
            xweb_fetch("https://www.zaobao.com/");
            continue;
        }

        // Cheat Sheet
        if (strncmp(cmd_start, "cheat ", 6) == 0) {
            const char *cmd = cmd_start + 6;
            char url[1024];
            snprintf(url, sizeof(url), "https://cheat.sh/%s", cmd);
            // cheat.sh 返回纯文本，不需要 Python 过滤，但这里统一走 filter 也可以，或者直接 curl
            // 为了简单，直接 curl
            printf("\n\033[1;36m正在获取: %s\033[0m\n\n", url);
            char sys_cmd[2048];
            snprintf(sys_cmd, sizeof(sys_cmd), "curl -s '%s'", url);
            system(sys_cmd);
            printf("\n");
            continue;
        }
        
        // Bing 搜索
        if (strncmp(cmd_start, "bing ", 5) == 0) {
            const char *query = cmd_start + 5;
            char url[1024];
            snprintf(url, sizeof(url), "https://cn.bing.com/search?q=%s", query);
            xweb_fetch(url);
            continue;
        }
        
        // 搜狗搜索 (替换百度)
        if (strncmp(cmd_start, "sogou ", 6) == 0) {
            const char *query = cmd_start + 6;
            char url[1024];
            snprintf(url, sizeof(url), "https://www.sogou.com/web?query=%s", query);
            xweb_fetch(url);
            continue;
        }
        
        // 百度 (保留但作为备用, 使用搜狗替代)
        if (strncmp(cmd_start, "baidu ", 6) == 0) {
            printf("建议使用 sogou <关键词>，百度反爬虫较严。\n");
            const char *query = cmd_start + 6;
            char url[1024];
            snprintf(url, sizeof(url), "https://www.baidu.com/s?wd=%s", query);
            xweb_fetch(url);
            continue;
        }
        
        // 默认：获取URL内容
        // 如果没有协议前缀，添加 http://
        if (strncmp(cmd_start, "http://", 7) != 0 && strncmp(cmd_start, "https://", 8) != 0) {
            char url[1100];
            snprintf(url, sizeof(url), "http://%s", cmd_start);
            xweb_fetch(url);
        } else {
            xweb_fetch(cmd_start);
        }
    }
    
    return 0;
}

// ==================== 内置命令入口 ====================

int cmd_xweb(Command *cmd, ShellContext *ctx) {
    if (cmd->arg_count < 2) {
        // 无参数，进入交互模式
        return xweb_browser(ctx);
    }
    
    // 有参数，直接获取URL
    if (strcmp(cmd->args[1], "--help") == 0 || strcmp(cmd->args[1], "-h") == 0) {
        printf("用法: xweb [URL]\n");
        printf("在终端中浏览网页内容\n\n");
        printf("选项:\n");
        printf("  --help, -h   显示帮助信息\n\n");
        printf("无参数时进入交互模式\n");
        printf("交互模式中可使用: weather, bing <关键词>, baidu <关键词>\n");
        return 0;
    }
    
    return xweb_fetch(cmd->args[1]);
}
