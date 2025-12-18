/*
 * xfile.c - 显示文件类型
 * 
 * 功能：检测并显示文件类型
 * 用法：xfile [选项] <文件>...
 * 
 * 选项：
 *   -b    简洁输出（不显示文件名）
 *   --help 显示帮助信息
 * 
 * 注意：简化实现，只检查常见类型
 */

#define _POSIX_C_SOURCE 200809L
#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <strings.h>

#define MAX_MAGIC_BYTES 16

// 显示帮助信息
static void show_help(const char *cmd_name) {
    printf("用法: %s [选项] <文件>...\n", cmd_name);
    printf("功能: 检测并显示文件类型\n");
    printf("选项:\n");
    printf("  -b, --brief     简洁输出（不显示文件名）\n");
    printf("  -h, --help      显示此帮助信息\n");
    printf("示例:\n");
    printf("  %s file.txt\n", cmd_name);
    printf("  %s -b file.txt\n", cmd_name);
}

// 检查文件魔数（简化实现）
static const char* check_magic(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        return NULL;
    }
    
    unsigned char magic[MAX_MAGIC_BYTES];
    size_t bytes_read = fread(magic, 1, MAX_MAGIC_BYTES, file);
    fclose(file);
    
    if (bytes_read < 2) {
        return NULL;
    }
    
    // 检查常见文件类型
    if (bytes_read >= 4) {
        // ELF 可执行文件
        if (magic[0] == 0x7f && magic[1] == 'E' && magic[2] == 'L' && magic[3] == 'F') {
            return "ELF executable";
        }
        // PNG 图片
        if (magic[0] == 0x89 && magic[1] == 'P' && magic[2] == 'N' && magic[3] == 'G') {
            return "PNG image";
        }
        // GIF 图片
        if (magic[0] == 'G' && magic[1] == 'I' && magic[2] == 'F') {
            return "GIF image";
        }
        // JPEG 图片
        if (magic[0] == 0xff && magic[1] == 0xd8 && magic[2] == 0xff) {
            return "JPEG image";
        }
        // ZIP 文件
        if (magic[0] == 'P' && magic[1] == 'K' && magic[2] == 0x03 && magic[3] == 0x04) {
            return "ZIP archive";
        }
    }
    
    if (bytes_read >= 2) {
        // PDF 文件
        if (magic[0] == '%' && magic[1] == 'P') {
            return "PDF document";
        }
    }
    
    return NULL;
}

// 根据扩展名判断文件类型
static const char* check_extension(const char *filename) {
    const char *ext = strrchr(filename, '.');
    if (ext == NULL) {
        return NULL;
    }
    ext++;
    
    if (strcasecmp(ext, "txt") == 0 || strcasecmp(ext, "text") == 0) {
        return "ASCII text";
    }
    if (strcasecmp(ext, "c") == 0 || strcasecmp(ext, "h") == 0) {
        return "C source";
    }
    if (strcasecmp(ext, "cpp") == 0 || strcasecmp(ext, "cxx") == 0 || strcasecmp(ext, "cc") == 0) {
        return "C++ source";
    }
    if (strcasecmp(ext, "py") == 0) {
        return "Python script";
    }
    if (strcasecmp(ext, "sh") == 0 || strcasecmp(ext, "bash") == 0) {
        return "shell script";
    }
    if (strcasecmp(ext, "html") == 0 || strcasecmp(ext, "htm") == 0) {
        return "HTML document";
    }
    if (strcasecmp(ext, "json") == 0) {
        return "JSON data";
    }
    if (strcasecmp(ext, "xml") == 0) {
        return "XML document";
    }
    
    return NULL;
}

// 检测文件类型
static int detect_file_type(const char *filename, int brief, ShellContext *ctx) {
    struct stat st;
    
    if (lstat(filename, &st) != 0) {
        XSHELL_LOG_ERROR(ctx, "xfile: %s: %s\n", filename, strerror(errno));
        return -1;
    }
    
    const char *type = NULL;
    
    // 检查文件类型
    if (S_ISDIR(st.st_mode)) {
        type = "directory";
    } else if (S_ISLNK(st.st_mode)) {
        type = "symbolic link";
    } else if (S_ISREG(st.st_mode)) {
        // 普通文件，检查魔数和扩展名
        type = check_magic(filename);
        if (type == NULL) {
            type = check_extension(filename);
        }
        if (type == NULL) {
            // 检查是否为可执行文件
            if (st.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
                type = "executable";
            } else {
                // 尝试读取前几个字节判断是否为文本文件
                FILE *file = fopen(filename, "r");
                if (file != NULL) {
                    int ch;
                    int is_text = 1;
                    int count = 0;
                    while ((ch = fgetc(file)) != EOF && count < 512) {
                        if (ch < 32 && ch != '\n' && ch != '\r' && ch != '\t') {
                            is_text = 0;
                            break;
                        }
                        count++;
                    }
                    fclose(file);
                    type = is_text ? "ASCII text" : "data";
                } else {
                    type = "regular file";
                }
            }
        }
    } else if (S_ISCHR(st.st_mode)) {
        type = "character device";
    } else if (S_ISBLK(st.st_mode)) {
        type = "block device";
    } else if (S_ISFIFO(st.st_mode)) {
        type = "FIFO/pipe";
    } else if (S_ISSOCK(st.st_mode)) {
        type = "socket";
    } else {
        type = "unknown";
    }
    
    if (brief) {
        printf("%s\n", type);
    } else {
        printf("%s: %s\n", filename, type);
    }
    
    return 0;
}

// xfile 命令实现
int cmd_xfile(Command *cmd, ShellContext *ctx) {
    int brief = 0;
    int has_files = 0;
    
    // 解析参数
    if (cmd->arg_count < 2) {
        show_help(cmd->name);
        return 0;
    }
    
    int i = 1;
    while (i < cmd->arg_count) {
        if (strcmp(cmd->args[i], "--help") == 0 || strcmp(cmd->args[i], "-h") == 0) {
            show_help(cmd->name);
            return 0;
        } else if (strcmp(cmd->args[i], "-b") == 0 || strcmp(cmd->args[i], "--brief") == 0) {
            brief = 1;
            i++;
        } else {
            break;
        }
    }
    
    // 处理文件
    for (; i < cmd->arg_count; i++) {
        detect_file_type(cmd->args[i], brief, ctx);
        has_files = 1;
    }
    
    if (!has_files) {
        XSHELL_LOG_ERROR(ctx, "xfile: 错误: 需要指定文件\n");
        show_help(cmd->name);
        return -1;
    }
    
    return 0;
}

