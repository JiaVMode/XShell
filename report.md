# XShell 项目汇报

## 一、项目概述

XShell 是一个**功能完整的 Linux Shell 实现**，使用纯 C 语言从零构建。本项目模拟了真实 Shell 的核心功能，支持 **60+ 内置命令**、**管道**、**I/O 重定向**、**命令链**、**Tab 补全**、**历史记录**、**别名系统**、**后台作业控制**等高级特性。

### 核心亮点

| 特性 | 描述 |
|------|------|
| 60+ 内置命令 | 涵盖文件操作、文本处理、系统管理等 |
| 命令管道 | 支持 `|` 管道连接多个命令 |
| I/O 重定向 | 支持 `>`、`>>`、`<`、`2>` 多种重定向 |
| 命令链 | 支持 `&&` 和 `||` 条件执行 |
| Tab 智能补全 | 支持命令名、路径、选项补全 |
| 历史记录 | 支持上下键浏览、Ctrl+R 反向搜索、持久化保存 |
| 别名系统 | 支持自定义命令别名 |
| 后台作业 | 支持 `&` 后台执行、`xjobs`/`xfg`/`xbg` 作业控制 |
| for 循环 | 支持 `for i in ...; do ...; done` 语法 |
| 大括号展开 | 支持 `{1..10}` 范围展开 |

---

## 二、项目架构

### 2.1 目录结构

```
XShell/
├── src/                    # 源代码目录
│   ├── main.c              # 程序入口（25行）
│   ├── xshell.c            # Shell 主循环（669行）
│   ├── parser.c            # 命令行解析器（584行）
│   ├── executor.c          # 命令执行器（1078行）
│   ├── input.c             # 输入处理模块（681行）
│   ├── history.c           # 历史记录管理（240行）
│   ├── completion.c        # 智能补全实现（600行）
│   ├── alias.c             # 别名系统（123行）
│   ├── job.c               # 作业管理（264行）
│   ├── utils.c             # 工具函数（约200行）
│   ├── builtin/            # 内置命令实现（65个文件）
│   ├── game/               # 游戏模块（贪吃蛇、俄罗斯方块、2048）
│   ├── UI/                 # UI系统（终端界面）
│   └── web/                # 网页浏览器模块
├── include/                # 头文件（13个）
├── Makefile                # 构建配置
└── tests/                  # 测试脚本
```

### 2.2 模块关系图

```
┌─────────────────────────────────────────────────────┐
│                    用户输入                          │
└─────────────────────┬───────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────┐
│  Input Module (input.c, completion.c, history.c)    │
│  • Tab 补全  • 行编辑  • 历史导航  • Ctrl快捷键      │
└─────────────────────┬───────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────┐
│            Parser Module (parser.c)                  │
│  • 词法分析  • 引号处理  • 变量展开  • 管道分割      │
└─────────────────────┬───────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────┐
│          Executor Module (executor.c)                │
│  • 内置命令分发  • 外部命令执行  • 管道/重定向       │
└─────────────────────┬───────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────┐
│        Built-in Commands (builtin/*.c)               │
│              65个内置命令实现文件                    │
└─────────────────────────────────────────────────────┘
```

---

## 三、核心模块实现详解

### 3.1 程序入口 (`main.c`)

**功能**：程序启动入口，初始化 Shell 环境并进入主循环。

```c
int main(int argc, char *argv[]) {
    ShellContext ctx;           // 声明 Shell 上下文结构体
    init_shell(&ctx);           // 初始化 Shell 环境
    shell_loop(&ctx);           // 进入主循环
    cleanup_shell(&ctx);        // 清理资源
    return 0;
}
```

**实现要点**：
- `ShellContext` 结构体存储 Shell 的全局状态（当前目录、上一目录、主目录、日志文件、运行标志等）
- 采用"初始化 → 主循环 → 清理"的标准程序结构

---

### 3.2 Shell 主循环 (`xshell.c`)

**功能**：Shell 的核心控制逻辑，负责初始化、命令读取、执行和清理。

#### 3.2.1 初始化函数 `init_shell()`

```c
int init_shell(ShellContext *ctx) {
    // 1. 获取当前工作目录
    getcwd(ctx->cwd, sizeof(ctx->cwd));
    
    // 2. 初始化上一目录为当前目录
    strcpy(ctx->prev_dir, ctx->cwd);
    
    // 3. 获取用户主目录（从 HOME 环境变量）
    ctx->home_dir = getenv("HOME");
    
    // 4. 初始化日志文件（.xshell_error）
    ctx->log_file = fopen(".xshell_error", "a");
    
    // 5. 设置运行标志
    ctx->running = 1;
    ctx->last_exit_status = 0;
}
```

#### 3.2.2 主循环 `shell_loop()`

```c
void shell_loop(ShellContext *ctx) {
    // 1. 初始化各子系统
    history_init();              // 历史记录系统
    alias_init();                // 别名系统
    job_init();                  // 作业管理系统
    job_install_signal_handler(); // 信号处理
    
    // 2. 显示欢迎信息
    printf("######## Welcome to XShell! ########\n");
    
    // 3. 主循环
    while (ctx->running) {
        display_prompt(ctx);                              // 显示提示符
        read_line_with_completion(line, size, callback);  // 读取输入（支持Tab补全）
        history_add(line);                                // 添加到历史
        execute_command_line(line, ctx);                  // 执行命令
    }
    
    // 4. 退出时清理
    history_cleanup();
    alias_cleanup();
}
```

#### 3.2.3 提示符显示 `display_prompt()`

```c
void display_prompt(ShellContext *ctx) {
    char display_path[PATH_MAX];
    
    // 如果在主目录，显示 ~
    if (strcmp(ctx->cwd, ctx->home_dir) == 0) {
        strcpy(display_path, "~");
    } else if (strncmp(ctx->cwd, ctx->home_dir, len) == 0) {
        // 在主目录下，用 ~ 替换主目录部分
        snprintf(display_path, size, "~%s", ctx->cwd + len);
    } else {
        strcpy(display_path, ctx->cwd);
    }
    
    // 将 / 替换为 \（Windows 风格）
    for (size_t i = 0; i < strlen(display_path); i++) {
        if (display_path[i] == '/') display_path[i] = '\\';
    }
    
    printf("[%s]# ", display_path);  // 格式：[路径]#
}
```

#### 3.2.4 命令链执行 `execute_command_line()`

**支持 `&&` 和 `||` 操作符**：

```c
int execute_command_line(const char *line, ShellContext *ctx) {
    // 1. 检查是否是 for 循环
    if (execute_for_loop(line, ctx) != 0) return;
    
    // 2. 分割 && 和 || 命令链
    while (current != NULL) {
        // 解析并执行当前命令
        Command *cmd = parse_command(current);
        last_status = execute_command(cmd, ctx);
        
        // 根据分隔符决定是否继续
        if (sep_type == 1 && last_status != 0) break;  // && 失败停止
        if (sep_type == 2 && last_status == 0) break;  // || 成功停止
        
        current = next;
    }
}
```

#### 3.2.5 for 循环支持 `execute_for_loop()`

**语法**：`for i in 1 2 3; do xecho $i; done` 或 `for i in {1..10}; do ...; done`

```c
static int execute_for_loop(const char *line, ShellContext *ctx) {
    // 1. 解析 "for VAR in LIST; do BODY done" 语法
    // 2. 提取变量名、列表、循环体
    // 3. 展开列表（支持大括号展开 {1..100}）
    // 4. 对每个列表值：
    //    - 将 $VAR 替换为当前值
    //    - 执行循环体
}
```

---

### 3.3 命令解析器 (`parser.c`)

**功能**：将用户输入的命令字符串解析为结构化的 `Command` 对象。

#### 3.3.1 Command 结构体

```c
typedef struct Command {
    char *name;           // 命令名
    char **args;          // 参数数组
    int arg_count;        // 参数数量
    
    // 重定向
    RedirectType redirect_type;  // 重定向类型
    char *redirect_file;         // 重定向文件
    char *stdout_file;           // 标准输出重定向
    char *stderr_file;           // 标准错误重定向
    char *stdin_file;            // 标准输入重定向
    int stdout_append;           // 是否追加模式
    
    // 管道
    struct Command *pipe_next;   // 管道中的下一个命令
    
    // 后台执行
    int background;              // 是否后台执行
} Command;
```

#### 3.3.2 核心解析流程

```c
Command* parse_command(const char *line) {
    // 1. 处理注释（# 后的内容）
    // 2. 检测后台执行符号 &
    // 3. 查找管道符号 |，分割命令链
    // 4. 对每个子命令调用 parse_single_command()
}

static Command* parse_single_command(char *line_start, char *line_end) {
    // 1. 分配 Command 对象
    // 2. 逐字符解析，识别：
    //    - 重定向符号：>、>>、<、2>
    //    - 引号字符串："..." 或 '...'
    //    - 普通参数
    // 3. 对每个 token 进行变量展开
    // 4. 设置重定向文件
}
```

#### 3.3.3 变量展开 `expand_variables()`

**支持的展开**：
- `$VAR` → 环境变量值
- `~` → 用户主目录

```c
static char* expand_variables(const char *input) {
    while (*p != '\0') {
        if (*p == '$' && (isalnum(p[1]) || p[1] == '_')) {
            // 提取变量名
            // 获取环境变量值 getenv(var_name)
            // 将值复制到输出
        } else if (*p == '~') {
            // 波浪号展开为 HOME 目录
        } else {
            // 普通字符直接复制
        }
    }
}
```

#### 3.3.4 重定向类型

```c
typedef enum {
    REDIRECT_NONE,    // 无重定向
    REDIRECT_OUT,     // >  覆盖输出
    REDIRECT_APPEND,  // >> 追加输出
    REDIRECT_IN,      // <  输入重定向
    REDIRECT_ERR      // 2> 错误重定向
} RedirectType;
```

---

### 3.4 命令执行器 (`executor.c`)

**功能**：执行解析后的命令，包括内置命令分发、外部命令执行、管道处理。

#### 3.4.1 执行流程

```c
int execute_command(Command *cmd, ShellContext *ctx) {
    // 1. 检查命令链（chain_next）
    // 2. 根据链类型（&& 或 ||）决定是否执行下一个命令
}

static int execute_single_command(Command *cmd, ShellContext *ctx) {
    // 1. 特殊处理 quit 命令
    if (strcmp(cmd->name, "quit") == 0) return cmd_quit(cmd, ctx);
    
    // 2. 展开大括号表达式（如 test{1..3}.txt）
    expanded_args = expand_args(cmd->args, cmd->arg_count);
    
    // 3. 检查是否有管道
    if (cmd->pipe_next != NULL) return execute_pipeline(cmd, ctx);
    
    // 4. 检查是否为内置命令
    if (is_builtin(cmd->name)) {
        if (has_redirect(cmd)) {
            // 有重定向：fork 子进程执行
            fork() → setup_redirect() → execute_builtin()
        } else {
            // 无重定向：直接执行
            return execute_builtin(cmd, ctx);
        }
    }
    
    // 5. 执行外部命令
    return execute_external(cmd, ctx);
}
```

#### 3.4.2 大括号展开 `expand_brace()`

**示例**：`test{1..3}.txt` → `test1.txt test2.txt test3.txt`

```c
char** expand_brace(const char *arg) {
    // 1. 查找 {start..end} 格式
    // 2. 提取前缀、起始值、结束值、后缀
    // 3. 生成范围内的所有字符串
    for (int i = start; i <= end; i++) {
        result[idx] = prefix + i + suffix;
    }
}
```

#### 3.4.3 管道执行 `execute_pipeline()`

```c
static int execute_pipeline(Command *cmd, ShellContext *ctx) {
    // 1. 计算管道数量
    // 2. 创建管道 pipe(pipes[i])
    // 3. 对每个命令 fork 子进程：
    //    - 设置输入管道（非第一个）：dup2(pipes[i-1][0], STDIN)
    //    - 设置输出管道（非最后一个）：dup2(pipes[i][1], STDOUT)
    //    - 关闭所有管道
    //    - 执行命令
    // 4. 父进程关闭所有管道
    // 5. 等待所有子进程
}
```

#### 3.4.4 外部命令执行 `execute_external()`

```c
static int execute_external(Command *cmd, ShellContext *ctx) {
    // 1. 在 PATH 中查找可执行文件
    char *exec_path = find_executable(cmd->name);
    
    // 2. fork 子进程
    pid_t pid = fork();
    
    if (pid == 0) {  // 子进程
        setup_redirect(cmd);        // 设置重定向
        execv(exec_path, cmd->args); // 执行
        exit(1);
    } else {         // 父进程
        if (cmd->background) {
            // 后台执行：添加到作业列表
            job_add(pid, cmd_str);
        } else {
            // 前台执行：等待子进程
            waitpid(pid, &status, 0);
        }
    }
}
```

#### 3.4.5 内置命令列表

```c
const char *builtins[] = {
    // 文件与目录操作
    "xpwd", "xcd", "xls", "xmkdir", "xrmdir", "xrm", "xcp", "xmv",
    "xtouch", "xln", "xstat", "xfind", "xtree", "xchmod", "xchown",
    
    // 文本处理
    "xcat", "xecho", "xgrep", "xhead", "xtail", "xwc", "xsort", 
    "xuniq", "xcut", "xpaste", "xtr", "xdiff",
    
    // 系统与进程
    "xps", "xkill", "xjobs", "xfg", "xbg", "xenv", "xexport", 
    "xunset", "xuname", "xuptime", "xhostname", "xwhoami", "xdate",
    
    // Shell 功能
    "xhistory", "xalias", "xunalias", "xsource", "xhelp", "xclear", "quit",
    
    // 其他工具
    "xcalc", "xmenu", "xtime", "xsleep", "xwhich", "xtype",
    
    // 特色功能
    "xui", "xweb", "xsnake", "xtetris", "x2048", "xsysmon"
};
```

---

### 3.5 输入处理模块 (`input.c`)

**功能**：实现类似 Bash 的交互式输入，支持 Tab 补全、行编辑、历史导航。

#### 3.5.1 核心特性

| 按键 | 功能 |
|------|------|
| Tab | 第一次：补全公共前缀；第二次：显示所有匹配 |
| ↑/↓ | 浏览历史记录 |
| ←/→ | 光标左右移动 |
| Backspace | 删除字符 |
| Ctrl+A | 移到行首 |
| Ctrl+E | 移到行尾 |
| Ctrl+U | 清除从行首到光标的内容 |
| Ctrl+L | 清屏并重新显示当前行 |
| Ctrl+R | 反向搜索历史 |
| Ctrl+C | 取消当前输入 |
| Ctrl+D | 退出 Shell（输入为空时） |

#### 3.5.2 终端原始模式

```c
char* read_line_with_completion(char *buffer, size_t size, PromptCallback callback) {
    struct termios old_term, new_term;
    tcgetattr(STDIN_FILENO, &old_term);       // 保存原始设置
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO | ISIG);  // 关闭规范模式、回显、信号
    tcsetattr(STDIN_FILENO, TCSANOW, &new_term);  // 应用新设置
    
    while (1) {
        ch = getchar();  // 逐字符读取
        // 处理各种按键...
    }
    
    tcsetattr(STDIN_FILENO, TCSANOW, &old_term);  // 恢复原始设置
}
```

#### 3.5.3 双击 Tab 处理

```c
if (ch == KEY_TAB) {
    int is_double_tab = (last_was_tab && strcmp(buffer, last_input) == 0);
    
    char **matches = NULL;
    int count = get_smart_completions(buffer, pos, &matches);
    
    if (count == 0) {
        printf("\a");  // 响铃提示无匹配
    } else if (count == 1) {
        // 直接补全
    } else {
        if (!is_double_tab) {
            // 第一次 Tab：补全公共前缀
            get_common_prefix(matches, count, common);
            // 补全公共部分
        } else {
            // 第二次 Tab：显示所有匹配项
            for (int i = 0; i < count; i++) {
                printf("%s  ", matches[i]);
            }
        }
    }
}
```

#### 3.5.4 Ctrl+R 反向搜索

```c
else if (ch == KEY_CTRL_R) {
    char query[256] = {0};
    
    while (1) {
        printf("\r\033[K(reverse-i-search)`%s`: %s", query, match);
        
        int sch = getchar();
        if (sch == KEY_ESC) { /* 取消 */ }
        if (sch == KEY_RETURN) { /* 接受匹配 */ }
        if (sch == KEY_BACKSPACE) { /* 删除查询字符 */ }
        if (sch >= 32) {
            query[qlen++] = sch;
            match = history_reverse_search(query);
        }
    }
}
```

---

### 3.6 智能补全模块 (`completion.c`)

**功能**：实现上下文感知的智能补全。

#### 3.6.1 补全类型

```c
typedef enum {
    COMPLETION_TYPE_COMMAND,    // 命令名补全
    COMPLETION_TYPE_PATH,       // 路径补全
    COMPLETION_TYPE_DIR_ONLY,   // 仅目录补全（如 xcd）
    COMPLETION_TYPE_FILE_ONLY,  // 仅文件补全
    COMPLETION_TYPE_OPTION      // 选项补全（--help 等）
} CompletionType;
```

#### 3.6.2 智能补全入口

```c
int get_smart_completions(const char *input, int cursor_pos, char ***matches) {
    CompletionType type = get_completion_type(input, cursor_pos);
    
    switch (type) {
        case COMPLETION_TYPE_COMMAND:
            // 命令名补全：匹配内置命令列表
            return get_command_completions(partial, matches);
            
        case COMPLETION_TYPE_OPTION:
            // 选项补全：根据命令返回可用选项
            return get_option_completions(cmd_name, partial, matches);
            
        case COMPLETION_TYPE_DIR_ONLY:
        case COMPLETION_TYPE_PATH:
        default:
            // 路径补全：遍历目录匹配文件
            return get_enhanced_path_completions(input, cursor_pos, type, matches);
    }
}
```

#### 3.6.3 路径补全实现

```c
int get_path_completions(const char *input, int cursor_pos, char ***matches) {
    // 1. 提取路径部分
    extract_path_to_complete(input, cursor_pos, prefix, partial);
    
    // 2. 确定搜索目录
    char *search_dir = (prefix[0] == '\0') ? "." : prefix;
    
    // 3. 打开目录并遍历
    DIR *dir = opendir(search_dir);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, partial, partial_len) == 0) {
            // 匹配！检查是否为目录，添加 / 后缀
            if (S_ISDIR(st.st_mode)) {
                sprintf(matches[i], "%s/", entry->d_name);
            } else {
                matches[i] = strdup(entry->d_name);
            }
        }
    }
}
```

---

### 3.7 历史记录模块 (`history.c`)

**功能**：管理命令历史，支持导航和持久化。

#### 3.7.1 数据结构

```c
static char *history[MAX_HISTORY];   // 历史记录数组（1000条）
static int history_count_value = 0;  // 当前记录数量
static char history_file[256];       // 历史文件路径（.xshell_history）
```

#### 3.7.2 核心功能

| 函数 | 功能 |
|------|------|
| `history_init()` | 初始化并从文件加载历史 |
| `history_add(line)` | 添加命令（自动去重、去空白） |
| `history_prev(index)` | 获取上一条（↑键） |
| `history_next(index)` | 获取下一条（↓键） |
| `history_save()` | 保存到文件 |
| `history_cleanup()` | 保存并释放内存 |

#### 3.7.3 添加历史记录

```c
void history_add(const char *line) {
    // 1. 跳过空行
    // 2. 跳过与上一条重复的命令
    // 3. 如果已满，删除最旧的记录
    if (history_count < MAX_HISTORY) {
        history[history_count++] = strdup(line);
    } else {
        free(history[0]);
        memmove(history, history + 1, (MAX_HISTORY - 1) * sizeof(char*));
        history[MAX_HISTORY - 1] = strdup(line);
    }
}
```

---

### 3.8 别名系统 (`alias.c`)

**功能**：管理命令别名，允许用户自定义快捷命令。

#### 3.8.1 数据结构

```c
typedef struct {
    char name[64];   // 别名名称
    char value[256]; // 别名值（实际命令）
} Alias;

static Alias aliases[MAX_ALIASES];  // 别名表（100个）
static int alias_count_value = 0;
```

#### 3.8.2 核心功能

| 函数 | 功能 |
|------|------|
| `alias_set(name, value)` | 设置或更新别名 |
| `alias_get(name)` | 获取别名对应的命令 |
| `alias_remove(name)` | 删除别名 |
| `alias_list()` | 显示所有别名 |

**使用示例**：
```bash
xalias ll="xls -l"     # 设置别名
ll                     # 使用别名
xunalias ll           # 删除别名
```

---

### 3.9 作业控制模块 (`job.c`)

**功能**：管理后台作业，支持作业状态跟踪和控制。

#### 3.9.1 数据结构

```c
typedef enum {
    JOB_RUNNING,  // 运行中
    JOB_STOPPED,  // 已停止（Ctrl+Z）
    JOB_DONE      // 已完成
} JobStatus;

typedef struct {
    int id;              // 作业 ID
    pid_t pid;           // 进程 ID
    JobStatus status;    // 作业状态
    char command[256];   // 命令字符串
    bool notified;       // 是否已通知完成
} Job;
```

#### 3.9.2 信号处理

```c
void job_install_signal_handler(void) {
    // SIGCHLD：子进程状态变化
    sigaction(SIGCHLD, &sa, NULL);
    
    // SIGINT (Ctrl+C)：只中断当前输入，不退出 Shell
    sigaction(SIGINT, &sa, NULL);
    
    // SIGTSTP (Ctrl+Z)：Shell 本身忽略
    sigaction(SIGTSTP, &sa, NULL);
    
    // 忽略其他信号
    signal(SIGQUIT, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
}
```

**使用示例**：
```bash
xsleep 10 &    # 后台执行
xjobs          # 显示作业列表
xfg 1          # 将作业1调到前台
xkill 1        # 终止作业1
```

---

## 四、内置命令实现示例

### 4.1 文件操作命令

#### `xls` - 列出目录内容

**实现文件**：`src/builtin/xls.c`（17258字节，最大的内置命令）

**支持选项**：
- `-l`：长格式显示（权限、大小、时间等）
- `-a`：显示隐藏文件
- `-h`：人性化显示大小
- `-R`：递归显示子目录

**核心实现**：
```c
int cmd_xls(Command *cmd, ShellContext *ctx) {
    // 1. 解析命令行选项
    int show_long = 0, show_all = 0, human_readable = 0;
    
    // 2. 打开目录
    DIR *dir = opendir(target_dir);
    
    // 3. 遍历目录项
    while ((entry = readdir(dir)) != NULL) {
        if (!show_all && entry->d_name[0] == '.') continue;
        
        if (show_long) {
            // 获取文件状态
            stat(entry->d_name, &st);
            // 格式化输出：权限 链接数 用户 组 大小 时间 文件名
            print_long_format(entry->d_name, &st);
        } else {
            printf("%s  ", entry->d_name);
        }
    }
}
```

#### `xcp` - 复制文件/目录

**支持选项**：
- `-r/-R`：递归复制目录
- `--progress`：显示进度

**核心实现**：
```c
int cmd_xcp(Command *cmd, ShellContext *ctx) {
    if (recursive && S_ISDIR(st.st_mode)) {
        // 递归复制目录
        copy_directory_recursive(src, dst);
    } else {
        // 复制单个文件
        int src_fd = open(src, O_RDONLY);
        int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        while ((bytes = read(src_fd, buffer, BUFSIZE)) > 0) {
            write(dst_fd, buffer, bytes);
        }
    }
}
```

### 4.2 文本处理命令

#### `xgrep` - 文本搜索

**支持选项**：
- `-i`：忽略大小写
- `-n`：显示行号
- `-v`：反向匹配
- `-c`：只显示匹配行数
- `-r`：递归搜索目录

**核心实现**：
```c
int cmd_xgrep(Command *cmd, ShellContext *ctx) {
    while (fgets(line, sizeof(line), fp)) {
        int match = (strstr(line, pattern) != NULL);
        if (ignore_case) {
            // 大小写不敏感匹配
            match = strcasestr(line, pattern) != NULL;
        }
        if (invert_match) match = !match;
        
        if (match) {
            if (show_line_numbers) printf("%d:", line_num);
            printf("%s", line);
        }
    }
}
```

#### `xwc` - 统计行数/字数/字节数

**支持选项**：
- `-l`：只统计行数
- `-w`：只统计单词数
- `-c`：只统计字节数

### 4.3 系统命令

#### `xps` - 显示进程信息

**核心实现**：
```c
int cmd_xps(Command *cmd, ShellContext *ctx) {
    DIR *proc = opendir("/proc");
    
    // 遍历 /proc 目录
    while ((entry = readdir(proc)) != NULL) {
        if (!isdigit(entry->d_name[0])) continue;
        
        pid_t pid = atoi(entry->d_name);
        
        // 读取 /proc/[pid]/stat 获取进程信息
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        // 解析状态文件...
        
        // 读取 /proc/[pid]/cmdline 获取命令行
        snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
        // 读取并显示...
    }
}
```

#### `xkill` - 终止进程

**支持的信号**：
- `xkill PID`：发送 SIGTERM
- `xkill -9 PID`：发送 SIGKILL
- `xkill -SIGSTOP PID`：发送 SIGSTOP

---

## 五、高级特性

### 5.1 管道示例

```bash
xls -l | xgrep ".c" | xwc -l     # 统计 .c 文件数量
xcat file.txt | xsort | xuniq    # 排序并去重
xps | xgrep xshell               # 查找 xshell 进程
```

### 5.2 重定向示例

```bash
xecho "Hello" > output.txt       # 覆盖写入
xecho "World" >> output.txt      # 追加写入
xcat < input.txt                 # 输入重定向
xls /nonexistent 2> error.log    # 错误重定向
```

### 5.3 命令链示例

```bash
xcd /tmp && xpwd                 # 成功后执行
xls /nonexistent || xecho "Failed"  # 失败后执行
xmkdir test && xcd test && xtouch file.txt  # 链式执行
```

### 5.4 for 循环示例

```bash
for i in 1 2 3; do xecho $i; done           # 打印 1 2 3
for i in {1..100}; do xecho $i; done        # 打印 1 到 100
for f in *.c; do xwc -l $f; done            # 统计所有 .c 文件行数
```

### 5.5 特色功能

| 命令 | 功能 |
|------|------|
| `xmenu` | 交互式菜单系统 |
| `xui` | 终端 UI 界面 |
| `xweb` | 终端网页浏览器 |
| `xsnake` | 贪吃蛇游戏 |
| `xtetris` | 俄罗斯方块游戏 |
| `x2048` | 2048 游戏 |
| `xsysmon` | 系统监控工具 |

---

## 六、技术要点总结

### 6.1 使用的系统调用

| 系统调用 | 用途 |
|----------|------|
| `fork()` | 创建子进程执行外部命令 |
| `execv()` | 执行外部程序 |
| `waitpid()` | 等待子进程结束 |
| `pipe()` | 创建管道 |
| `dup2()` | 重定向文件描述符 |
| `open()/close()` | 文件操作 |
| `stat()/lstat()` | 获取文件状态 |
| `opendir()/readdir()` | 目录遍历 |
| `tcgetattr()/tcsetattr()` | 终端控制 |
| `sigaction()` | 信号处理 |

### 6.2 关键数据结构

| 结构体 | 用途 |
|--------|------|
| `ShellContext` | Shell 全局状态 |
| `Command` | 解析后的命令表示 |
| `Job` | 后台作业信息 |
| `Alias` | 别名映射 |
| `struct termios` | 终端设置 |
| `struct stat` | 文件状态 |
| `struct dirent` | 目录项 |

### 6.3 代码统计

| 模块 | 文件数 | 代码行数 |
|------|--------|----------|
| 核心模块 | 10 | ~4,500 |
| 内置命令 | 65 | ~10,000 |
| 头文件 | 13 | ~500 |
| **总计** | **88** | **~15,000** |

---

## 七、编译与运行

### 编译命令

```bash
make clean && make    # 编译
make debug            # 调试模式编译
```

### 运行

```bash
./xshell              # 启动 Shell
```

### 退出

```bash
quit                  # 退出命令
# 或按 Ctrl+D（输入为空时）
```

---

## 八、命令实现详解（第一部分：1-10）

本章节详细介绍每个内置命令的实现过程，力求让没有编程基础的读者也能理解。

---

### 8.1 `xpwd` - 显示当前目录

**功能说明**：打印当前工作目录的完整路径，类似于系统的 `pwd` 命令。

**通俗理解**：
想象你在一栋大楼里，`xpwd` 就像是问"我现在在哪里？"，系统会告诉你"你在A栋3楼302室"这样的完整地址。

**实现流程**：

```
用户输入 xpwd
     ↓
[步骤1] 检查是否请求帮助（--help）
     ↓
[步骤2] 准备一个"容器"存放路径
     │      （技术上叫缓冲区，大小为PATH_MAX，通常4096字节）
     ↓
[步骤3] 调用系统函数 getcwd()
     │      getcwd = get current working directory
     │      这是Linux系统提供的"询问当前位置"功能
     ↓
[步骤4] 判断是否成功获取
     ├── 成功 → 打印路径，返回0（成功）
     └── 失败 → 打印错误信息，返回-1（失败）
```

**核心代码解读**：

```c
char cwd[PATH_MAX];                    // 准备一个足够大的数组存放路径

if (getcwd(cwd, sizeof(cwd)) != NULL) { // 调用系统函数获取当前目录
    printf("%s\n", cwd);               // 成功，打印路径
    return 0;                          // 返回成功
} else {
    perror("getcwd");                  // 失败，打印错误原因
    return -1;                         // 返回失败
}
```

**关键点**：
- `getcwd()` 是系统提供的标准函数，直接询问操作系统"我在哪"
- 使用 `PATH_MAX`（通常4096字节）确保能存储任何合法路径
- 返回值约定：**0 表示成功，非0 表示失败**，这是 Shell 命令的通用约定

---

### 8.2 `quit` - 退出 Shell

**功能说明**：退出 XShell 程序，返回到系统 Shell。

**通俗理解**：
就像关闭一个应用程序的"退出"按钮。当你想离开 XShell 时，输入 `quit`，Shell 就会说"再见"然后关闭。

**实现流程**：

```
用户输入 quit
     ↓
[步骤1] 检查是否请求帮助（--help）
     ↓
[步骤2] 设置"退出标志" ctx->running = 0
     │      （ctx是Shell的"状态管理器"，running=1表示继续运行）
     ↓
[步骤3] 返回 0（成功）
     ↓
主循环检测到 running=0，停止循环，Shell 退出
```

**核心代码解读**：

```c
// Shell 主循环（在xshell.c中）
while (ctx->running) {        // running=1 时持续循环
    // 读取命令...
    // 执行命令...
}

// quit 命令实现
int cmd_quit(Command *cmd, ShellContext *ctx) {
    ctx->running = 0;         // 把开关"关掉"
    return 0;                 // 返回成功
}
```

**关键点**：
- Shell 是一个"无限循环"程序，不断等待用户输入
- `quit` 通过设置 `running = 0` 来"打破"这个循环
- 这是一种**控制流设计模式**：用标志变量控制循环

---

### 8.3 `xcd` - 切换目录

**功能说明**：改变当前工作目录，类似于系统的 `cd` 命令。

**通俗理解**：
就像在文件管理器中双击文件夹进入不同目录。`xcd /tmp` 相当于"我要去/tmp文件夹"。

**实现流程**：

```
用户输入 xcd /tmp
     ↓
[步骤1] 解析参数，确定目标目录
     ├── 无参数    → 目标 = 用户主目录 (HOME)
     ├── xcd .    → 目标 = 上次访问的目录
     ├── xcd ..   → 目标 = 上一级目录
     ├── xcd ~    → 目标 = 用户主目录
     ├── xcd ~/Documents → 展开 ~ 为主目录
     └── xcd /tmp → 目标 = 指定路径
     ↓
[步骤2] 保存当前目录到 old_dir（用于下次 xcd .）
     ↓
[步骤3] 调用 chdir(target_dir) 切换目录
     │      chdir = change directory，系统提供的切换目录函数
     ↓
[步骤4] 判断是否成功
     ├── 成功 → 更新保存的目录信息，返回0
     └── 失败 → 打印错误（如"目录不存在"），返回-1
```

**核心代码解读**：

```c
// 步骤1: 确定目标目录
if (cmd->arg_count == 1) {
    target_dir = ctx->home_dir;           // 无参数，回主目录
}
else if (strcmp(cmd->args[1], ".") == 0) {
    target_dir = ctx->prev_dir;           // xcd . 回到上次的目录
}
else if (strcmp(cmd->args[1], "..") == 0) {
    target_dir = "..";                    // 上级目录
}

// 步骤2: 保存当前目录
getcwd(old_dir, sizeof(old_dir));

// 步骤3: 执行切换
if (chdir(target_dir) != 0) {
    perror("chdir");                      // 失败，打印原因
    return -1;
}

// 步骤4: 更新状态
strcpy(ctx->prev_dir, old_dir);           // 记住旧目录
getcwd(ctx->cwd, sizeof(ctx->cwd));       // 更新当前目录
```

**关键点**：
- `chdir()` 是系统函数，真正执行目录切换
- 需要记住"上一个目录"，这样 `xcd .` 才能返回
- 支持 `~` 展开：将 `~` 替换为 HOME 环境变量的值

---

### 8.4 `xmkdir` - 创建目录

**功能说明**：创建一个或多个目录，支持 `-p` 选项创建多级目录。

**通俗理解**：
就像在文件管理器中右键"新建文件夹"。`xmkdir -p a/b/c` 可以一次性创建 a、a/b、a/b/c 三个嵌套文件夹。

**实现流程**：

```
用户输入 xmkdir -p project/src/main
     ↓
[步骤1] 解析选项
     ├── 检测到 -p → 启用"递归创建"模式
     └── 未检测到 → 普通模式（父目录必须存在）
     ↓
[步骤2] 遍历所有目录参数
     ↓
[步骤3] 对每个目录执行创建
     ├── 普通模式 → 直接调用 mkdir(目录名, 权限)
     └── -p 模式  → 调用递归创建函数
     ↓
[步骤4] 返回结果
```

**-p 递归创建的原理**：

```
要创建: project/src/main

递归函数的工作方式：
1. 路径 = "project/src/main"
2. 找到第一个 / → 位置在 "project" 之后
3. 临时截断为 "project"，尝试创建
4. 恢复为 "project/src/main"
5. 找到第二个 / → 位置在 "src" 之后
6. 临时截断为 "project/src"，尝试创建
7. 恢复，最后创建 "project/src/main"
```

**核心代码解读**：

```c
// 递归创建函数
static int mkdir_recursive(const char *path, mode_t mode) {
    char tmp[PATH_MAX];
    strcpy(tmp, path);                    // 复制路径
    
    // 逐级创建
    for (p = tmp + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';                    // 临时截断
            mkdir(tmp, mode);             // 创建当前级别
            *p = '/';                     // 恢复
        }
    }
    mkdir(tmp, mode);                     // 创建最后一级
}
```

**关键点**：
- `mkdir()` 系统函数只能创建单个目录
- `-p` 选项需要自己实现"逐级创建"逻辑
- 权限 `0755` 表示：所有者可读写执行，其他人可读和执行

---

### 8.5 `xrmdir` - 删除空目录

**功能说明**：删除一个或多个**空**目录。

**通俗理解**：
只能删除空文件夹。如果文件夹里有东西，就会报错"目录非空"。

**实现流程**：

```
用户输入 xrmdir empty_folder
     ↓
[步骤1] 检查参数是否足够
     ↓
[步骤2] 遍历所有目录参数
     ↓
[步骤3] 对每个目录调用 rmdir()
     ├── 成功 → 继续下一个
     └── 失败 → 报错（可能是目录非空、不存在、权限不足）
     ↓
[步骤4] 返回结果
```

**核心代码解读**：

```c
for (int i = 1; i < cmd->arg_count; i++) {
    const char *dirname = cmd->args[i];
    
    if (rmdir(dirname) == -1) {           // 尝试删除
        perror(dirname);                  // 失败，打印原因
        has_error = 1;
    }
}
```

**关键点**：
- `rmdir()` 系统函数只能删除**空**目录
- 如果目录非空，需要使用 `xrm -r` 递归删除
- 这是一种安全设计，防止误删

---

### 8.6 `xtouch` - 创建文件/更新时间戳

**功能说明**：如果文件不存在则创建空文件，如果存在则更新其访问和修改时间。

**通俗理解**：
像是"轻轻触碰"一个文件：
- 文件不存在 → 创建一个新的空文件
- 文件已存在 → 更新它的"最后修改时间"为现在

**实现流程**：

```
用户输入 xtouch myfile.txt
     ↓
[步骤1] 遍历所有文件参数
     ↓
[步骤2] 对每个文件，先尝试更新时间戳
     │      调用 utime(filename, NULL)
     │      NULL 表示设置为当前时间
     ↓
[步骤3] 判断 utime 结果
     ├── 成功 → 文件存在，时间已更新，处理下一个
     └── 失败 → 检查失败原因
              ├── ENOENT（文件不存在） → 创建新文件
              └── 其他错误 → 报错
     ↓
[步骤4] 创建新文件（如果需要）
     │      open(filename, O_CREAT | O_WRONLY, 0644)
     ↓
[步骤5] 返回结果
```

**核心代码解读**：

```c
for (int i = 1; i < cmd->arg_count; i++) {
    const char *filename = cmd->args[i];
    
    // 尝试更新时间戳
    if (utime(filename, NULL) == 0) {
        continue;                         // 成功，处理下一个文件
    }
    
    // 检查是否是"文件不存在"错误
    if (errno != ENOENT) {               // 其他错误
        perror(filename);
        has_error = 1;
        continue;
    }
    
    // 文件不存在，创建新文件
    int fd = open(filename, O_CREAT | O_WRONLY, 0644);
    if (fd == -1) {
        perror(filename);                // 创建失败
        has_error = 1;
    } else {
        close(fd);                       // 创建成功，关闭文件
    }
}
```

**关键点**：
- `utime(file, NULL)` 将时间戳更新为当前时间
- `O_CREAT` 标志告诉 `open()` "如果文件不存在就创建"
- `0644` 权限表示：所有者可读写，其他人只读

---

### 8.7 `xrm` - 删除文件或目录

**功能说明**：删除文件或目录。删除目录需要 `-r` 选项进行递归删除。

**通俗理解**：
- `xrm file.txt` → 删除单个文件
- `xrm -r folder` → 删除整个文件夹（包括里面所有内容）
- `xrm -f file.txt` → 强制删除，如果文件不存在也不报错

**实现流程**：

```
用户输入 xrm -rf old_project
     ↓
[步骤1] 解析选项
     ├── -r/-R → 启用递归删除
     ├── -f    → 启用强制模式
     └── -rf   → 同时启用（可以组合）
     ↓
[步骤2] 遍历所有要删除的路径
     ↓
[步骤3] 获取文件信息（判断是文件还是目录）
     ├── 是目录
     │    ├── 有 -r 选项 → 调用递归删除函数
     │    └── 无 -r 选项 → 报错"需要-r选项"
     └── 是文件 → 调用 unlink() 删除
     ↓
[步骤4] 返回结果
```

**递归删除目录的原理**：

```
要删除: old_project/
          ├── src/
          │    └── main.c
          └── README.md

递归删除过程：
1. 打开目录 old_project/
2. 遍历内容，发现 src/ 是目录 → 递归处理
   2.1 打开目录 src/
   2.2 删除文件 main.c
   2.3 src/ 现在是空的，删除 src/
3. 删除文件 README.md
4. old_project/ 现在是空的，删除 old_project/
```

**核心代码解读**：

```c
// 递归删除目录
static int remove_directory_recursive(const char *path) {
    DIR *dir = opendir(path);             // 打开目录
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {  // 遍历每一项
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;
        
        // 构建完整路径
        snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
        
        // 获取文件信息
        lstat(filepath, &statbuf);
        
        if (S_ISDIR(statbuf.st_mode)) {
            remove_directory_recursive(filepath);  // 是目录，递归
        } else {
            unlink(filepath);             // 是文件，直接删除
        }
    }
    
    closedir(dir);
    rmdir(path);                          // 最后删除空目录本身
}
```

**关键点**：
- `unlink()` 删除文件
- `rmdir()` 删除空目录
- 递归思想：先删除内容，再删除容器
- `-f` 选项让命令在文件不存在时静默忽略

---

### 8.8 `xecho` - 输出文本

**功能说明**：将文本输出到标准输出，支持转义字符和多种选项。

**通俗理解**：
就像"复读机"，你说什么它就输出什么。但它还能理解一些"暗号"（转义字符），比如 `\n` 表示换行。

**实现流程**：

```
用户输入 xecho -e "Hello\nWorld"
     ↓
[步骤1] 解析选项
     ├── -n    → 不输出末尾换行符
     ├── -e    → 解释转义字符（\n, \t 等）
     ├── -E    → 不解释转义字符（默认）
     └── -c    → 彩色输出（扩展功能）
     ↓
[步骤2] 确定从哪个参数开始是要输出的内容
     ↓
[步骤3] 逐个输出参数
     ├── 有 -e 选项 → 调用转义处理函数
     └── 无 -e 选项 → 直接原样输出
     ↓
[步骤4] 根据选项决定是否输出换行符
```

**转义字符处理原理**：

```
输入: "Hello\nWorld"

逐字符处理：
H → 输出 H
e → 输出 e
l → 输出 l
l → 输出 l
o → 输出 o
\ → 发现反斜杠，看下一个字符
n → 是 n，组合为 \n，输出换行符
W → 输出 W
...以此类推
```

**支持的转义字符**：

| 转义序列 | 含义 | 输出 |
|----------|------|------|
| `\\` | 反斜杠 | `\` |
| `\n` | 换行 | 换到下一行 |
| `\t` | 制表符 | 水平Tab |
| `\a` | 响铃 | 发出"嘟"声 |
| `\c` | 停止 | 立即停止输出 |
| `\0nnn` | 八进制 | 对应ASCII字符 |
| `\xHH` | 十六进制 | 对应ASCII字符 |

**核心代码解读**：

```c
// 转义字符处理函数
static void print_with_escapes(const char *str, int *stop_output) {
    for (int i = 0; str[i] != '\0'; i++) {
        if (str[i] != '\\') {
            putchar(str[i]);              // 普通字符，直接输出
            continue;
        }
        
        i++;                              // 跳到反斜杠后面
        switch (str[i]) {
            case 'n':  putchar('\n'); break;  // \n → 换行
            case 't':  putchar('\t'); break;  // \t → 制表符
            case '\\': putchar('\\'); break;  // \\ → 反斜杠
            case 'c':  *stop_output = 1; return;  // \c → 停止
            // ... 其他转义字符
        }
    }
}
```

**关键点**：
- 默认**不**解释转义字符（需要 `-e` 选项）
- `-n` 常用于脚本中，避免多余的换行
- `\c` 的特殊用途：用于进度条显示

---

### 8.9 `xcat` - 显示文件内容

**功能说明**：显示一个或多个文件的内容，支持行号显示等选项。

**通俗理解**：
把文件内容"倒"出来显示在屏幕上。`cat` 这个名字来自"concatenate"（连接），因为它可以把多个文件内容连接起来显示。

**实现流程**：

```
用户输入 xcat -n file1.txt file2.txt
     ↓
[步骤1] 解析选项
     ├── -n    → 显示行号
     ├── -A    → 显示不可见字符
     └── -T    → 显示制表符为 ^I
     ↓
[步骤2] 检查是否有文件参数
     ├── 无    → 从标准输入读取（用于管道）
     └── 有    → 处理每个文件
     ↓
[步骤3] 对每个文件调用 cat_file 函数
     │      （行号在多文件间连续）
     ↓
[步骤4] 返回结果
```

**单个文件处理原理**：

```
读取 file.txt:
┌─────────────────┐
│Hello World      │  ← 读取字符 H,e,l,l,o,...
│This is line 2   │
│End              │
└─────────────────┘

逐字符处理：
├── 在行首且开启-n → 输出 "     1  "
├── H → 输出 H
├── ... → 继续输出
├── \n → 输出换行，行号+1，标记"在行首"
├── 在行首且开启-n → 输出 "     2  "
└── 继续...
```

**核心代码解读**：

```c
static int cat_file(const char *filename, int show_line_numbers, ...) {
    FILE *file;
    int ch;
    
    // 特殊处理："-" 表示标准输入
    if (strcmp(filename, "-") == 0) {
        file = stdin;
    } else {
        file = fopen(filename, "r");      // 打开文件
        if (file == NULL) {
            perror(filename);
            return -1;
        }
    }
    
    // 逐字符读取并输出
    while ((ch = fgetc(file)) != EOF) {
        // 如果在行首且需要显示行号
        if (show_line_numbers && at_line_start) {
            printf("%6d  ", line_number);
            at_line_start = 0;
        }
        
        putchar(ch);                      // 输出字符
        
        if (ch == '\n') {                 // 遇到换行
            line_number++;
            at_line_start = 1;
        }
    }
    
    if (file != stdin) fclose(file);      // 关闭文件
}
```

**关键点**：
- 使用 `fgetc()` 逐字符读取，灵活处理各种选项
- 支持 `-` 表示标准输入，与管道配合使用
- 行号是连续的：多文件时不会重新从1开始

---

### 8.10 `xls` - 列出目录内容

**功能说明**：列出目录中的文件和子目录，支持多种显示格式。

**通俗理解**：
就像文件管理器的"详细信息"视图。可以看到文件名、大小、修改时间等信息。

**实现流程**：

```
用户输入 xls -la /home
     ↓
[步骤1] 解析选项
     ├── -l    → 长格式（详细信息）
     ├── -a    → 显示隐藏文件（以.开头的）
     ├── -h    → 人性化显示大小（KB, MB）
     ├── -R    → 递归显示子目录
     └── -S    → 按大小排序
     ↓
[步骤2] 确定目标目录
     ├── 无参数 → 当前目录
     └── 有参数 → 指定目录
     ↓
[步骤3] 打开目录并读取内容
     ↓
[步骤4] 对每个条目：
     ├── 检查是否需要显示（隐藏文件过滤）
     ├── 获取文件详细信息（-l 模式）
     └── 格式化输出
     ↓
[步骤5] 如果是 -R 模式，递归处理子目录
```

**长格式输出解析**：

```
-rw-r--r--  1 user group  1234 Dec 21 10:30 file.txt
│           │  │    │      │    │           │
│           │  │    │      │    │           └── 文件名
│           │  │    │      │    └── 修改时间
│           │  │    │      └── 文件大小（字节）
│           │  │    └── 所属组
│           │  └── 所有者
│           └── 链接数
└── 权限和文件类型
    第1位：d=目录, -=文件, l=链接
    2-4位：所有者权限 (rwx)
    5-7位：组权限 (rwx)
    8-10位：其他人权限 (rwx)
```

**核心代码解读**：

```c
int cmd_xls(Command *cmd, ShellContext *ctx) {
    DIR *dir;
    struct dirent *entry;
    struct stat st;
    
    dir = opendir(target_dir);            // 打开目录
    
    while ((entry = readdir(dir)) != NULL) {  // 遍历每一项
        // 过滤隐藏文件（如果没有 -a 选项）
        if (!show_all && entry->d_name[0] == '.') continue;
        
        if (show_long) {                  // 长格式模式
            stat(entry->d_name, &st);     // 获取详细信息
            
            // 输出权限
            print_permissions(st.st_mode);
            
            // 输出其他信息
            printf(" %3ld", st.st_nlink); // 链接数
            printf(" %-8s", get_username(st.st_uid));  // 用户名
            printf(" %-8s", get_groupname(st.st_gid)); // 组名
            printf(" %8ld", st.st_size);  // 大小
            printf(" %s", format_time(st.st_mtime));   // 时间
            printf(" %s\n", entry->d_name);            // 文件名
        } else {
            printf("%s  ", entry->d_name);  // 简单模式
        }
    }
    
    closedir(dir);
}
```

**关键点**：
- `opendir()` + `readdir()` 是遍历目录的标准方法
- `stat()` 获取文件的详细信息（大小、时间、权限等）
- 权限显示需要解析 `st_mode` 位掩码

---

## 八（续）、命令实现详解（第二部分：11-20）

本节继续介绍更多内置命令的实现过程。

---

### 8.11 `xcp` - 复制文件或目录

**功能说明**：复制文件或目录到指定位置，支持递归复制。

**通俗理解**：
就像 Windows 里的"复制-粘贴"。`xcp file.txt backup.txt` 把文件复制一份，`xcp -r folder backup_folder` 复制整个文件夹。

**实现流程**：

```
用户输入 xcp -r project project_backup
     ↓
[步骤1] 解析选项
     ├── -r/-R → 启用递归复制（用于目录）
     ↓
[步骤2] 确定目标是文件还是目录
     ├── 目标是目录 → 复制到该目录下
     └── 目标是文件 → 直接覆盖
     ↓
[步骤3] 判断源是文件还是目录
     ├── 是文件 → 调用 copy_file()
     └── 是目录 → 需要 -r 选项
              └── 调用 copy_directory_recursive()
```

**文件复制的原理**：

```
源文件 file.txt (1MB)
     ↓
打开源文件（只读模式）
     ↓
创建目标文件（写入模式）
     ↓
循环读取：
├── 读取 8KB 到缓冲区
├── 写入 8KB 到目标
├── 继续读取...
└── 直到读完
     ↓
关闭两个文件
```

**核心代码解读**：

```c
static int copy_file(const char *src, const char *dst) {
    char buffer[8192];                    // 8KB 缓冲区
    ssize_t nread;
    
    // 打开源文件
    int src_fd = open(src, O_RDONLY);
    
    // 获取源文件权限
    fstat(src_fd, &src_stat);
    
    // 创建目标文件（保持相同权限）
    int dst_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, src_stat.st_mode);
    
    // 循环复制
    while ((nread = read(src_fd, buffer, sizeof(buffer))) > 0) {
        write(dst_fd, buffer, nread);     // 写入目标
    }
    
    close(src_fd);
    close(dst_fd);
}
```

**关键点**：
- 使用缓冲区（8KB）提高复制效率
- 保留源文件的权限设置
- 递归复制需要遍历目录中的每个文件

---

### 8.12 `xmv` - 移动或重命名

**功能说明**：移动文件/目录到新位置，或重命名文件/目录。

**通俗理解**：
- `xmv old.txt new.txt` → 重命名文件
- `xmv file.txt /tmp/` → 移动文件到其他目录
- 相当于 Windows 的"剪切-粘贴"

**实现流程**：

```
用户输入 xmv old_name.txt new_name.txt
     ↓
[步骤1] 检查源文件是否存在
     ↓
[步骤2] 检查目标：
     ├── 目标是目录 → 构建目标路径：目录/源文件名
     └── 目标是文件 → 直接使用目标路径
     ↓
[步骤3] 调用 rename(源路径, 目标路径)
     ↓
[步骤4] 返回结果
```

**核心代码解读**：

```c
int cmd_xmv(Command *cmd, ShellContext *ctx) {
    const char *src = cmd->args[1];
    const char *dst = cmd->args[2];
    
    // 检查目标是否为目录
    if (stat(dst, &dst_stat) == 0 && S_ISDIR(dst_stat.st_mode)) {
        // 构建目标路径：目录/文件名
        const char *basename = strrchr(src, '/');
        basename = (basename == NULL) ? src : basename + 1;
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, basename);
    } else {
        strcpy(dst_path, dst);
    }
    
    // 执行移动/重命名
    if (rename(src, dst_path) != 0) {
        perror(src);
        return -1;
    }
    return 0;
}
```

**关键点**：
- `rename()` 系统调用完成实际的移动/重命名
- 同一文件系统内移动非常快（只修改目录项）
- 跨文件系统移动实际上是复制+删除

---

### 8.13 `xln` - 创建链接

**功能说明**：创建硬链接或符号链接（软链接）。

**通俗理解**：
- **硬链接**：像是同一个文件的另一个名字，删除原文件后仍可访问
- **符号链接**：像 Windows 的快捷方式，指向原文件的路径

**两种链接的区别**：

```
硬链接：                        符号链接：
┌──────────┐                    ┌──────────┐
│ file.txt │──┐                 │ link.txt │
└──────────┘  │                 └────┬─────┘
              ▼                      │
┌──────────┐  ┌─────────┐           │ 保存路径
│ hard.txt │──┤ 文件数据 │           ▼ "/path/to/file.txt"
└──────────┘  └─────────┘       ┌──────────┐
                                 │ file.txt │
  两个名字指向同一份数据         └──────────┘
```

**实现流程**：

```
用户输入 xln -s target.txt link.txt
     ↓
[步骤1] 检查是否有 -s 选项
     ├── 有 -s → 符号链接模式
     └── 无 -s → 硬链接模式
     ↓
[步骤2] 获取源文件和目标名称
     ↓
[步骤3] 创建链接
     ├── 符号链接 → symlink(源, 目标)
     └── 硬链接   → link(源, 目标)
```

**核心代码解读**：

```c
int cmd_xln(Command *cmd, ShellContext *ctx) {
    int symbolic = 0;
    
    // 检查 -s 选项
    if (strcmp(cmd->args[1], "-s") == 0) {
        symbolic = 1;
    }
    
    const char *src = cmd->args[symbolic ? 2 : 1];
    const char *dst = cmd->args[symbolic ? 3 : 2];
    
    if (symbolic) {
        symlink(src, dst);      // 创建符号链接
    } else {
        link(src, dst);         // 创建硬链接
    }
}
```

**关键点**：
- `link()` 创建硬链接，增加文件的链接计数
- `symlink()` 创建符号链接，存储目标路径字符串
- 硬链接不能跨文件系统，不能链接目录

---

### 8.14 `xstat` - 显示文件详细信息

**功能说明**：显示文件的详细统计信息（大小、权限、时间等）。

**通俗理解**：
查看文件的"身份证"——大小、创建时间、修改时间、权限、所有者等详细信息。

**实现流程**：

```
用户输入 xstat file.txt
     ↓
[步骤1] 调用 stat() 获取文件信息
     ↓
[步骤2] 解析 struct stat 结构体
     ├── st_size    → 文件大小
     ├── st_mode    → 权限和类型
     ├── st_uid     → 所有者ID
     ├── st_gid     → 组ID
     ├── st_nlink   → 硬链接数
     ├── st_ino     → inode号
     ├── st_atime   → 访问时间
     ├── st_mtime   → 修改时间
     └── st_ctime   → 状态改变时间
     ↓
[步骤3] 格式化输出
```

**输出示例**：

```
  文件: test.txt
  大小: 1234         块: 8          IO块: 4096   设备: 8/1
  Inode: 12345678    硬链接: 1
  权限: (0644/rw-r--r--)  Uid: (1000/user)  Gid: (1000/user)
  最近访问: 2024-12-21 10:30:00
  最近修改: 2024-12-21 09:15:30
  最近更改: 2024-12-21 09:15:30
```

**核心代码解读**：

```c
int show_file_stat(const char *filename) {
    struct stat st;
    
    // 获取文件状态
    if (stat(filename, &st) != 0) {
        perror(filename);
        return -1;
    }
    
    // 格式化权限字符串
    char perm[10];
    perm[0] = (st.st_mode & S_IRUSR) ? 'r' : '-';
    perm[1] = (st.st_mode & S_IWUSR) ? 'w' : '-';
    perm[2] = (st.st_mode & S_IXUSR) ? 'x' : '-';
    // ... 组权限和其他人权限
    
    // 获取用户名和组名
    struct passwd *pwd = getpwuid(st.st_uid);
    struct group *grp = getgrgid(st.st_gid);
    
    // 格式化时间
    char mtime[64];
    strftime(mtime, sizeof(mtime), "%Y-%m-%d %H:%M:%S", 
             localtime(&st.st_mtime));
    
    // 输出信息
    printf("  文件: %s\n", filename);
    printf("  大小: %ld\n", st.st_size);
    // ...
}
```

**关键点**：
- `stat()` 是获取文件信息的核心系统调用
- 权限需要通过位掩码解析
- 用户ID需要转换为用户名（通过 `getpwuid()`）

---

### 8.15 `xfind` - 查找文件

**功能说明**：在目录树中递归查找匹配特定模式的文件。

**通俗理解**：
就像 Windows 的搜索功能。`xfind . -name "*.txt"` 在当前目录及所有子目录中查找所有 .txt 文件。

**实现流程**：

```
用户输入 xfind /home -name "*.c"
     ↓
[步骤1] 解析参数
     ├── 搜索路径 = /home
     └── 匹配模式 = *.c
     ↓
[步骤2] 递归遍历目录
     ↓ 对目录中每个条目：
[步骤3] 检查文件名是否匹配模式
     ├── 匹配 → 打印路径
     ↓
[步骤4] 如果是子目录，递归进入
     ↓
[步骤5] 继续直到遍历完成
```

**通配符匹配原理**：

```
模式 "*.c" 匹配：
  main.c     ✓（以 .c 结尾）
  test.c     ✓
  file.txt   ✗（不以 .c 结尾）
  a.c.bak    ✗

模式 "test*" 匹配：
  test.c     ✓（以 test 开头）
  test123    ✓
  my_test    ✗（不以 test 开头）
```

**核心代码解读**：

```c
static void find_files(const char *path, const char *pattern) {
    DIR *dir = opendir(path);
    struct dirent *entry;
    
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (strcmp(entry->d_name, ".") == 0 || 
            strcmp(entry->d_name, "..") == 0) continue;
        
        // 构建完整路径
        char full_path[PATH_MAX];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
        
        // 检查文件名是否匹配模式
        if (fnmatch(pattern, entry->d_name, 0) == 0) {
            printf("%s\n", full_path);    // 匹配，输出路径
        }
        
        // 如果是目录，递归
        struct stat st;
        lstat(full_path, &st);
        if (S_ISDIR(st.st_mode)) {
            find_files(full_path, pattern);  // 递归进入子目录
        }
    }
    closedir(dir);
}
```

**关键点**：
- `fnmatch()` 函数用于通配符匹配
- 递归遍历实现目录树搜索
- 注意跳过 `.` 和 `..` 避免无限循环

---

### 8.16 `xgrep` - 文本搜索

**功能说明**：在文件中搜索包含指定文本的行。

**通俗理解**：
像 Ctrl+F 搜索功能的命令行版本。`xgrep "error" log.txt` 找出日志文件中所有包含"error"的行。

**实现流程**：

```
用户输入 xgrep -in "error" log.txt
     ↓
[步骤1] 解析选项
     ├── -i → 忽略大小写
     ├── -n → 显示行号
     ├── -v → 反向匹配（不包含的行）
     ├── -c → 只显示数量
     └── -w → 整词匹配
     ↓
[步骤2] 逐行读取文件
     ↓
[步骤3] 对每行检查是否匹配
     ├── 普通匹配 → strstr(行, 模式)
     └── 忽略大小写 → stristr(行, 模式)
     ↓
[步骤4] 根据选项输出结果
```

**核心代码解读**：

```c
static int grep_file(const char *filename, const char *pattern, 
                     GrepOptions *opts) {
    FILE *file = fopen(filename, "r");
    char line[4096];
    int line_num = 0;
    int match_count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        line_num++;
        
        // 检查是否匹配
        int is_match;
        if (opts->ignore_case) {
            is_match = (stristr(line, pattern) != NULL);
        } else {
            is_match = (strstr(line, pattern) != NULL);
        }
        
        // 反向匹配
        if (opts->invert_match) {
            is_match = !is_match;
        }
        
        if (is_match) {
            match_count++;
            if (!opts->count_only) {
                if (opts->show_line_num) printf("%d:", line_num);
                printf("%s", line);
            }
        }
    }
    
    if (opts->count_only) {
        printf("%d\n", match_count);
    }
    
    fclose(file);
}
```

**关键点**：
- `strstr()` 用于字符串搜索
- 忽略大小写需要自己实现 `stristr()`
- 支持从标准输入读取（配合管道使用）

---

### 8.17 `xhead` - 显示文件开头

**功能说明**：显示文件的前 N 行（默认10行）。

**通俗理解**：
快速预览文件内容。`xhead -n 5 file.txt` 只看前5行，不用打开整个大文件。

**实现流程**：

```
用户输入 xhead -n 20 file.txt
     ↓
[步骤1] 解析 -n 选项确定行数（默认10）
     ↓
[步骤2] 打开文件
     ↓
[步骤3] 循环读取并输出
     ├── line_count < N → 读取一行，输出，line_count++
     └── line_count >= N → 停止
     ↓
[步骤4] 关闭文件
```

**核心代码解读**：

```c
static int head_file(const char *filename, int num_lines) {
    FILE *file = fopen(filename, "r");
    char line[4096];
    int line_count = 0;
    
    // 读取并显示前 N 行
    while (line_count < num_lines && fgets(line, sizeof(line), file)) {
        printf("%s", line);
        line_count++;
    }
    
    fclose(file);
    return 0;
}
```

**关键点**：
- 实现简单：读够指定行数就停止
- 多文件时显示文件名标题
- 支持从管道读取

---

### 8.18 `xtail` - 显示文件结尾

**功能说明**：显示文件的后 N 行（默认10行）。

**通俗理解**：
查看日志文件的最新内容。`xtail -n 20 log.txt` 看日志文件的最后20行。

**实现难点**：
不像 `head` 可以读到够了就停，`tail` 需要读完整个文件才知道最后几行是什么。

**解决方案：循环缓冲区**

```
假设要显示最后 3 行，文件有 5 行：

读取过程（使用3个槽位的循环缓冲区）：
┌─────┬─────┬─────┐
│  ①  │     │     │  读入第1行
├─────┼─────┼─────┤
│  ①  │  ②  │     │  读入第2行
├─────┼─────┼─────┤
│  ①  │  ②  │  ③  │  读入第3行（满了）
├─────┼─────┼─────┤
│  ④  │  ②  │  ③  │  读入第4行（覆盖最老的）
├─────┼─────┼─────┤
│  ④  │  ⑤  │  ③  │  读入第5行
└─────┴─────┴─────┘

输出顺序：③ → ④ → ⑤（最后3行）
```

**核心代码解读**：

```c
// 循环缓冲区
typedef struct {
    char **lines;       // 行数组
    int capacity;       // 容量（N行）
    int count;          // 当前存储的行数
    int start;          // 起始位置
} CircularBuffer;

// 添加行到缓冲区
static void add_line(CircularBuffer *buf, const char *line) {
    int index = (buf->start + buf->count) % buf->capacity;
    strcpy(buf->lines[index], line);
    
    if (buf->count < buf->capacity) {
        buf->count++;
    } else {
        buf->start = (buf->start + 1) % buf->capacity;  // 覆盖最老的
    }
}
```

**关键点**：
- 循环缓冲区是经典数据结构，固定内存存储"最近N项"
- 时间复杂度 O(n)，空间复杂度 O(N)

---

### 8.19 `xwc` - 统计行数/字数/字节数

**功能说明**：统计文件的行数、单词数和字节数。

**通俗理解**：
像 Word 文档底部的字数统计功能。

**实现流程**：

```
用户输入 xwc -l file.txt
     ↓
[步骤1] 解析选项
     ├── -l → 只显示行数
     ├── -w → 只显示字数
     └── -c → 只显示字节数
     ↓
[步骤2] 逐字符读取文件
     ↓
[步骤3] 统计：
     ├── bytes++（每个字符）
     ├── 遇到 '\n' → lines++
     └── 从空白变为非空白 → words++
     ↓
[步骤4] 输出统计结果
```

**单词计数原理**：

```
输入: "Hello World  Test"

字符: H e l l o   W o r l d     T e s t
状态: 词 词 词 词 词 空 词 词 词 词 词 空 空 词 词 词 词

单词边界（从空白→非空白的转换次数）= 3
```

**核心代码解读**：

```c
static int wc_file(const char *filename, WcStats *stats) {
    FILE *file = fopen(filename, "r");
    int ch;
    int in_word = 0;      // 是否在单词内
    
    stats->lines = 0;
    stats->words = 0;
    stats->bytes = 0;
    
    while ((ch = fgetc(file)) != EOF) {
        stats->bytes++;
        
        if (ch == '\n') {
            stats->lines++;
        }
        
        // 统计单词
        if (isspace(ch)) {
            in_word = 0;
        } else {
            if (!in_word) {
                stats->words++;   // 进入新单词
                in_word = 1;
            }
        }
    }
    
    fclose(file);
}
```

**关键点**：
- 用状态机思想统计单词
- `isspace()` 判断是否为空白字符
- 多文件时显示总计

---

### 8.20 `xsort` - 排序文件内容

**功能说明**：对文件的行进行排序。

**通俗理解**：
像 Excel 的排序功能，可以按字母顺序或数字大小排序。

**实现流程**：

```
用户输入 xsort -rn numbers.txt
     ↓
[步骤1] 解析选项
     ├── -r → 逆序排序
     ├── -n → 数值排序
     └── -u → 去除重复
     ↓
[步骤2] 读取所有行到内存
     ↓
[步骤3] 调用 qsort() 排序
     ├── 字符串排序 → strcmp
     └── 数值排序   → 转换为数字后比较
     ↓
[步骤4] 输出排序后的行
     └── 如有 -u 选项，跳过重复行
```

**排序示例**：

```
原始文件:        字母排序:        数值排序:
banana          apple           1
1               banana          2
apple           cherry          10
10              1               100
2               10
cherry          100
100             2
```

**核心代码解读**：

```c
// 字符串比较函数
static int str_compare(const void *a, const void *b) {
    return strcmp(*(const char**)a, *(const char**)b);
}

// 数值比较函数
static int num_compare(const void *a, const void *b) {
    double num_a = atof(*(const char**)a);
    double num_b = atof(*(const char**)b);
    if (num_a < num_b) return -1;
    if (num_a > num_b) return 1;
    return 0;
}

static int sort_file(const char *filename, SortOptions *opts) {
    // 读取所有行
    char *lines[MAX_LINES];
    int count = 0;
    
    while (fgets(line, sizeof(line), file)) {
        lines[count++] = strdup(line);
    }
    
    // 选择比较函数
    int (*cmp)(const void*, const void*);
    if (opts->numeric) {
        cmp = opts->reverse ? num_compare_reverse : num_compare;
    } else {
        cmp = opts->reverse ? str_compare_reverse : str_compare;
    }
    
    // 排序
    qsort(lines, count, sizeof(char*), cmp);
    
    // 输出（去重处理）
    for (int i = 0; i < count; i++) {
        if (opts->unique && i > 0 && strcmp(lines[i], lines[i-1]) == 0) {
            continue;   // 跳过重复行
        }
        printf("%s", lines[i]);
    }
}
```

**关键点**：
- 使用 C 库的 `qsort()` 进行快速排序
- 通过函数指针实现不同排序方式
- 去重需要先排序，再跳过相邻重复项

---

## 八（续）、命令实现详解（第三部分：21-30）

本节继续介绍文本处理、权限管理、进程控制等命令的实现。

---

### 8.21 `xuniq` - 去除重复行

**功能说明**：去除文件中**相邻**的重复行。

**通俗理解**：
像去除通讯录中连续重复的联系人。注意：只能去除相邻的重复行，不相邻的重复行不会被去除。

**关键限制**：

```
输入:          xuniq输出:    先排序再xuniq:
aaa            aaa          aaa
aaa            bbb          bbb
bbb            aaa  ← 不相邻，保留
aaa

结论：要完全去重，需要先排序 → xsort file | xuniq
```

**实现流程**：

```
用户输入 xuniq -c file.txt
     ↓
[步骤1] 解析选项
     ├── -c → 显示每行重复次数
     ├── -d → 只显示重复的行
     └── -u → 只显示不重复的行
     ↓
[步骤2] 读取第一行作为"上一行"
     ↓
[步骤3] 逐行读取，与上一行比较
     ├── 相同 → 计数+1，继续
     └── 不同 → 输出上一行，更新上一行
     ↓
[步骤4] 输出最后一行
```

**核心代码解读**：

```c
static int uniq_file(const char *filename, UniqOptions *opts) {
    char prev_line[4096] = {0};
    char curr_line[4096];
    int line_count = 0;
    int is_first = 1;
    
    while (fgets(curr_line, sizeof(curr_line), file)) {
        if (is_first) {
            strcpy(prev_line, curr_line);
            line_count = 1;
            is_first = 0;
        } else if (strcmp(curr_line, prev_line) == 0) {
            line_count++;         // 相同行，计数增加
        } else {
            // 不同行，输出前一行
            if (opts->count) {
                printf("%7d %s", line_count, prev_line);
            } else {
                printf("%s", prev_line);
            }
            strcpy(prev_line, curr_line);
            line_count = 1;
        }
    }
    // 输出最后一行...
}
```

**关键点**：
- 只比较相邻的行
- 配合 `xsort` 使用才能实现完全去重
- `-c` 选项常用于统计词频

---

### 8.22 `xcut` - 提取列

**功能说明**：从文件中提取指定的列（字段）。

**通俗理解**：
像 Excel 中选择并复制某几列。`xcut -d: -f1 /etc/passwd` 提取用户名（以冒号分隔的第1列）。

**实现流程**：

```
用户输入 xcut -d: -f1,3 /etc/passwd
     ↓
[步骤1] 解析选项
     ├── -d: → 分隔符为冒号
     └── -f1,3 → 提取第1和第3个字段
     ↓
[步骤2] 解析字段规格 "1,3" → [1, 3]
     ↓
[步骤3] 逐行读取文件
     ↓
[步骤4] 按分隔符分割每行
     │      root:x:0:0:root:/root:/bin/bash
     │        ↓ 分割为
     │      [root, x, 0, 0, root, /root, /bin/bash]
     ↓
[步骤5] 输出指定字段
     │      root:0  (第1和第3个)
```

**核心代码解读**：

```c
// 解析字段规格（如 "1,3" 或 "1-3"）
static int parse_field_spec(const char *spec, int *fields, int max) {
    // 支持逗号分隔: 1,2,3
    // 支持范围: 1-3
    // 返回字段数量
}

// 处理每一行
while (fgets(line, sizeof(line), file)) {
    char *token = line;
    int field_num = 1;
    
    // 按分隔符分割
    for (size_t i = 0; i <= len; i++) {
        if (line[i] == delimiter || line[i] == '\0') {
            // 检查是否需要输出此字段
            for (int j = 0; j < field_count; j++) {
                if (fields[j] == field_num) {
                    // 输出这个字段
                }
            }
            token = line + i + 1;
            field_num++;
        }
    }
}
```

**关键点**：
- 默认分隔符是制表符
- 字段编号从1开始
- 支持范围表达式（如 `1-5`）

---

### 8.23 `xtr` - 字符转换

**功能说明**：转换或删除字符。

**通俗理解**：
批量替换字符。`xtr 'a-z' 'A-Z'` 把所有小写字母转换为大写。

**实现流程**：

```
用户输入 xtr 'a-z' 'A-Z' < file.txt
     ↓
[步骤1] 解析字符集
     ├── 'a-z' → a,b,c,...,z（26个字母）
     └── 'A-Z' → A,B,C,...,Z
     ↓
[步骤2] 逐字符读取输入
     ↓
[步骤3] 对每个字符：
     ├── 在字符集1中 → 转换为字符集2对应位置
     └── 不在字符集1中 → 原样输出
```

**删除模式示例**：

```
xtr -d '0-9' < file.txt

输入: "Hello123World456"
输出: "HelloWorld"

原理：删除所有在 '0-9' 范围内的字符
```

**核心代码解读**：

```c
// 转换字符
static char translate_char(char c, const char *from, const char *to) {
    // 简化实现：支持 "a-z" -> "A-Z"
    if (from_len == 3 && from[1] == '-' && 
        to_len == 3 && to[1] == '-') {
        
        if (c >= from[0] && c <= from[2]) {
            // 计算偏移量并映射
            int offset = c - from[0];
            return to[0] + offset;
        }
    }
    return c;
}
```

**关键点**：
- 从标准输入读取
- 支持范围表示法（`a-z`）
- `-d` 选项用于删除字符

---

### 8.24 `xdiff` - 比较文件差异

**功能说明**：比较两个文件，显示差异。

**通俗理解**：
就像 Word 的"修订"功能，显示两个版本之间的差异。

**实现流程**：

```
用户输入 xdiff file1.txt file2.txt
     ↓
[步骤1] 读取两个文件的所有行到内存
     ↓
[步骤2] 逐行比较
     ├── 行相同 → 跳过
     └── 行不同 → 记录差异
     ↓
[步骤3] 输出差异
     ├── - 开头：file1 有，file2 没有
     └── + 开头：file2 有，file1 没有
```

**输出格式**：

```
--- old.txt
+++ new.txt
-3: 这行被删除了
+3: 这行是新增的
-5: 原来的内容
+5: 修改后的内容
```

**核心代码解读**：

```c
static void print_simple_diff(char **lines1, int count1, 
                               char **lines2, int count2) {
    int i = 0, j = 0;
    
    while (i < count1 || j < count2) {
        if (i >= count1) {
            printf("+%d: %s", j+1, lines2[j++]);  // file2多出的行
        } else if (j >= count2) {
            printf("-%d: %s", i+1, lines1[i++]);  // file1多出的行
        } else {
            if (strcmp(lines1[i], lines2[j]) == 0) {
                i++; j++;                         // 相同，跳过
            } else {
                printf("-%d: %s", i+1, lines1[i++]);
                printf("+%d: %s", j+1, lines2[j++]);
            }
        }
    }
}
```

**关键点**：
- 简化实现：逐行顺序比较
- 真正的 diff 使用 LCS（最长公共子序列）算法
- `-u` 选项输出统一格式

---

### 8.25 `xchmod` - 修改文件权限

**功能说明**：修改文件的访问权限。

**通俗理解**：
设置谁可以读、写、执行文件。像设置房门的钥匙权限。

**权限表示法**：

```
八进制模式:              符号模式:
  7 5 5                    u+x（用户加执行权限）
  ↓ ↓ ↓                    g-w（组去掉写权限）
  用户 组 其他              a=r（所有人设为只读）

数字含义:
  4 = 读(r)
  2 = 写(w)
  1 = 执行(x)

755 = rwxr-xr-x = 4+2+1, 4+1, 4+1
644 = rw-r--r-- = 4+2, 4, 4
```

**实现流程**：

```
用户输入 xchmod 755 script.sh
     ↓
[步骤1] 检测权限格式
     ├── 全为数字 → 八进制模式
     └── 包含字母 → 符号模式
     ↓
[步骤2] 解析权限值
     ├── 八进制：strtol("755", 8) → 0755
     └── 符号：解析 u/g/o/a + +/-/= + r/w/x
     ↓
[步骤3] 调用 chmod(文件名, 权限值)
```

**核心代码解读**：

```c
// 解析八进制模式
static int parse_octal_mode(const char *str, mode_t *mode) {
    long val = strtol(str, NULL, 8);   // 基数8（八进制）
    *mode = (mode_t)val;
    return 0;
}

// 解析符号模式
static int parse_symbolic_mode(const char *str, mode_t current, mode_t *new) {
    // 解析 "u+x" -> 
    // who='u', op='+', perm='x'
    // 返回新权限
}

// 执行权限修改
chmod(filename, mode);
```

**关键点**：
- 支持两种权限表示法
- `chmod()` 系统调用执行实际修改
- 符号模式需要先获取当前权限

---

### 8.26 `xps` - 显示进程信息

**功能说明**：显示系统中运行的进程列表。

**通俗理解**：
像 Windows 任务管理器，显示正在运行的程序及其资源使用情况。

**实现原理**：

```
Linux 的 /proc 文件系统：
/proc/
├── 1/                    # PID为1的进程目录
│   ├── stat              # 进程状态信息
│   ├── status            # 详细状态（UID等）
│   └── cmdline           # 命令行参数
├── 1234/                 # 另一个进程
│   ├── stat
│   └── ...
└── ...

读取 /proc/1234/stat:
"1234 (bash) S 1000 ..."
 PID  命令   状态 PPID
```

**实现流程**：

```
用户输入 xps -a
     ↓
[步骤1] 遍历 /proc 目录
     └── 只处理数字命名的目录（进程目录）
     ↓
[步骤2] 对每个进程读取信息
     ├── /proc/[pid]/stat    → PID、状态、PPID、内存
     ├── /proc/[pid]/status  → UID（用户）
     └── /proc/[pid]/cmdline → 命令名
     ↓
[步骤3] 格式化输出为表格
```

**进程状态说明**：

| 状态 | 含义 |
|-----|------|
| R | 运行中（Running） |
| S | 睡眠（Sleeping） |
| D | 不可中断睡眠（等待I/O） |
| Z | 僵尸（已终止但未被回收） |
| T | 停止（被暂停） |

**核心代码解读**：

```c
static int read_process_info(int pid, ProcessInfo *info) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/stat", pid);
    
    FILE *fp = fopen(path, "r");
    fscanf(fp, "%d (%255[^)]) %c %d",
           &info->pid, info->cmd, &info->state, &info->ppid);
    fclose(fp);
    
    // 读取用户ID
    snprintf(path, sizeof(path), "/proc/%d/status", pid);
    // 解析 "Uid: 1000 ..."
}
```

**关键点**：
- 通过 `/proc` 虚拟文件系统获取进程信息
- `/proc/[pid]/stat` 包含进程的基本状态
- 使用表格格式美化输出

---

### 8.27 `xkill` - 终止进程

**功能说明**：向进程发送信号，通常用于终止进程。

**通俗理解**：
像任务管理器的"结束任务"。可以"请求"进程退出，也可以"强制"终止。

**常用信号**：

| 信号 | 编号 | 作用 |
|-----|------|------|
| SIGTERM | 15 | 请求终止（可被捕获） |
| SIGKILL | 9 | 强制终止（无法忽略） |
| SIGINT | 2 | 中断（Ctrl+C） |
| SIGSTOP | 19 | 暂停进程 |
| SIGCONT | 18 | 继续运行 |

**实现流程**：

```
用户输入 xkill -s SIGKILL 1234
     ↓
[步骤1] 解析选项和参数
     ├── -s SIGKILL → 信号类型
     └── 1234 → 进程ID
     ↓
[步骤2] 将信号名转换为编号
     │      "SIGKILL" → 9
     ↓
[步骤3] 调用 kill(pid, signal)
```

**核心代码解读**：

```c
// 信号名转编号
static int signal_name_to_num(const char *name) {
    if (strcasecmp(name, "TERM") == 0) return SIGTERM;
    if (strcasecmp(name, "KILL") == 0) return SIGKILL;
    if (strcasecmp(name, "INT") == 0) return SIGINT;
    return -1;
}

int cmd_xkill(Command *cmd, ShellContext *ctx) {
    pid_t pid = atoi(cmd->args[1]);
    int signal = SIGTERM;       // 默认信号
    
    // 解析 -s 选项...
    
    if (kill(pid, signal) != 0) {
        perror("xkill");
        return -1;
    }
    
    printf("已向进程 %d 发送信号 %d\n", pid, signal);
    return 0;
}
```

**关键点**：
- `kill()` 系统调用发送信号
- SIGTERM 可以被进程捕获并做清理
- SIGKILL 是"最终手段"，进程无法拒绝

---

### 8.28 `xenv` - 显示环境变量

**功能说明**：显示当前 Shell 的所有环境变量。

**通俗理解**：
查看系统的"全局设置"，如搜索路径、主目录等。

**常见环境变量**：

| 变量 | 作用 |
|-----|------|
| PATH | 命令搜索路径 |
| HOME | 用户主目录 |
| USER | 当前用户名 |
| SHELL | 默认 Shell |
| PWD | 当前目录 |
| LANG | 语言设置 |

**实现原理**：

```c
// C 语言中，environ 是一个全局变量
// 它指向一个字符串数组，每个字符串格式为 "NAME=value"
extern char **environ;

// environ 的内存布局：
// environ → ["PATH=/usr/bin", "HOME=/home/user", "USER=john", NULL]

int cmd_xenv(Command *cmd, ShellContext *ctx) {
    // 遍历并打印所有环境变量
    for (char **env = environ; *env != NULL; env++) {
        printf("%s\n", *env);
    }
    return 0;
}
```

**关键点**：
- `environ` 是 C 运行时提供的全局变量
- 每个环境变量是 "NAME=value" 格式的字符串
- 配合 `xgrep` 可以搜索特定变量

---

### 8.29 `xexport` - 设置环境变量

**功能说明**：设置或导出环境变量。

**通俗理解**：
修改系统的"全局设置"。设置的变量可以被子进程继承。

**实现流程**：

```
用户输入 xexport MY_VAR=hello
     ↓
[步骤1] 解析参数
     ├── 找到等号 → 提取变量名和值
     │      MY_VAR = hello
     └── 无等号 → 显示该变量
     ↓
[步骤2] 验证变量名
     ├── 只允许字母、数字、下划线
     └── 不能以数字开头
     ↓
[步骤3] 调用 setenv(name, value, 1)
     │      参数 1 表示覆盖已存在的变量
```

**核心代码解读**：

```c
int cmd_xexport(Command *cmd, ShellContext *ctx) {
    const char *arg = cmd->args[1];   // "MY_VAR=hello"
    
    // 找等号
    char *eq = strchr(arg, '=');
    
    if (eq) {
        // 分离变量名和值
        size_t name_len = eq - arg;
        char name[256];
        strncpy(name, arg, name_len);
        name[name_len] = '\0';
        
        const char *value = eq + 1;   // "hello"
        
        // 设置环境变量
        if (setenv(name, value, 1) != 0) {
            perror("setenv");
            return -1;
        }
    } else {
        // 显示变量值
        char *value = getenv(arg);
        if (value) printf("%s=%s\n", arg, value);
    }
}
```

**关键点**：
- `setenv()` 系统函数设置环境变量
- 变量只在当前 Shell 及其子进程有效
- Shell 退出后设置会丢失

---

### 8.30 `xalias` - 命令别名

**功能说明**：为命令创建简短的别名。

**通俗理解**：
给常用的长命令起个"绰号"。比如把 `xls -lah` 命名为 `ll`，以后输入 `ll` 就等于输入 `xls -lah`。

**实现流程**：

```
用户输入 xalias ll='xls -lah'
     ↓
[步骤1] 解析参数
     ├── 找到等号 → 设置别名
     └── 无等号 → 显示该别名
     ↓
[步骤2] 提取别名名称和值
     │      ll = xls -lah
     ↓
[步骤3] 去除值两端的引号
     ↓
[步骤4] 存入别名表
     │      aliases[count] = {name: "ll", value: "xls -lah"}
```

**别名系统的工作原理**：

```
用户输入: ll Documents

Shell 处理流程:
1. 检查 "ll" 是否是别名 → 是
2. 获取别名值: "xls -lah"
3. 替换命令: "xls -lah Documents"
4. 执行实际命令
```

**核心代码解读**：

```c
// 别名数据结构
typedef struct {
    char name[64];
    char value[256];
} Alias;

static Alias aliases[MAX_ALIASES];
static int alias_count = 0;

// 设置别名
int alias_set(const char *name, const char *value) {
    // 查找是否已存在
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            strcpy(aliases[i].value, value);  // 更新
            return 0;
        }
    }
    // 新增
    strcpy(aliases[alias_count].name, name);
    strcpy(aliases[alias_count].value, value);
    alias_count++;
    return 0;
}
```

**关键点**：
- 别名存储在内存数组中
- 命令执行前会检查是否有对应别名
- 别名不能递归展开（防止无限循环）

---

## 八（续）、命令实现详解（第四部分：31-40）

本节介绍作业控制、历史记录、脚本执行、系统工具等命令的实现。

---

### 8.31 `xjobs` - 显示后台任务

**功能说明**：显示当前 Shell 的所有后台任务。

**通俗理解**：
查看"后台运行"的程序列表。当你用 `命令 &` 在后台执行任务时，可以用 `xjobs` 查看它们。

**后台任务的概念**：

```
前台任务：占据终端，你必须等它完成
$ sleep 10          ← 终端被占用10秒

后台任务：不占据终端，你可以继续工作
$ sleep 10 &        ← 立即返回提示符
[1] 12345           ← 任务编号和进程ID
```

**核心代码解读**：

```c
// 作业数据结构
typedef struct Job {
    int id;              // 作业编号 [1], [2]...
    pid_t pid;           // 进程ID
    char command[256];   // 命令字符串
    JobStatus status;    // Running, Stopped, Done
} Job;

int cmd_xjobs(Command *cmd, ShellContext *ctx) {
    job_print_all();     // 打印所有作业
    return 0;
}

// 输出格式：
// [1]+  Running                 sleep 100 &
// [2]-  Stopped                 vim file.txt
```

**关键点**：
- 作业表存储在 Shell 内存中
- 后台执行用 `&` 结尾
- `xfg` 可以将后台任务调到前台

---

### 8.32 `xfg` - 前台运行

**功能说明**：将后台任务调到前台执行。

**通俗理解**：
把"后台播放的音乐"调到"主屏幕"。如果任务被暂停了（Ctrl+Z），可以用 `xfg` 恢复并在前台运行。

**实现流程**：

```
用户输入 xfg 1
     ↓
[步骤1] 获取作业ID（默认最后一个）
     ↓
[步骤2] 查找对应的 Job 结构
     ↓
[步骤3] 如果是 Stopped 状态
     │      发送 SIGCONT 信号恢复
     ↓
[步骤4] 等待进程完成
     │      waitpid(pid, &status, WUNTRACED)
     ↓
[步骤5] 根据结果处理
     ├── 正常退出 → 从作业表移除
     └── 再次停止 → 更新状态为 Stopped
```

**核心代码解读**：

```c
int cmd_xfg(Command *cmd, ShellContext *ctx) {
    int job_id = atoi(cmd->args[1]);
    Job *job = job_get(job_id);
    
    printf("%s\n", job->command);
    
    // 如果是停止状态，发送 SIGCONT 信号恢复
    if (job->status == JOB_STOPPED) {
        kill(job->pid, SIGCONT);
        job->status = JOB_RUNNING;
    }
    
    // 等待进程完成（WUNTRACED 允许检测停止信号）
    int status;
    waitpid(job->pid, &status, WUNTRACED);
    
    if (WIFSTOPPED(status)) {
        job->status = JOB_STOPPED;
        printf("\n[%d]+  Stopped   %s\n", job->id, job->command);
    } else {
        job_remove(job_id);  // 进程结束，移除作业
    }
}
```

**关键点**：
- `SIGCONT` 信号使停止的进程继续运行
- `waitpid()` 的 `WUNTRACED` 标志检测进程停止
- Ctrl+Z 会发送 `SIGTSTP` 使进程停止

---

### 8.33 `xbg` - 后台运行

**功能说明**：让暂停的任务在后台继续运行。

**通俗理解**：
程序用 Ctrl+Z 暂停后，`xbg` 让它在后台"悄悄地"继续运行。

**与 `xfg` 的区别**：

| 命令 | 作用 |
|-----|------|
| `xfg` | 在前台运行，占据终端 |
| `xbg` | 在后台运行，返回提示符 |

**核心代码解读**：

```c
int cmd_xbg(Command *cmd, ShellContext *ctx) {
    int job_id = ...;  // 获取作业ID
    Job *job = job_get(job_id);
    
    if (job->status != JOB_STOPPED) {
        printf("xbg: 任务 %d 不是停止状态\n", job_id);
        return -1;
    }
    
    // 发送 SIGCONT 继续执行
    kill(job->pid, SIGCONT);
    job->status = JOB_RUNNING;
    
    printf("[%d]+ %s &\n", job->id, job->command);
    
    return 0;
    // 注意：不调用 waitpid()，所以不会等待进程
}
```

**关键点**：
- 只发送 `SIGCONT` 信号
- 不调用 `waitpid()` 所以不会阻塞
- 进程在后台继续运行

---

### 8.34 `xhistory` - 历史记录

**功能说明**：显示命令历史列表。

**通俗理解**：
像浏览器的"历史记录"，显示之前执行过的所有命令。

**历史记录系统**：

```
┌─────────────────────────────────────┐
│          命令历史管理               │
├─────────────────────────────────────┤
│ 内存中的历史数组                     │
│   [0] xpwd                         │
│   [1] xls -la                      │
│   [2] xcd /tmp                     │
│   ...                              │
├─────────────────────────────────────┤
│ 持久化存储：.xshell_history         │
│ • 启动时加载                        │
│ • 退出时保存                        │
│ • 最多1000条                        │
└─────────────────────────────────────┘
```

**核心代码解读**：

```c
// 历史记录数据结构（在 history.c 中）
static char *history[HISTORY_MAX];   // 字符串数组
static int history_count = 0;        // 当前记录数

// 显示历史
void history_show() {
    for (int i = 0; i < history_count; i++) {
        printf("%5d  %s\n", i + 1, history[i]);
    }
}

// 添加历史
void history_add(const char *command) {
    // 跳过重复的连续命令
    if (history_count > 0 && 
        strcmp(history[history_count-1], command) == 0) {
        return;
    }
    
    history[history_count++] = strdup(command);
}
```

**关键点**：
- 历史记录存储在内存数组中
- 自动过滤连续重复的命令
- 可以用方向键浏览历史

---

### 8.35 `xsource` - 执行脚本

**功能说明**：读取脚本文件并逐行执行其中的命令。

**通俗理解**：
像"批处理文件"或"自动化脚本"，把多个命令写在文件里，一次性执行。

**脚本文件格式**：

```bash
# 这是注释，会被忽略
xcd /home/user

# 创建目录结构
xmkdir -p project/src
xmkdir -p project/docs

# 初始化文件
xtouch project/README.md
xecho "Hello World" > project/src/main.c
```

**实现流程**：

```
用户输入 xsource setup.sh
     ↓
[步骤1] 打开文件
     ↓
[步骤2] 逐行读取
     ↓
[步骤3] 对每行：
     ├── 空行或#开头 → 跳过
     └── 有效命令 → 解析并执行
     ↓
[步骤4] 如果 ctx->running == 0（执行了quit）
     │      停止执行
     ↓
[步骤5] 关闭文件，返回结果
```

**核心代码解读**：

```c
int cmd_xsource(Command *cmd, ShellContext *ctx) {
    FILE *file = fopen(filename, "r");
    char line[1024];
    
    while (fgets(line, sizeof(line), file)) {
        // 去除换行符和前导空格
        char *start = line;
        while (*start == ' ' || *start == '\t') start++;
        
        // 跳过空行和注释
        if (*start == '\0' || *start == '#') continue;
        
        // 解析并执行命令
        Command *script_cmd = parse_command(start);
        execute_command(script_cmd, ctx);
        free_command(script_cmd);
        
        // 如果Shell要退出，停止执行
        if (!ctx->running) break;
    }
    
    fclose(file);
}
```

**关键点**：
- 逐行读取和执行
- 支持注释（`#` 开头）
- 命令失败不影响后续执行

---

### 8.36 `xunalias` - 删除别名

**功能说明**：删除指定的命令别名。

**通俗理解**：
取消之前设置的"命令快捷方式"。

**核心代码解读**：

```c
int cmd_xunalias(Command *cmd, ShellContext *ctx) {
    // 遍历所有参数
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *name = cmd->args[i];
        
        if (alias_remove(name) != 0) {
            XSHELL_LOG_ERROR(ctx, "xunalias: %s: not found\n", name);
            has_error = 1;
        }
    }
    return has_error ? -1 : 0;
}

// 在 alias.c 中
int alias_remove(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp(aliases[i].name, name) == 0) {
            // 移动后面的元素
            for (int j = i; j < alias_count - 1; j++) {
                aliases[j] = aliases[j + 1];
            }
            alias_count--;
            return 0;
        }
    }
    return -1;  // 未找到
}
```

**关键点**：
- 从别名数组中删除对应项
- 不存在的别名会报错
- 支持一次删除多个

---

### 8.37 `xunset` - 删除环境变量

**功能说明**：删除指定的环境变量。

**通俗理解**：
取消设置的"全局配置"，删除后该变量不再存在。

**核心代码解读**：

```c
int cmd_xunset(Command *cmd, ShellContext *ctx) {
    for (int i = 1; i < cmd->arg_count; i++) {
        const char *var_name = cmd->args[i];
        
        // 验证变量名（只允许字母、数字、下划线）
        for (size_t j = 0; j < strlen(var_name); j++) {
            char c = var_name[j];
            if (!isalnum(c) && c != '_') {
                XSHELL_LOG_ERROR(ctx, "xunset: invalid variable: %s\n", var_name);
                continue;
            }
        }
        
        // 删除环境变量
        unsetenv(var_name);
    }
}
```

**关键点**：
- `unsetenv()` 系统函数删除变量
- 删除不存在的变量不会报错
- 变量名必须符合规范

---

### 8.38 `xtype` - 显示命令类型

**功能说明**：显示命令是内置命令、别名还是外部程序。

**通俗理解**：
查看一个命令的"真实身份"——是 Shell 内部功能、用户定义的快捷方式，还是系统中的程序。

**命令类型**：

| 类型 | 说明 | 示例 |
|-----|------|------|
| 内置命令 | Shell 内部实现 | `xcd`, `quit` |
| 别名 | 用户定义的快捷方式 | `ll` → `xls -la` |
| 外部命令 | 系统中的可执行文件 | `/usr/bin/ls` |

**实现流程**：

```
用户输入 xtype xls
     ↓
[步骤1] 检查是否是内置命令
     │      is_builtin("xls") → true
     └── 是 → 输出 "xls is a shell builtin"
     ↓
[步骤2] 检查是否是别名
     │      alias_get("xls") → NULL
     └── 是 → 输出 "xls is aliased to '...'"
     ↓
[步骤3] 在 PATH 中搜索
     │      search_in_path("xls")
     └── 找到 → 输出 "xls is /path/to/xls"
     ↓
[步骤4] 都不是
     └── 输出 "xls: not found"
```

**核心代码解读**：

```c
// 在 PATH 中搜索命令
static char* search_in_path(const char *command) {
    char *path_env = getenv("PATH");
    char *path_copy = strdup(path_env);
    
    char *dir = strtok(path_copy, ":");
    while (dir != NULL) {
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        if (access(full_path, X_OK) == 0) {
            return full_path;   // 找到可执行文件
        }
        dir = strtok(NULL, ":");
    }
    return NULL;
}
```

**关键点**：
- 按优先级检查：内置 → 别名 → 外部
- PATH 搜索使用 `access()` 检查可执行权限
- 类似 bash 的 `type` 命令

---

### 8.39 `xsleep` - 休眠

**功能说明**：暂停执行指定的秒数。

**通俗理解**：
让 Shell"睡一会儿"。常用于脚本中添加延迟。

**核心代码解读**：

```c
int cmd_xsleep(Command *cmd, ShellContext *ctx) {
    const char *seconds_str = cmd->args[1];
    
    // 验证是否是有效的正整数
    if (!is_valid_number(seconds_str)) {
        XSHELL_LOG_ERROR(ctx, "xsleep: invalid time: '%s'\n", seconds_str);
        return -1;
    }
    
    int seconds = atoi(seconds_str);
    
    // 执行休眠
    sleep(seconds);
    
    return 0;
}
```

**关键点**：
- `sleep()` 系统调用实现休眠
- 休眠期间 Shell 被阻塞
- 可以用 Ctrl+C 中断

---

### 8.40 `xdate` - 显示日期时间

**功能说明**：显示当前的日期和时间。

**通俗理解**：
查看"现在几点了"，支持本地时间和 UTC 时间。

**实现流程**：

```
用户输入 xdate
     ↓
[步骤1] 获取当前时间
     │      time_t now = time(NULL)
     ↓
[步骤2] 转换时间格式
     ├── 本地时间 → localtime(&now)
     └── -u 选项  → gmtime(&now) (UTC)
     ↓
[步骤3] 格式化输出
     │      strftime("Thu Oct 30 14:30:25 CST 2025")
```

**核心代码解读**：

```c
int cmd_xdate(Command *cmd, ShellContext *ctx) {
    int use_utc = (strcmp(cmd->args[1], "-u") == 0);
    
    // 获取当前时间
    time_t now = time(NULL);
    
    // 转换为结构化时间
    struct tm *timeinfo;
    if (use_utc) {
        timeinfo = gmtime(&now);     // UTC 时间
    } else {
        timeinfo = localtime(&now);  // 本地时间
    }
    
    // 格式化输出
    char buffer[256];
    strftime(buffer, sizeof(buffer), 
             "%a %b %d %H:%M:%S %Z %Y", timeinfo);
    printf("%s\n", buffer);
}
```

**时间格式说明**：

| 格式符 | 含义 | 示例 |
|-------|------|------|
| `%a` | 星期缩写 | Thu |
| `%b` | 月份缩写 | Oct |
| `%d` | 日期 | 30 |
| `%H:%M:%S` | 时:分:秒 | 14:30:25 |
| `%Z` | 时区 | CST |
| `%Y` | 年份 | 2025 |

**关键点**：
- `time()` 获取当前时间戳
- `localtime()` / `gmtime()` 转换时间
- `strftime()` 格式化输出

---

## 八（续）、命令实现详解（第五部分：41-50）

本节介绍清屏、帮助系统、系统信息查询、计算器等实用工具命令的实现。

---

### 8.41 `xclear` - 清屏

**功能说明**：清除终端屏幕内容。

**通俗理解**：
像显示器上的"清除"按钮，把屏幕上的所有内容都擦掉。

**实现原理**：

使用 ANSI 转义序列控制终端：

```
\033[2J  - 清除整个屏幕
\033[H   - 将光标移动到左上角(0,0)

完整序列：\033[2J\033[H
```

**核心代码解读**：

```c
int cmd_xclear(Command *cmd, ShellContext *ctx) {
    // 使用ANSI转义序列清屏
    // \033[2J - 清除整个屏幕
    // \033[H  - 移动光标到左上角(0,0)
    printf("\033[2J\033[H");
    fflush(stdout);  // 立即刷新输出
    return 0;
}
```

**关键点**：
- ANSI 转义序列是终端控制的标准方式
- `fflush()` 确保立即生效
- 不依赖任何外部程序

---

### 8.42 `xhelp` - 帮助系统

**功能说明**：显示所有命令的列表或特定命令的帮助。

**通俗理解**：
XShell 的"说明书"，告诉你所有可用的命令和如何使用它们。

**功能设计**：

```
xhelp          → 显示所有命令的分类列表
xhelp xls      → 显示 xls 命令的详细帮助

等价于：
xls --help
```

**核心代码解读**：

```c
int cmd_xhelp(Command *cmd, ShellContext *ctx) {
    // 没有参数：显示所有命令列表
    if (cmd->arg_count == 1) {
        show_all_commands();
        return 0;
    }
    
    // 显示特定命令的帮助
    const char *command = cmd->args[1];
    
    // 构造带--help的命令
    Command help_cmd;
    help_cmd.name = command;
    help_cmd.args[1] = "--help";
    help_cmd.arg_count = 2;
    
    // 执行该命令的帮助
    if (is_builtin(command)) {
        return execute_builtin(&help_cmd, ctx);
    } else {
        printf("xhelp: %s: command not found\n", command);
        return -1;
    }
}
```

**关键点**：
- 命令按功能分类显示
- 支持查看特定命令帮助
- 调用内置命令的 `--help` 选项

---

### 8.43 `xwhoami` - 显示当前用户

**功能说明**：显示当前登录的用户名。

**通俗理解**：
回答"我是谁？"——显示你正在用哪个账户运行 Shell。

**实现原理**：

```
步骤1: getuid() → 获取用户ID (如 1000)
     ↓
步骤2: getpwuid(uid) → 查询用户数据库
     ↓
步骤3: 返回 passwd 结构 → pw_name = "username"
```

**核心代码解读**：

```c
int cmd_xwhoami(Command *cmd, ShellContext *ctx) {
    // 获取当前用户ID
    uid_t uid = getuid();
    
    // 通过用户ID获取用户信息
    struct passwd *pw = getpwuid(uid);
    if (pw == NULL) {
        perror("xwhoami");
        return -1;
    }
    
    // 输出用户名
    printf("%s\n", pw->pw_name);
    
    return 0;
}
```

**关键点**：
- `getuid()` 获取用户 ID
- `getpwuid()` 从 `/etc/passwd` 查询用户信息
- `passwd` 结构包含用户名、主目录等信息

---

### 8.44 `xhostname` - 显示主机名

**功能说明**：显示当前系统的主机名。

**通俗理解**：
像问计算机"你叫什么名字？"——返回计算机的网络名称。

**核心代码解读**：

```c
int cmd_xhostname(Command *cmd, ShellContext *ctx) {
    char hostname[HOST_NAME_MAX + 1];
    
    // 获取主机名
    if (gethostname(hostname, sizeof(hostname)) == -1) {
        perror("xhostname");
        return -1;
    }
    
    printf("%s\n", hostname);
    return 0;
}
```

**关键点**：
- `gethostname()` 系统调用获取主机名
- `HOST_NAME_MAX` 定义最大主机名长度
- 主机名配置在 `/etc/hostname`

---

### 8.45 `xuname` - 显示系统信息

**功能说明**：显示系统信息，包括内核名称、版本等。

**通俗理解**：
查看操作系统的"身份证"——系统类型、版本、架构等信息。

**选项说明**：

| 选项 | 显示内容 | 示例 |
|-----|---------|------|
| `-s` | 内核名称 | Linux |
| `-n` | 主机名 | ubuntu-server |
| `-r` | 内核版本 | 6.2.0-26-generic |
| `-m` | 硬件架构 | x86_64 |
| `-a` | 所有信息 | Linux ubuntu 6.2.0... |

**核心代码解读**：

```c
int cmd_xuname(Command *cmd, ShellContext *ctx) {
    struct utsname info;
    
    // 获取系统信息
    if (uname(&info) == -1) {
        perror("xuname");
        return -1;
    }
    
    // 根据选项输出信息
    if (show_all) {
        printf("%s %s %s %s %s\n",
               info.sysname,     // 内核名称
               info.nodename,    // 主机名
               info.release,     // 版本号
               info.version,     // 发布版本
               info.machine);    // 硬件架构
    }
}
```

**关键点**：
- `uname()` 系统调用获取系统信息
- `utsname` 结构包含5个字段
- 默认只显示内核名称

---

### 8.46 `xwhich` - 显示命令路径

**功能说明**：在 PATH 环境变量中搜索命令的完整路径。

**通俗理解**：
问"这个命令在哪里？"——显示可执行文件的完整位置。

**与 `xtype` 的区别**：

| 命令 | 搜索范围 | 用途 |
|-----|---------|------|
| `xtype` | 内置 + 别名 + 外部 | 判断命令类型 |
| `xwhich` | 只搜索 PATH | 找外部命令路径 |

**核心代码解读**：

```c
static int find_command(const char *command) {
    char *path_env = getenv("PATH");
    
    // 复制PATH（strtok会修改字符串）
    char *path_copy = strdup(path_env);
    
    char full_path[1024];
    char *dir = strtok(path_copy, ":");
    
    while (dir != NULL) {
        // 构造完整路径
        snprintf(full_path, sizeof(full_path), "%s/%s", dir, command);
        
        // 检查是否可执行
        if (access(full_path, X_OK) == 0) {
            printf("%s\n", full_path);
            free(path_copy);
            return 0;
        }
        
        dir = strtok(NULL, ":");
    }
    
    free(path_copy);
    return -1;
}
```

**关键点**：
- PATH 用冒号分隔多个目录
- `access()` 检查文件权限
- `X_OK` 标志检查是否可执行

---

### 8.47 `xcalc` - 计算器

**功能说明**：计算简单的数学表达式。

**通俗理解**：
命令行计算器，支持基本四则运算和括号。

**支持的功能**：

```
基本运算:     xcalc '10 + 5'        → 15
优先级:       xcalc '2 + 3 * 4'     → 14 (先乘后加)
括号:         xcalc '(2 + 3) * 4'   → 20
取模:         xcalc '17 % 5'        → 2
负数:         xcalc '-5 + 3'        → -2
浮点数:       xcalc '3.14 * 2'      → 6.28
```

**递归下降解析器**：

```
表达式解析层次结构：

parse_expression()  ← 处理 + -
    ↓
parse_term()        ← 处理 * / %
    ↓
parse_factor()      ← 处理数字、括号、负号
```

**核心代码解读**：

```c
// 解析因子（最底层）
static int parse_factor(ParseState *state, double *result) {
    if (*state->pos == '(') {
        state->pos++;
        parse_expression(state, result);  // 递归
        // 期待 ')'
        state->pos++;
        return 0;
    } else if (*state->pos == '-') {
        state->pos++;
        parse_factor(state, result);
        *result = -*result;  // 取负
        return 0;
    } else {
        return parse_number(state, result);  // 普通数字
    }
}

// 解析项（处理乘除）
static int parse_term(ParseState *state, double *result) {
    parse_factor(state, result);
    
    while (*state->pos == '*' || *state->pos == '/') {
        char op = *state->pos++;
        double right;
        parse_factor(state, &right);
        
        if (op == '*') *result *= right;
        else           *result /= right;
    }
}
```

**关键点**：
- 递归下降是编译原理的经典算法
- 通过函数调用层次实现运算符优先级
- 支持任意嵌套的括号

---

### 8.48 `xtime` - 测量执行时间

**功能说明**：执行命令并测量其执行时间。

**通俗理解**：
像秒表一样测量命令需要多长时间运行完。

**核心代码解读**：

```c
int cmd_xtime(Command *cmd, ShellContext *ctx) {
    // 构建要执行的命令
    char command_str[1024];
    // 将 xtime 后面的参数组合成命令...
    
    // 记录开始时间
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    // 执行命令
    Command *time_cmd = parse_command(command_str);
    int result = execute_command(time_cmd, ctx);
    
    // 记录结束时间
    gettimeofday(&end, NULL);
    
    // 计算时间差（秒）
    long seconds = end.tv_sec - start.tv_sec;
    long microseconds = end.tv_usec - start.tv_usec;
    double elapsed = seconds + microseconds / 1000000.0;
    
    printf("\n执行时间: %.3f秒\n", elapsed);
    
    free_command(time_cmd);
    return result;
}
```

**关键点**：
- `gettimeofday()` 获取微秒级精度时间
- 时间差 = 结束时间 - 开始时间
- 单位转换：微秒 / 1000000 = 秒

---

### 8.49 `xtree` - 树形显示目录

**功能说明**：以树形结构递归显示目录内容。

**通俗理解**：
像文件管理器的目录树视图，显示文件夹结构。

**输出格式**：

```
.
├── src/
│   ├── main.c
│   └── utils.c
├── include/
│   └── header.h
└── Makefile

2 directories, 4 files
```

**树形字符绘制**：

```c
static void print_tree_prefix(int depth, int is_last[], int is_last_entry) {
    for (int i = 0; i < depth - 1; i++) {
        if (is_last[i]) {
            printf("    ");    // 空格（上级是最后一项）
        } else {
            printf("│   ");    // 竖线（上级还有后续）
        }
    }
    
    if (depth > 0) {
        if (is_last_entry) {
            printf("└── ");    // 最后一项
        } else {
            printf("├── ");    // 中间项
        }
    }
}
```

**核心递归逻辑**：

```c
static void show_tree(const char *path, int depth, int is_last[]) {
    DIR *dir = opendir(path);
    
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (is_hidden(entry->d_name)) continue;
        
        print_tree_prefix(depth, is_last, is_last_entry);
        
        if (S_ISDIR(st.st_mode)) {
            printf("%s/\n", entry->d_name);
            dir_count++;
            
            // 递归显示子目录
            is_last[depth] = is_last_entry;
            show_tree(full_path, depth + 1, is_last);
        } else {
            printf("%s\n", entry->d_name);
            file_count++;
        }
    }
}
```

**关键点**：
- `is_last[]` 数组记录每层是否是最后一项
- 递归遍历子目录
- `-L` 选项限制显示深度

---

### 8.50 `xuptime` - 系统运行时间

**功能说明**：显示系统已运行的时间。

**通俗理解**：
查看"电脑开机多久了"。

**实现原理**：

```
步骤1: 读取 /proc/uptime
       内容: "12345.67 ..."
       含义: 系统运行了12345.67秒

步骤2: 转换为可读格式
       12345 秒 = 3小时 25分钟 45秒
```

**核心代码解读**：

```c
int cmd_xuptime(Command *cmd, ShellContext *ctx) {
    // 输出当前时间
    time_t now = time(NULL);
    struct tm *timeinfo = localtime(&now);
    printf("%02d:%02d:%02d up ",
           timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
    
    // 读取系统运行时间
    FILE *fp = fopen("/proc/uptime", "r");
    double uptime_seconds;
    fscanf(fp, "%lf", &uptime_seconds);
    fclose(fp);
    
    // 转换为天、小时、分钟
    int days = (int)(uptime_seconds / 86400);
    int hours = (int)((uptime_seconds - days * 86400) / 3600);
    int minutes = (int)((uptime_seconds - ...) / 60);
    
    // 格式化输出
    if (days > 0) {
        printf("%d days, %d:%02d", days, hours, minutes);
    } else {
        printf("%d:%02d", hours, minutes);
    }
}
```

**时间换算**：
- 1天 = 86400秒
- 1小时 = 3600秒
- 1分钟 = 60秒

**关键点**：
- `/proc/uptime` 是 Linux 虚拟文件系统
- 第一个数字是系统运行秒数
- 单位换算：秒 → 天时分

---

## 八（续）、命令实现详解（第六部分：51-60）

本节介绍磁盘管理、权限修改、路径处理、文件类型检测等命令的实现。

---

### 8.51 `xdu` - 显示目录大小

**功能说明**：显示目录及其子目录的磁盘使用量。

**通俗理解**：
查看"文件夹有多大"——计算目录占用的磁盘空间。

**实现原理**：

```
递归遍历目录：
目录A/
├── file1.txt (100B)
├── file2.txt (200B)
└── subdir/
    └── file3.txt (150B)

总大小 = 100 + 200 + 150 = 450B
```

**核心代码解读**：

```c
// 递归计算目录大小
static long long calculate_dir_size(const char *dir_path) {
    long long total_size = 0;
    DIR *dir = opendir(dir_path);
    
    while ((entry = readdir(dir)) != NULL) {
        // 跳过 . 和 ..
        if (is_dot(entry->d_name)) continue;
        
        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);
        lstat(path, &st);
        
        if (S_ISDIR(st.st_mode)) {
            // 递归计算子目录
            total_size += calculate_dir_size(path);
        } else {
            // 累加文件大小
            total_size += st.st_size;
        }
    }
    closedir(dir);
    return total_size;
}
```

**关键点**：
- 递归遍历所有子目录
- `st_size` 获取文件大小
- `-h` 选项人性化显示（KB/MB/GB）

---

### 8.52 `xdf` - 显示磁盘空间

**功能说明**：显示文件系统的磁盘空间使用情况。

**通俗理解**：
查看"磁盘还剩多少空间"——硬盘总容量、已用、可用。

**输出格式**：

```
文件系统           总大小      已用      可用   使用%  挂载点
/                  100G       40G       60G    40%   /
```

**核心代码解读**：

```c
static int show_filesystem(const char *path, int human_readable) {
    struct statvfs vfs;
    statvfs(path, &vfs);
    
    // 计算空间（字节）
    unsigned long long block_size = vfs.f_frsize;
    unsigned long long total = vfs.f_blocks * block_size;
    unsigned long long free = vfs.f_bfree * block_size;
    unsigned long long available = vfs.f_bavail * block_size;
    unsigned long long used = total - free;
    
    unsigned long long percent = (used * 100) / total;
    
    printf("%-20s %10s %10s %10s %5llu%% %s\n",
           path, total_str, used_str, available_str, percent, path);
}
```

**关键点**：
- `statvfs()` 获取文件系统信息
- `f_blocks` 是总块数
- `f_bfree` 是空闲块数

---

### 8.53 `xchown` - 修改所有者

**功能说明**：修改文件或目录的所有者和所属组。

**通俗理解**：
改变文件"属于谁"——像转让物品的所有权。

**格式说明**：

```
xchown user file        # 只改所有者
xchown user:group file  # 改所有者和组
xchown :group file      # 只改组
```

**核心代码解读**：

```c
// 解析用户名和组名
static int parse_owner_group(const char *spec, uid_t *uid, gid_t *gid) {
    char *colon = strchr(spec, ':');
    
    if (colon != NULL) {
        *colon = '\0';
        user_part = spec;
        group_part = colon + 1;
    }
    
    // 用户名 → UID
    struct passwd *pwd = getpwnam(user_part);
    *uid = pwd->pw_uid;
    
    // 组名 → GID
    struct group *grp = getgrnam(group_part);
    *gid = grp->gr_gid;
}

// 修改所有者
chown(path, uid, gid);
```

**关键点**：
- `getpwnam()` 查询用户名对应的 UID
- `getgrnam()` 查询组名对应的 GID
- 通常需要 root 权限

---

### 8.54 `xfile` - 检测文件类型

**功能说明**：检测并显示文件的类型。

**通俗理解**：
问文件"你是什么类型？"——图片、程序、文本等。

**检测方法**：

```
1. 检查魔数（文件头）
   ELF: 0x7f E L F
   PNG: 0x89 P N G
   ZIP: P K 0x03 0x04

2. 检查扩展名
   .c → C source
   .py → Python script

3. 检查文件模式
   S_ISDIR → directory
   S_ISLNK → symbolic link
```

**核心代码解读**：

```c
static const char* check_magic(const char *filename) {
    unsigned char magic[16];
    fread(magic, 1, 16, file);
    
    // ELF 可执行文件
    if (magic[0] == 0x7f && magic[1] == 'E' 
        && magic[2] == 'L' && magic[3] == 'F') {
        return "ELF executable";
    }
    
    // PNG 图片
    if (magic[0] == 0x89 && magic[1] == 'P' 
        && magic[2] == 'N' && magic[3] == 'G') {
        return "PNG image";
    }
    // ...
}
```

**关键点**：
- 魔数是最准确的识别方式
- 扩展名作为后备方案
- 检查内容判断是文本还是二进制

---

### 8.55 `xpaste` - 合并文件行

**功能说明**：将多个文件的行按列合并。

**通俗理解**：
把两个列表并排放在一起，像 Excel 的"并排对比"。

**效果示例**：

```
file1.txt:          file2.txt:        xpaste file1.txt file2.txt:
apple               red               apple   red
banana              yellow            banana  yellow
cherry              red               cherry  red
```

**核心代码解读**：

```c
int cmd_xpaste(Command *cmd, ShellContext *ctx) {
    // 打开所有文件
    for (i = 0; i < argc; i++) {
        files[i] = fopen(filenames[i], "r");
    }
    
    // 逐行合并
    while (!all_eof) {
        // 读取每个文件的下一行
        for (j = 0; j < file_count; j++) {
            fgets(lines[j], sizeof(lines[j]), files[j]);
        }
        
        // 输出合并的行
        for (j = 0; j < file_count; j++) {
            if (j > 0) putchar(delimiter);
            printf("%s", lines[j]);
        }
        putchar('\n');
    }
}
```

**关键点**：
- 同时读取多个文件
- 用分隔符连接各文件的同一行
- 文件长度不同时用空字符串补齐

---

### 8.56 `xreadlink` - 读取符号链接

**功能说明**：显示符号链接指向的目标路径。

**通俗理解**：
像查看"快捷方式指向哪里"。

**核心代码解读**：

```c
int cmd_xreadlink(Command *cmd, ShellContext *ctx) {
    if (canonicalize) {
        // -f: 解析到最终目标
        char resolved[PATH_MAX];
        realpath(link_path, resolved);
        printf("%s\n", resolved);
    } else {
        // 直接读取链接内容
        char target[PATH_MAX];
        ssize_t len = readlink(link_path, target, sizeof(target) - 1);
        target[len] = '\0';
        printf("%s\n", target);
    }
}
```

**关键点**：
- `readlink()` 读取链接的直接目标
- `realpath()` 解析所有链接到最终目标
- `-f` 选项用于获取绝对路径

---

### 8.57 `xrealpath` - 显示绝对路径

**功能说明**：显示文件的绝对路径（解析所有符号链接）。

**通俗理解**：
把相对路径转换成完整路径，像导航给出的"精确地址"。

**示例**：

```
当前目录: /home/user/project

xrealpath ../docs/readme.md
输出: /home/user/docs/readme.md
```

**核心代码解读**：

```c
int cmd_xrealpath(Command *cmd, ShellContext *ctx) {
    char resolved[PATH_MAX];
    
    if (realpath(path, resolved) != NULL) {
        printf("%s\n", resolved);
    } else {
        // 手动构造绝对路径
        if (path[0] != '/') {
            getcwd(cwd, sizeof(cwd));
            snprintf(resolved, sizeof(resolved), "%s/%s", cwd, path);
        }
    }
}
```

**关键点**：
- `realpath()` 是核心函数
- 解析 `.` 和 `..`
- 解析符号链接到最终目标

---

### 8.58 `xbasename` - 提取文件名

**功能说明**：从路径中提取文件名（去除目录部分）。

**通俗理解**：
从地址中只取"门牌号"——`/path/to/file.txt` → `file.txt`

**示例**：

```
xbasename /home/user/docs/report.txt     → report.txt
xbasename /home/user/docs/report.txt .txt → report
```

**核心代码解读**：

```c
int cmd_xbasename(Command *cmd, ShellContext *ctx) {
    // 查找最后一个 '/'
    const char *filename = strrchr(path, '/');
    if (filename == NULL) {
        filename = path;   // 没有目录分隔符
    } else {
        filename++;        // 跳过 '/'
    }
    
    // 如果指定了后缀，去除后缀
    if (suffix != NULL) {
        size_t fn_len = strlen(filename);
        size_t sf_len = strlen(suffix);
        
        if (ends_with(filename, suffix)) {
            // 输出不含后缀的部分
            printf("%.*s\n", (int)(fn_len - sf_len), filename);
        }
    }
}
```

**关键点**：
- `strrchr()` 找最后一个分隔符
- 第二个参数可指定要去除的后缀
- 常用于脚本中提取文件名

---

### 8.59 `xdirname` - 提取目录名

**功能说明**：从路径中提取目录部分（去除文件名）。

**通俗理解**：
从地址中取"街道名"——`/path/to/file.txt` → `/path/to`

**示例**：

```
xdirname /home/user/docs/report.txt → /home/user/docs
xdirname file.txt                   → .
xdirname /                          → /
```

**核心代码解读**：

```c
int cmd_xdirname(Command *cmd, ShellContext *ctx) {
    const char *last_slash = strrchr(path, '/');
    
    if (last_slash == NULL) {
        printf(".\n");           // 无目录分隔符，返回当前目录
    } else if (last_slash == path) {
        printf("/\n");           // 根目录
    } else {
        // 输出目录部分（不含末尾的 '/'）
        size_t dir_len = last_slash - path;
        printf("%.*s\n", (int)dir_len, path);
    }
}
```

**关键点**：
- 与 `xbasename` 互补
- 单独的文件名返回 `.`
- 常用于脚本中获取文件所在目录

---

### 8.60 `xtec` (tee) - 分流输出

**功能说明**：从标准输入读取数据，同时输出到标准输出和文件。

**通俗理解**：
像水管的"三通接头"——水流同时流向两个方向。一方面显示在屏幕上，一方面保存到文件。

**使用场景**：

```
xls | xtec listing.txt

效果：
1. xls 的输出显示在屏幕上
2. 同时保存到 listing.txt 文件
```

**核心代码解读**：

```c
int cmd_xtec(Command *cmd, ShellContext *ctx) {
    // 打开所有输出文件
    const char *mode = append_mode ? "a" : "w";
    for (i = 0; i < file_count; i++) {
        files[i] = fopen(filenames[i], mode);
    }
    
    // 逐字符读取并分流输出
    int ch;
    while ((ch = getchar()) != EOF) {
        // 输出到屏幕
        putchar(ch);
        
        // 输出到所有文件
        for (i = 0; i < file_count; i++) {
            fputc(ch, files[i]);
        }
    }
    
    // 关闭文件
    for (i = 0; i < file_count; i++) {
        fclose(files[i]);
    }
}
```

**关键点**：
- 需要配合管道使用
- `-a` 选项追加而不覆盖
- 支持同时写入多个文件

---

## 八（续）、命令实现详解（第七部分：61-65）

本节介绍文件比较、分割、连接以及特色功能命令的实现。

---

### 8.61 `xcomm` - 比较排序文件

**功能说明**：比较两个已排序的文件，显示共同行和独有行。

**通俗理解**：
像 Excel 中的"比对"功能——找出两个列表的共同项和各自独有的项。

**输出格式**：

```
三列格式：
第1列: 只在文件1中出现
第2列: 只在文件2中出现
第3列: 两个文件都有

例：
apple                    ← 文件1独有
        banana           ← 文件2独有
                cherry   ← 共同行
```

**核心代码解读**：

```c
int cmd_xcomm(Command *cmd, ShellContext *ctx) {
    // 读取两个文件的第一行
    has_line1 = read_line(f1, line1, sizeof(line1));
    has_line2 = read_line(f2, line2, sizeof(line2));
    
    while (has_line1 || has_line2) {
        if (!has_line1) {
            // 文件1结束，剩余都是文件2独有
            if (!hide_col2) printf("\t%s\n", line2);
            has_line2 = read_line(f2, line2, sizeof(line2));
        } else if (!has_line2) {
            // 文件2结束，剩余都是文件1独有
            if (!hide_col1) printf("%s\n", line1);
            has_line1 = read_line(f1, line1, sizeof(line1));
        } else {
            int cmp = strcmp(line1, line2);
            if (cmp < 0) {
                // 文件1独有（因为已排序，line1更小说明文件2没有它）
                if (!hide_col1) printf("%s\n", line1);
                has_line1 = read_line(f1, line1);
            } else if (cmp > 0) {
                // 文件2独有
                if (!hide_col2) printf("\t%s\n", line2);
                has_line2 = read_line(f2, line2);
            } else {
                // 共同行
                if (!hide_col3) printf("\t\t%s\n", line1);
                has_line1 = read_line(f1, line1);
                has_line2 = read_line(f2, line2);
            }
        }
    }
}
```

**关键点**：
- **前提**：文件必须已排序
- 利用排序特性高效比较（归并算法思想）
- `-1` `-2` `-3` 选项隐藏对应列

---

### 8.62 `xsplit` - 分割文件

**功能说明**：将大文件分割成多个小文件。

**通俗理解**：
像"拆分文件"工具——把一个大文件切成多个小块。

**分割方式**：

```
-l 行数：按行数分割
  xsplit -l 1000 big.txt
  → xaa (前1000行), xab (1001-2000行), ...

-b 大小：按字节分割
  xsplit -b 10M big.zip
  → xaa (10MB), xab (10MB), ...
```

**核心代码解读**：

```c
// 按行数分割
static int split_by_lines(const char *input_file, const char *prefix, int lines_per_file) {
    int file_index = 0;
    int line_count = 0;
    FILE *output = NULL;
    
    while (fgets(line, sizeof(line), input) != NULL) {
        if (line_count == 0) {
            // 开始新文件
            generate_filename(prefix, file_index, output_filename);
            output = fopen(output_filename, "w");
            file_index++;
        }
        
        fputs(line, output);
        line_count++;
        
        if (line_count >= lines_per_file) {
            line_count = 0;  // 触发创建下一个文件
        }
    }
}

// 生成文件名：xaa, xab, xac...
static void generate_filename(const char *prefix, int index, char *filename) {
    char a = 'a' + (index / 26);
    char b = 'a' + (index % 26);
    snprintf(filename, size, "%s%c%c", prefix, a, b);
}
```

**关键点**：
- 支持 K/M/G 后缀（智能大小解析）
- 输出文件名自动递增（xaa, xab...）
- 用于拆分日志、备份传输等

---

### 8.63 `xjoin` - 连接文件

**功能说明**：基于共同字段连接两个文件（类似 SQL JOIN）。

**通俗理解**：
像数据库的"表连接"——根据共同的键将两个表合并。

**示例**：

```
file1.txt:              file2.txt:
1001 Alice              1001 Sales
1002 Bob                1002 IT
1003 Carol              1003 HR

xjoin file1.txt file2.txt:
1001 Alice 1001 Sales
1002 Bob 1002 IT
1003 Carol 1003 HR
```

**核心代码解读**：

```c
int cmd_xjoin(Command *cmd, ShellContext *ctx) {
    // 读取两个文件
    while (has_line1 && has_line2) {
        const char *key1 = fields1[field1 - 1];  // 连接字段
        const char *key2 = fields2[field2 - 1];
        int cmp = strcmp(key1, key2);
        
        if (cmp < 0) {
            // 文件1的键更小，读取文件1下一行
            has_line1 = read_next(f1);
        } else if (cmp > 0) {
            // 文件2的键更小，读取文件2下一行
            has_line2 = read_next(f2);
        } else {
            // 键匹配，输出连接结果
            printf("%s %s\n", line1, line2);
            has_line1 = read_next(f1);
        }
    }
}
```

**关键点**：
- **前提**：文件必须按连接字段排序
- `-1` `-2` 指定连接字段
- `-t` 指定分隔符

---

### 8.64 `xsysmon` - 系统监控

**功能说明**：实时显示 CPU、内存、磁盘使用情况的监控界面。

**通俗理解**：
类似 Windows 的"任务管理器"或 Linux 的 `htop`——美观的系统资源监控。

**界面预览**：

```
╔════════════════════════════════════════════╗
║              [ 系统监控 ]                  ║
╟────────────────────────────────────────────╢
║ 主机名: ubuntu-server                      ║
║ 系统:   Linux 6.2.0-26-generic             ║
║ 运行:   2天 5小时 30分钟                   ║
║ 负载:   0.15  0.20  0.18                   ║
╟────────────────────────────────────────────╢
║ CPU  [████████░░░░░░░░░░░░░░░░░░░░░]  25%  ║
║ 内存 [████████████░░░░░░░░░░░░░░░░░]  40%  ║
║      2.1 GB / 8.0 GB                       ║
║ 磁盘 [████████████████████░░░░░░░░░]  65%  ║
║      130 GB / 200 GB                       ║
╟────────────────────────────────────────────╢
║ 按 Q 退出 | 自动刷新中...         12:30:45 ║
╚════════════════════════════════════════════╝
```

**核心实现技术**：

```c
// 获取 CPU 使用率（从 /proc/stat）
static int get_cpu_usage(void) {
    static long prev_idle = 0, prev_total = 0;
    
    FILE *fp = fopen("/proc/stat", "r");
    fscanf(fp, "cpu %ld %ld %ld %ld ...", &user, &nice, &system, &idle);
    
    total = user + nice + system + idle + ...;
    idle = idle_time + iowait;
    
    // 计算差值百分比
    usage = 100 * (diff_total - diff_idle) / diff_total;
    
    prev_idle = idle;
    prev_total = total;
    return usage;
}

// 绘制进度条（彩色）
static void draw_progress_bar(int percent, int width) {
    int filled = (percent * width) / 100;
    
    // 根据百分比选择颜色
    if (percent < 50) color = GREEN;
    else if (percent < 80) color = YELLOW;
    else color = RED;
    
    printf("[");
    for (int i = 0; i < width; i++) {
        if (i < filled) printf("█");
        else printf("░");
    }
    printf("] %d%%", percent);
}
```

**关键点**：
- 读取 `/proc/stat` 获取 CPU 信息
- 使用 `sysinfo()` 获取内存信息
- 使用 `statvfs()` 获取磁盘信息
- 每秒自动刷新
- 彩色进度条（绿/黄/红）

---

### 8.65 `xmenu` - 交互式菜单

**功能说明**：显示交互式菜单，支持键盘导航。

**通俗理解**：
像安装程序的"菜单选择"界面——用方向键选择、回车执行。

**界面效果**：

```
XShell 主菜单

  > 1. 显示当前目录    ← 高亮（当前选中）
    2. 列出文件
    3. 显示历史记录
    4. 显示帮助
    5. 清屏
    6. 退出 Shell

使用方向键选择，Enter 执行，q 退出
```

**核心代码解读**：

```c
// 设置终端为原始模式（读取单个按键）
static void set_raw_mode(void) {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);  // 禁用行缓冲和回显
    tcsetattr(STDIN_FILENO, TCSANOW, &term);
}

// 读取方向键
static int read_key(void) {
    char ch;
    read(STDIN_FILENO, &ch, 1);
    
    if (ch == '\033') {  // 转义序列
        char seq[2];
        read(STDIN_FILENO, &seq[0], 1);
        read(STDIN_FILENO, &seq[1], 1);
        
        if (seq[0] == '[') {
            switch (seq[1]) {
                case 'A': return UP;
                case 'B': return DOWN;
            }
        }
    }
    return ch;
}

// 主循环
while (running) {
    display_menu(&menu, selected);
    
    int key = read_key();
    switch (key) {
        case UP:   selected--; break;
        case DOWN: selected++; break;
        case ENTER: execute_menu_item(&menu.items[selected]); break;
        case 'q':   running = 0; break;
    }
}
```

**关键点**：
- `termios` 设置终端为原始模式
- 解析 ANSI 转义序列识别方向键
- 高亮显示当前选中项
- 执行选中的命令

---

## 八（续）、命令实现详解（第八部分：特色功能）

本节介绍 XShell 的图形化界面、网络功能和内置游戏系统，展示了 Shell 在常规命令之外的扩展能力。

---

### 8.66 `xui` - 图形化主菜单

**功能说明**：提供交互式的图形化菜单界面，整合常用功能。

**通俗理解**：
给命令行穿上"图形界面"的外衣——用键盘选择菜单而不是输入命令。

**界面设计**：

```
+--------------------------------------------------+
|               XShell UI 控制面板  v1.0            |
+--------------------------------------------------+
|  上下键选择  Enter执行  0-9快捷键  q退出         |
+--------------------------------------------------+
|                                                  |
|    📝 1. 执行命令        🐍 5. 贪吃蛇            |
|    💻 2. 系统监控        📜 6. 历史记录          |
|    🌐 3. 网页浏览        🎮 7. 俄罗斯方块        |
|    📁 4. 文件列表        🧮 8. 计算器            |
|                         🎲 9. 2048               |
|                         🚪 0. 退出 UI            |
|                                                  |
+--------------------------------------------------+
| 提示: 设置 TERM=xterm-256color 获得最佳效果      |
+--------------------------------------------------+
```

**技术实现**：
1.  **终端控制**：使用 `termios` 进入原始模式，接管所有按键输入。
2.  **备用屏幕**：使用 ANSI 转义序列 `\033[?1049h` 进入备用缓冲区，退出时恢复现场。
3.  **UI 渲染**：封装了绘制边框、标题栏、菜单项的函数库 `xui_widgets.c`。
4.  **事件循环**：非阻塞读取键盘输入，支持 ANSI 转义序列解析（方向键）。

**核心代码解读**：

```c
// 处理键盘输入（支持方向键和快捷键）
static int handle_input() {
    int key = xui_term_read_key();
    switch (key) {
        case XUI_KEY_UP:    // 上
            if (selected > 0) selected--;
            break;
        case XUI_KEY_DOWN:  // 下
            if (selected < count - 1) selected++;
            break;
        case XUI_KEY_ENTER: // 确认
            execute_item(items[selected]);
            break;
    }
}
```

---

### 8.67 `xweb` - 终端浏览器

**功能说明**：在终端中浏览网页内容，支持 Bing/搜狗搜索。

**通俗理解**：
命令行的"Chrome"——虽然只能看文字，但能查资料、看新闻。

**智能提取引擎**：
为了在终端提供良好的阅读体验，`xweb` 实现了多级渲染策略：

1.  **优先策略**：检测系统是否安装 `lynx` / `w3m` / `links`，如果有则直接调用。
2.  **降级策略**：如果没有文本浏览器，则使用 `curl` + `Python/Perl` 进行智能提取。
    *   **正文提取**：使用正则定位 `<div id="content">` 或 `<main>` 等核心区域。
    *   **标签清洗**：去除 `<script>`, `<style>`，将 `<div>`, `<p>` 转换为换行。
    *   **HTML转义**：将 `&lt;` 等实体转换为字符。

**核心代码解读**：

```c
// 智能正文提取（Python过滤器）
snprintf(cmd, sizeof(cmd), 
    "curl -s '%s' | python3 -c \""
    "import sys, re, html; "
    "c = sys.stdin.read(); "
    "c = re.sub(r'<(script|style)[^>]*>.*?</\\1>', '', c, flags=re.S); " // 去脚本
    "c = re.sub(r'<[^>]+>', '', c); " // 去标签
    "print(html.unescape(c).strip())\"", 
    url);
```

**关键点**：
- 支持 `weather` 查询天气
- 支持 `cheat <cmd>` 查询命令速查表
- 自动处理 User-Agent 伪装浏览器

---

### 8.68 `xsnake` - 贪吃蛇

**功能说明**：经典的贪吃蛇游戏。

**通俗理解**：
控制蛇吃豆子，越吃越长，撞墙或撞自己就输了。

**实现亮点**：
- **无闪烁绘制**：只重绘变化的部分（清除尾部，绘制新头部），而不是清空全屏。
- **碰撞检测**：预判下一步坐标是否在蛇身数组中。
- **排行榜**：记录并显示前三名高分。

**核心代码解读**：

```c
// 移动逻辑（循环队列思想）
// 1. 移动身体：从尾部开始，每一节移到前一节的位置
for (int i = snake.length - 1; i > 0; i--) {
    snake.body[i] = snake.body[i - 1];
}
// 2. 更新头部
snake.body[0] = new_head;

// 3. 进食判定
if (head.x == food.x && head.y == food.y) {
    length++;         // 变长
    spawn_food();     // 生成新食物
} else {
    clear_point(tail); // 清除旧尾巴的显示
}
```

---

### 8.69 `xtetris` - 俄罗斯方块

**功能说明**：经典的俄罗斯方块游戏。

**通俗理解**：
不同形状的方块掉下来，凑满一行消除得分。

**核心算法**：
1.  **方块表示**：使用 4x4 的二维数组表示7种方块（I, O, T, S, Z, J, L）。
2.  **碰撞检测**：遍历方块的非空格子，检查是否超出边界或与已固定方块重叠。
3.  **旋转算法**：预定义每个方块的4种旋转状态，旋转时直接切换索引。

**核心代码解读**：

```c
// 碰撞检测
static int can_place(int piece, int rot, int x, int y) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (pieces[piece][rot][r][c]) {
                int bx = x + c;
                int by = y + r;
                // 检查边界和已占用位置
                if (bx < 0 || bx >= 10 || by >= 20 || board[by][bx])
                    return 0;
            }
        }
    }
    return 1;
}
```

**关键点**：
- 支持软降（加速）和硬降（直接到底）
- 256色渲染，不同方块不同颜色
- 随分数提高自动加速

---

### 8.70 `x2048` - 2048 游戏

**功能说明**：滑动合并数字的益智游戏。

**通俗理解**：
上下左右滑动，相同的数字撞在一起会加倍（2+2=4, 4+4=8...），目标是凑出2048。

**核心算法**：
1.  **矩阵操作**：核心逻辑是"一行数据的合并"。4个方向的移动都可以转换为"行合并"问题。
    *   向左：直接合并每一行。
    *   向右：将每一行翻转，合并，再翻转回来。
    *   向上：矩阵转置，合并，再转置回来。
2.  **合并逻辑**：
    *   移除零（压缩）：`[2, 0, 2, 0]` → `[2, 2, 0, 0]`
    *   合并相邻：`[2, 2, 0, 0]` → `[4, 0, 0, 0]`

**核心代码解读**：

```c
// 单行合并逻辑
void merge_line(int *line) {
    // 1. 移动非零数到左边
    int pos = 0;
    for (int i = 0; i < 4; i++) {
        if (line[i] != 0) line[pos++] = line[i];
    }
    while (pos < 4) line[pos++] = 0;
    
    // 2. 合并相邻相同数
    for (int i = 0; i < 3; i++) {
        if (line[i] != 0 && line[i] == line[i+1]) {
            line[i] *= 2;      // 合并
            line[i+1] = 0;     // 置零
        }
    }
    
    // 3. 再次压缩（填补合并产生的空洞）
    // ...重复步骤1...
}
```

**关键点**：
- 颜色根据数字大小动态变化（冷色调→暖色调）
- 记录历史最高分
- 完美的终端适配

---

## 九、总结

XShell 项目完整实现了一个 Linux Shell 的核心功能，涵盖了：

1. **命令解析**：变量展开、引号处理、管道分割、重定向识别
2. **命令执行**：内置命令分发、外部命令执行、管道实现
3. **交互特性**：Tab 补全、历史记录、行编辑、快捷键
4. **作业控制**：后台执行、作业管理、信号处理
5. **扩展功能**：for 循环、命令链、大括号展开、别名

项目使用纯 C 语言实现，深入运用了 Linux 系统编程的核心知识，包括进程管理、文件 I/O、信号处理、终端控制等，是一个优秀的 Linux 系统编程实践项目。
