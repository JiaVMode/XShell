# XShell - A Linux Shell Implementation in C

<p align="center">
  <img src="https://img.shields.io/badge/Language-C-blue.svg" alt="Language">
  <img src="https://img.shields.io/badge/Platform-Linux-green.svg" alt="Platform">
  <img src="https://img.shields.io/badge/License-MIT-yellow.svg" alt="License">
  <img src="https://img.shields.io/badge/Commands-70+-red.svg" alt="Commands">
</p>

XShell 是一个用 C 语言编写的功能完整的 Linux Shell，包含 70+ 内置命令、图形化 UI 界面和休闲小游戏。

## ✨ 特性

- **70+ 内置命令** - 文件操作、文本处理、系统管理等完整命令集
- **管道与重定向** - 支持 `|`, `>`, `>>`, `<`, `2>`
- **作业控制** - 后台执行 `&`、`jobs`、`fg`、`bg`
- **命令历史** - 上下键浏览、持久化存储
- **Tab 补全** - 命令和文件名自动补全
- **别名系统** - 自定义命令别名
- **图形化 UI** - 基于 TUI 的交互式菜单
- **内置游戏** - 贪吃蛇、俄罗斯方块、2048
- **网页浏览** - 终端内浏览网页

## 🚀 快速开始

### 编译

```bash
git clone git@github.com:JiaVMode/XShell.git
cd XShell
make
```

### 运行

```bash
./xshell
```

### 测试

```bash
./tests/run_tests.sh
```

## 📦 内置命令

### 基础命令
`xpwd` `xcd` `xls` `xecho` `xclear` `quit`

### 文件操作
`xtouch` `xcat` `xrm` `xcp` `xmv` `xstat` `xfile` `xln` `xreadlink` `xrealpath` `xbasename` `xdirname`

### 目录操作
`xmkdir` `xrmdir` `xtree` `xfind` `xdu` `xdf`

### 权限管理
`xchmod` `xchown`

### 文本处理
`xgrep` `xwc` `xhead` `xtail` `xsort` `xuniq` `xdiff` `xcut` `xpaste` `xtr` `xcomm` `xsplit` `xjoin`

### 系统信息
`xuname` `xhostname` `xwhoami` `xdate` `xuptime` `xps`

### 环境变量
`xenv` `xexport` `xunset` `xalias` `xunalias`

### 作业控制
`xjobs` `xfg` `xbg` `xkill`

### 实用工具
`xhelp` `xtype` `xwhich` `xsleep` `xcalc` `xtime` `xsource` `xtec` `xhistory`

### 特色功能
`xui` `xmenu` `xweb` `xsysmon` `xsnake` `xtetris` `x2048`

## 🎮 游戏演示

启动图形界面：
```bash
xui
```

或直接启动游戏：
```bash
xsnake      # 贪吃蛇
xtetris     # 俄罗斯方块
x2048       # 2048
```

## 📂 项目结构

```
XShell/
├── src/
│   ├── main.c              # 主入口
│   ├── xshell.c            # Shell 核心逻辑
│   ├── parser.c            # 命令解析器
│   ├── executor.c          # 命令执行器
│   ├── builtin/            # 70+ 内置命令
│   ├── UI/                 # TUI 界面
│   ├── game/               # 内置游戏
│   └── web/                # 网页浏览器
├── include/                # 头文件
├── tests/                  # 测试脚本
├── Makefile                # 构建配置
└── README.md
```

## 🛠️ 技术栈

- **语言**: C (C99)
- **系统调用**: fork, exec, pipe, dup2, waitpid, signal
- **终端控制**: termios, ANSI 转义序列
- **构建工具**: Make, GCC

## 📄 License

MIT License

## 👤 Author

JiaVMode
