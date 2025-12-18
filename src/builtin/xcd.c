// 引入自定义头文件
#include "builtin.h"                        // 内置命令函数声明
#include "utils.h"                          // 工具函数（normalize_path）

// 引入标准库
#include <stdio.h>                          // 标准输入输出（perror）
#include <unistd.h>                         // UNIX 标准函数（chdir, getcwd, getenv）
#include <string.h>                         // 字符串处理（strcmp, strcpy, strncmp, snprintf）
#include <linux/limits.h>                   // 系统限制常量（PATH_MAX）
#include <errno.h>                          // 错误码（errno, strerror）

// xcd 命令实现
// 命令名称：xcd
// 对应系统命令：cd
// 使用示例：
//   [\home\user\]# xcd /tmp      -> 切换到 /tmp
//   [\tmp\]# xcd                 -> 返回主目录
//   [\home\user\]# xcd .         -> 返回上一个目录
int cmd_xcd(Command *cmd, ShellContext *ctx) {
    // 步骤0：检查是否请求帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xcd - 切换工作目录\n\n");
        printf("用法:\n");
        printf("  xcd [目录] [--help]\n\n");
        printf("说明:\n");
        printf("  改变当前工作目录。\n");
        printf("  Change Directory - 切换目录。\n\n");
        printf("参数:\n");
        printf("  目录      要切换到的目录路径\n");
        printf("            - 绝对路径：/home/user/Documents\n");
        printf("            - 相对路径：../parent 或 subdir\n");
        printf("            - 无参数：返回 HOME 目录\n");
        printf("            - ~    ：用户主目录\n");
        printf("            - ~/path：主目录下的路径\n");
        printf("            - .  ：当前目录（不变）\n");
        printf("            - .. ：上级目录\n\n");
        printf("选项:\n");
        printf("  --help    显示此帮助信息\n\n");
        printf("特性:\n");
        printf("  • 支持 Windows 风格路径（自动转换 \\ 为 /）\n");
        printf("  • 支持混合分隔符路径\n\n");
        printf("示例:\n");
        printf("  xcd              # 返回 HOME 目录\n");
        printf("  xcd ~            # 切换到用户主目录\n");
        printf("  xcd ~/Documents  # 切换到主目录下的 Documents\n");
        printf("  xcd /tmp         # 切换到 /tmp\n");
        printf("  xcd ..           # 上级目录\n");
        printf("  xcd LJ/XShell    # 相对路径\n");
        printf("  xcd LJ\\XShell    # Windows 风格\n\n");
        printf("对应系统命令: cd\n");
        return 0;
    }

    char *target_dir = NULL;                // 目标目录指针
    
    // 步骤1：根据参数量确定目标目录
    if (cmd->arg_count == 1) {
        // 情况1：无参数->返回用户主目录
        target_dir = ctx->home_dir;
    }
    else if (cmd->arg_count == 2) {
        // 情况2：xcd <参数>
        if (strcmp(cmd->args[1], ".") == 0) {
            // xcd . -> 返回上一个工作目录
            target_dir = ctx->prev_dir;
        }
        else if (strcmp(cmd->args[1], "..") == 0) {
            // xcd .. -> 返回上一级目录（父目录）
            target_dir = "..";
        }
        else if (strcmp(cmd->args[1], "~") == 0) {
            // xcd ~ -> 返回用户主目录
            target_dir = ctx->home_dir;
        }
        else if (cmd->args[1][0] == '~' && (cmd->args[1][1] == '/' || cmd->args[1][1] == '\0')) {
            // xcd ~/path -> 展开 ~ 为用户主目录
            if (ctx->home_dir == NULL) {
                XSHELL_LOG_ERROR(ctx, "xcd: HOME not set\n");
                return -1;
            }
            // 构建完整路径：home_dir + / + 剩余路径
            static char expanded_path[PATH_MAX];
            if (cmd->args[1][1] == '\0') {
                // 只有 ~
                strcpy(expanded_path, ctx->home_dir);
            } else {
                // ~/path
                snprintf(expanded_path, sizeof(expanded_path), "%s%s", ctx->home_dir, cmd->args[1] + 1);
            }
            normalize_path(expanded_path);
            target_dir = expanded_path;
        }
        else {
            // xcd <路径> -> 切换到指定路径
            // 规范化路径：将反斜杠 \ 转换为正斜杠 /
            normalize_path(cmd->args[1]);
            target_dir = cmd->args[1];
        }
    }
    else {
        // 参数过多
        XSHELL_LOG_ERROR(ctx, "xcd: too many arguments\n");
        return -1;
    }

    // 步骤2：保存当前目录到prev_dir（在切换前保存）
    char old_dir[PATH_MAX];
    if (getcwd(old_dir, sizeof(old_dir)) == NULL) {
        XSHELL_LOG_PERROR(ctx, "getcwd");
        return -1;
    }

    // 步骤3：执行切换目录
    // chdir(path)成功返回0， 失败返回-2
    if (chdir(target_dir) != 0) {
        XSHELL_LOG_PERROR(ctx, "chdir");
        return -1;
    }

    // 步骤4：更新Shell 上下文
    // 保存旧目录到prev_dir（用于下次 xcd.）
    strcpy(ctx->prev_dir, old_dir);

    // 更新当前工作目录到cwd
    if (getcwd(ctx->cwd, sizeof(ctx->cwd)) == NULL) {
        XSHELL_LOG_PERROR(ctx, "getcwd");
        return -1;
    }

    // 步骤5：返回成功状态
    return 0;
}