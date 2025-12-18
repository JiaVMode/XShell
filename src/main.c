#include "xshell.h"
#include <stdio.h>

// 程序入口函数
int main(int argc, char *argv[])
{
    // 因为这个不需要参数，所以将两个参数忽略
    (void)argc;
    (void)argv;

    // 初始化Shell上下文
    ShellContext ctx;
    if (init_shell(&ctx) != 0) {
        fprintf(stderr, "Failed to initialize shell\n");
        return 1;
    }

    // 进入交互模式（主循环）
    shell_loop(&ctx);

    // 清理资源
    cleanup_shell(&ctx);

    return 0;
}