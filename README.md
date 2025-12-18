# XShell - 自定义 Linux Shell 实现

XShell 是一个功能完整的 Linux Shell 实现，使用纯 C 语言从零构建，支持 60+ 内置命令、管道、重定向、命令链、Tab 补全等高级特性。

## ✨ 特性亮点

- **60+ 内置命令**：涵盖文件操作、文本处理、系统管理等
- **命令管道**：支持 `|` 管道连接多个命令
- **I/O 重定向**：支持 `>`、`>>`、`<`、`2>` 重定向
- **命令链**：支持 `&&` 和 `||` 条件执行
- **Tab 补全**：智能路径和命令补全
- **历史记录**：支持上下键浏览历史命令
- **别名系统**：支持自定义命令别名
- **彩色输出**：终端彩色显示支持
- **错误日志**：双轨错误记录（stderr + 文件）

## 📁 项目结构

```
XShell/
├── src/                    # 源代码
│   ├── main.c              # 程序入口
│   ├── xshell.c            # Shell 主循环
│   ├── parser.c            # 命令行解析器
│   ├── executor.c          # 命令执行器
│   ├── input.c             # 输入处理（Tab补全、行编辑）
│   ├── history.c           # 历史记录管理
│   ├── completion.c        # 智能补全实现
│   ├── alias.c             # 别名系统
│   ├── utils.c             # 工具函数
│   └── builtin/            # 内置命令实现（64个）
├── include/                # 头文件
├── tests/                  # 测试脚本
├── Makefile                # 构建配置
└── README.md               # 本文档
```

## 🛠️ 编译与运行

### 环境要求
- Linux 操作系统
- GCC 编译器（支持 C99）
- Make 构建工具

### 编译
```bash
make clean && make
```

### 运行
```bash
./xshell
```

### 调试模式
```bash
make debug
```

## 📖 内置命令列表

### 文件与目录操作
| 命令 | 说明 | 对应系统命令 |
|------|------|-------------|
| `xpwd` | 显示当前目录 | `pwd` |
| `xcd` | 切换目录 | `cd` |
| `xls` | 列出目录内容 | `ls` |
| `xmkdir` | 创建目录 | `mkdir` |
| `xrmdir` | 删除空目录 | `rmdir` |
| `xrm` | 删除文件/目录 | `rm` |
| `xcp` | 复制文件/目录 | `cp` |
| `xmv` | 移动/重命名 | `mv` |
| `xtouch` | 创建文件 | `touch` |
| `xln` | 创建链接 | `ln` |
| `xstat` | 文件状态 | `stat` |
| `xfind` | 查找文件 | `find` |
| `xtree` | 目录树显示 | `tree` |
| `xchmod` | 修改权限 | `chmod` |
| `xchown` | 修改所有者 | `chown` |

### 文本处理
| 命令 | 说明 | 对应系统命令 |
|------|------|-------------|
| `xcat` | 查看文件 | `cat` |
| `xecho` | 输出文本 | `echo` |
| `xgrep` | 文本搜索 | `grep` |
| `xhead` | 显示开头 | `head` |
| `xtail` | 显示结尾 | `tail` |
| `xwc` | 统计行数 | `wc` |
| `xsort` | 排序 | `sort` |
| `xuniq` | 去重 | `uniq` |
| `xcut` | 列提取 | `cut` |
| `xpaste` | 列合并 | `paste` |
| `xtr` | 字符替换 | `tr` |
| `xdiff` | 文件比较 | `diff` |

### 系统与进程
| 命令 | 说明 | 对应系统命令 |
|------|------|-------------|
| `xps` | 进程列表 | `ps` |
| `xkill` | 终止进程 | `kill` |
| `xjobs` | 后台作业 | `jobs` |
| `xfg` | 前台运行 | `fg` |
| `xbg` | 后台运行 | `bg` |
| `xenv` | 环境变量 | `env` |
| `xexport` | 导出变量 | `export` |
| `xunset` | 删除变量 | `unset` |
| `xuname` | 系统信息 | `uname` |
| `xuptime` | 运行时间 | `uptime` |
| `xhostname` | 主机名 | `hostname` |
| `xwhoami` | 当前用户 | `whoami` |
| `xdate` | 日期时间 | `date` |

### Shell 功能
| 命令 | 说明 |
|------|------|
| `xhistory` | 显示命令历史 |
| `xalias` | 定义别名 |
| `xunalias` | 删除别名 |
| `xsource` | 执行脚本 |
| `xhelp` | 帮助信息 |
| `xclear` | 清屏 |
| `quit` | 退出 Shell |

### 其他工具
| 命令 | 说明 |
|------|------|
| `xcalc` | 计算器 |
| `xmenu` | 交互式菜单 |
| `xtime` | 命令计时 |
| `xsleep` | 延时 |
| `xwhich` | 查找命令 |
| `xtype` | 命令类型 |

## 🔧 高级用法

### 管道
```bash
xls -l | xgrep ".c" | xwc -l
```

### 重定向
```bash
xecho "Hello" > output.txt      # 覆盖写入
xecho "World" >> output.txt     # 追加写入
xcat < input.txt                # 输入重定向
xls /nonexistent 2> error.log   # 错误重定向
```

### 命令链
```bash
xcd /tmp && xpwd                # 成功后执行
xls /nonexistent || xecho "Failed"  # 失败后执行
```

### 别名
```bash
xalias ll="xls -l"
xalias la="xls -la"
ll                              # 使用别名
```

### For 循环
```bash
for i in 1 2 3; do xecho $i; done
for f in *.c; do xwc -l $f; done
```

## 📊 架构设计

```
┌─────────────────────────────────────────────────────┐
│                    用户输入                          │
└─────────────────────┬───────────────────────────────┘
                      ▼
┌─────────────────────────────────────────────────────┐
│  Input Module (input.c, completion.c, history.c)    │
│  • Tab 补全  • 行编辑  • 历史导航                    │
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
│              60+ 内置命令实现                        │
└─────────────────────────────────────────────────────┘
```

## 🧪 测试

运行测试脚本：
```bash
cd tests
./run_tests.sh
```

---
