// 头文件保护：防止重复包含
#ifndef BUILTIN_H                                   // 如果 BUILTIN_H 未定义
#define BUILTIN_H                                   // 则定义 BUILTIN_H

// 引入依赖的头文件
#include "parser.h"                                 // Command 结构体定义（内置命令需要解析命令参数）
#include "xshell.h"                                 // ShellContext 结构体定义（内置命令需要访问 Shell 上下文）

// 函数声明
// 内置命令处理函数：每个内置命令对应一个函数
// 函数命名规范：cmd_<命令名>，如 cmd_xpwd, cmd_xcd 等
// 参数说明：
//   - cmd: 解析后的命令对象，包含命令名和参数
//   - ctx: Shell 上下文，包含当前工作目录、运行状态等
// 返回值：0=成功，非0=失败

// xpwd 命令：显示当前工作目录
// 功能：打印当前工作目录的完整路径
// 对应系统命令：pwd（Print Working Directory）
int cmd_xpwd(Command *cmd, ShellContext *ctx);

// xcd 命令：切换工作目录
// 功能：改变当前工作目录
// 对应系统命令：cd
// 用法：xcd <路径> | xcd | xcd .
int cmd_xcd(Command *cmd, ShellContext *ctx);

// xls 命令：列出文件和目录
// 功能：显示指定目录下的所有文件和文件夹
// 对应系统命令：ls
// 用法：xls [路径] （路径为空时显示当前目录）
// 输出格式：Dir: 目录名/  File: 文件名
int cmd_xls(Command *cmd, ShellContext *ctx);

// xecho 命令：输出字符串到标准输出
// 功能：打印参数中的所有字符串，用空格分隔
// 对应系统命令：echo
// 用法：xecho [选项] [字符串...]
// 选项：
//   -n  不输出换行符（默认会输出换行符）
//   -e  启用转义字符解释（\n, \t, \\, \xHH, \0nnn 等）
//   -E  禁用转义字符解释（默认行为）
// 示例：
//   xecho Hello World           → 输出: Hello World\n
//   xecho -n Hello              → 输出: Hello（无换行）
//   xecho -e "Line1\nLine2"     → 输出: Line1 换行 Line2
//   xecho -ne "Tab\there"       → 输出: Tab 制表符 here（无换行）
int cmd_xecho(Command *cmd, ShellContext *ctx);

// xtouch 命令：创建文件或更新时间戳
// 功能：
//   1. 若文件不存在，创建空文件
//   2. 若文件已存在，更新访问时间和修改时间为当前时间
// 对应系统命令：touch
// 用法：xtouch <文件名> [文件名2 ...]
int cmd_xtouch(Command *cmd, ShellContext *ctx);

// xcat 命令：显示文件内容
// 功能：
//   1. 显示文件内容到标准输出
//   2. 支持多个文件（按顺序连接显示）
//   3. 支持 -n 选项（显示行号）
// 对应系统命令：cat
// 用法：xcat [选项] <文件1> [文件2 ...]
int cmd_xcat(Command *cmd, ShellContext *ctx);

// xrm 命令：删除文件或目录
// 功能：
//   1. 删除文件
//   2. 删除目录（需要 -r 选项）
//   3. 支持多个文件/目录
// 对应系统命令：rm
// 用法：xrm [选项] <文件/目录...>
int cmd_xrm(Command *cmd, ShellContext *ctx);

// xcp 命令：复制文件或目录
// 功能：
//   1. 复制文件到文件
//   2. 复制文件到目录
//   3. 递归复制目录（需要 -r 选项）
// 对应系统命令：cp
// 用法：xcp [选项] <源> <目标>
int cmd_xcp(Command *cmd, ShellContext *ctx);

// xmv 命令：移动或重命名文件/目录
// 功能：
//   1. 移动文件/目录
//   2. 重命名文件/目录
//   3. 移动多个文件到目录
// 对应系统命令：mv
// 用法：xmv <源> <目标>
int cmd_xmv(Command *cmd, ShellContext *ctx);

// xhistory 命令：显示命令历史记录
// 功能：
//   1. 显示已执行的命令列表
//   2. 每条命令带序号
// 对应系统命令：history
// 用法：xhistory
int cmd_xhistory(Command *cmd, ShellContext *ctx);

// xtec 命令：从标准输入读取并同时输出到文件和标准输出
// 功能：
//   1. 从标准输入读取数据
//   2. 同时输出到标准输出和文件
//   3. 支持追加模式（-a）
//   4. 支持多个输出文件
// 对应系统命令：tee
// 用法：xtec [-a] <文件名> [文件名2 ...]
// 注意：需要配合管道使用，目前 XShell 还不支持管道
int cmd_xtec(Command *cmd, ShellContext *ctx);

// xmkdir 命令：创建目录
// 功能：
//   1. 创建单个或多个目录
//   2. 支持 -p 选项创建多级目录
// 对应系统命令：mkdir
// 用法：xmkdir [-p] <目录名> [目录名2 ...]
int cmd_xmkdir(Command *cmd, ShellContext *ctx);

// xrmdir 命令：删除空目录
// 功能：
//   1. 删除一个或多个空目录
// 对应系统命令：rmdir
// 用法：xrmdir <目录名> [目录名2 ...]
int cmd_xrmdir(Command *cmd, ShellContext *ctx);

// xln 命令：创建链接
// 功能：
//   1. 创建硬链接或符号链接
//   2. 支持 -s 选项创建符号链接
// 对应系统命令：ln
// 用法：xln [-s] <源文件> <目标>
int cmd_xln(Command *cmd, ShellContext *ctx);

// xchmod 命令：修改文件权限
// 功能：
//   1. 修改文件或目录的访问权限
//   2. 使用八进制数字表示权限
// 对应系统命令：chmod
// 用法：xchmod <权限> <文件名> [文件名2 ...]
int cmd_xchmod(Command *cmd, ShellContext *ctx);

// xchown 命令：修改文件所有者
// 功能：
//   1. 修改文件或目录的所有者和/或所属组
//   2. 支持 user:group 格式
// 对应系统命令：chown
// 用法：xchown [选项] <用户[:组]> <文件>...
int cmd_xchown(Command *cmd, ShellContext *ctx);

// xfind 命令：查找文件
// 功能：
//   1. 在指定路径递归查找文件
//   2. 支持 -name 选项按文件名查找
//   3. 支持通配符 * 和 ?
// 对应系统命令：find
// 用法：xfind <路径> -name <模式>
int cmd_xfind(Command *cmd, ShellContext *ctx);

// xuname 命令：显示系统信息
// 功能：
//   1. 显示内核名称、版本等系统信息
//   2. 支持多种选项（-a, -s, -n, -r, -v, -m）
// 对应系统命令：uname
// 用法：xuname [选项]
int cmd_xuname(Command *cmd, ShellContext *ctx);

// xhostname 命令：显示主机名
// 功能：
//   1. 显示当前系统的主机名
// 对应系统命令：hostname
// 用法：xhostname
int cmd_xhostname(Command *cmd, ShellContext *ctx);

// xwhoami 命令：显示当前用户
// 功能：
//   1. 显示当前登录的用户名
// 对应系统命令：whoami
// 用法：xwhoami
int cmd_xwhoami(Command *cmd, ShellContext *ctx);

// xdate 命令：显示日期时间
// 功能：
//   1. 显示当前日期和时间
//   2. 支持 -u 选项显示UTC时间
// 对应系统命令：date
// 用法：xdate [选项]
int cmd_xdate(Command *cmd, ShellContext *ctx);

// xuptime 命令：显示系统运行时间
// 功能：
//   1. 显示系统已运行的时间
// 对应系统命令：uptime
// 用法：xuptime
int cmd_xuptime(Command *cmd, ShellContext *ctx);

// xps 命令：显示进程信息
// 功能：
//   1. 显示当前Shell进程的基本信息
// 对应系统命令：ps
// 用法：xps
int cmd_xps(Command *cmd, ShellContext *ctx);

// quit 命令：退出 Shell
// 功能：设置 ctx->running = 0，使 Shell 主循环退出
// 注意：这是 Shell 专有命令，不加 x 前缀
int cmd_quit(Command *cmd, ShellContext *ctx);

// ===== 文本处理命令 =====

// xbasename 命令：提取文件名
// 功能：从路径中提取文件名（去除目录部分）
// 对应系统命令：basename
// 用法：xbasename <路径> [后缀]
int cmd_xbasename(Command *cmd, ShellContext *ctx);

// xdirname 命令：提取目录名
// 功能：从路径中提取目录部分（去除文件名）
// 对应系统命令：dirname
// 用法：xdirname <路径>
int cmd_xdirname(Command *cmd, ShellContext *ctx);

// xreadlink 命令：读取符号链接
// 功能：显示符号链接指向的目标路径
// 对应系统命令：readlink
// 用法：xreadlink [选项] <链接文件>
int cmd_xreadlink(Command *cmd, ShellContext *ctx);

// xcut 命令：提取列
// 功能：从文件中提取指定的列（字段）
// 对应系统命令：cut
// 用法：xcut [选项] [文件...]
int cmd_xcut(Command *cmd, ShellContext *ctx);

// xpaste 命令：合并文件行
// 功能：将多个文件的行按列合并
// 对应系统命令：paste
// 用法：xpaste [选项] [文件...]
int cmd_xpaste(Command *cmd, ShellContext *ctx);

// xtr 命令：字符转换
// 功能：转换或删除字符
// 对应系统命令：tr
// 用法：xtr [选项] <字符集1> [字符集2]
int cmd_xtr(Command *cmd, ShellContext *ctx);

// xcomm 命令：比较排序文件
// 功能：比较两个已排序的文件，显示共同行和独有行
// 对应系统命令：comm
// 用法：xcomm [选项] <文件1> <文件2>
int cmd_xcomm(Command *cmd, ShellContext *ctx);

// xstat 命令：显示文件详细信息
// 功能：显示文件的详细统计信息（大小、权限、时间等）
// 对应系统命令：stat
// 用法：xstat [选项] <文件>...
int cmd_xstat(Command *cmd, ShellContext *ctx);

// xfile 命令：显示文件类型
// 功能：检测并显示文件类型
// 对应系统命令：file
// 用法：xfile [选项] <文件>...
int cmd_xfile(Command *cmd, ShellContext *ctx);

// xdu 命令：显示目录大小
// 功能：显示目录及其子目录的磁盘使用量
// 对应系统命令：du
// 用法：xdu [选项] [目录]...
int cmd_xdu(Command *cmd, ShellContext *ctx);

// xdf 命令：显示磁盘空间
// 功能：显示文件系统的磁盘空间使用情况
// 对应系统命令：df
// 用法：xdf [选项] [文件系统]...
int cmd_xdf(Command *cmd, ShellContext *ctx);

// xsplit 命令：分割文件
// 功能：将大文件分割成多个小文件
// 对应系统命令：split
// 用法：xsplit [选项] <文件> [前缀]
int cmd_xsplit(Command *cmd, ShellContext *ctx);

// xjoin 命令：连接文件
// 功能：基于共同字段连接两个文件（类似SQL JOIN）
// 对应系统命令：join
// 用法：xjoin [选项] <文件1> <文件2>
int cmd_xjoin(Command *cmd, ShellContext *ctx);

// xrealpath 命令：显示绝对路径
// 功能：显示文件的绝对路径（解析所有符号链接）
// 对应系统命令：realpath
// 用法：xrealpath [选项] <文件>...
int cmd_xrealpath(Command *cmd, ShellContext *ctx);

// xmenu 命令：交互式菜单系统
// 功能：显示交互式菜单，支持键盘导航
// 对应系统命令：无（XShell 特有功能）
// 用法：xmenu [选项]
int cmd_xmenu(Command *cmd, ShellContext *ctx);


// xdiff 命令：比较文件差异
// 功能：
//   1. 比较两个文件的差异，显示不同的行
//   2. 支持 -u 选项（统一格式输出）
// 对应系统命令：diff
// 用法：xdiff [选项] <文件1> <文件2>
int cmd_xdiff(Command *cmd, ShellContext *ctx);

// xgrep 命令：在文件中搜索文本
// 功能：
//   1. 搜索文件中包含指定模式的行
//   2. 支持忽略大小写、显示行号、反向匹配、统计匹配数
// 对应系统命令：grep
// 用法：xgrep [选项] <pattern> <file>...
int cmd_xgrep(Command *cmd, ShellContext *ctx);

// xwc 命令：统计文件的行数、字数和字节数
// 功能：
//   1. 统计文件的行数、字数、字节数
//   2. 支持只显示特定统计项
// 对应系统命令：wc
// 用法：xwc [选项] [file]...
int cmd_xwc(Command *cmd, ShellContext *ctx);

// xhead 命令：显示文件的前 N 行
// 功能：
//   1. 显示文件开头的指定行数（默认10行）
// 对应系统命令：head
// 用法：xhead [-n N] [file]...
int cmd_xhead(Command *cmd, ShellContext *ctx);

// xtail 命令：显示文件的后 N 行
// 功能：
//   1. 显示文件末尾的指定行数（默认10行）
// 对应系统命令：tail
// 用法：xtail [-n N] [file]...
int cmd_xtail(Command *cmd, ShellContext *ctx);

// xsort 命令：排序文件内容
// 功能：
//   1. 对文件行进行排序
//   2. 支持逆序排序、数值排序、去重
// 对应系统命令：sort
// 用法：xsort [选项] [file]...
int cmd_xsort(Command *cmd, ShellContext *ctx);

// xuniq 命令：去除文件中的重复行
// 功能：
//   1. 过滤相邻的重复行
//   2. 支持统计重复次数、只显示重复/唯一行
// 对应系统命令：uniq
// 用法：xuniq [选项] [file]
int cmd_xuniq(Command *cmd, ShellContext *ctx);

// ===== 环境变量和别名命令 =====

// xenv 命令：显示所有环境变量
// 功能：
//   1. 显示当前Shell的所有环境变量
// 对应系统命令：env, printenv
// 用法：xenv
int cmd_xenv(Command *cmd, ShellContext *ctx);

// xexport 命令：设置环境变量
// 功能：
//   1. 设置或导出环境变量
//   2. 支持 VAR=value 格式
//   3. 支持 -p 显示所有导出变量
// 对应系统命令：export
// 用法：xexport [VAR=value] | xexport [-p]
int cmd_xexport(Command *cmd, ShellContext *ctx);

// xunset 命令：删除环境变量
// 功能：
//   1. 从环境中删除指定的变量
// 对应系统命令：unset
// 用法：xunset VAR [VAR2 ...]
int cmd_xunset(Command *cmd, ShellContext *ctx);

// xalias 命令：设置和显示命令别名
// 功能：
//   1. 创建命令别名
//   2. 显示所有别名或指定别名
// 对应系统命令：alias
// 用法：xalias [name='value'] | xalias [name]
int cmd_xalias(Command *cmd, ShellContext *ctx);

// xunalias 命令：删除命令别名
// 功能：
//   1. 删除指定的别名
// 对应系统命令：unalias
// 用法：xunalias name [name2 ...]
int cmd_xunalias(Command *cmd, ShellContext *ctx);

// ===== 实用工具命令 =====

// xclear 命令：清屏
// 功能：
//   1. 清除终端屏幕内容
// 对应系统命令：clear
// 用法：xclear
int cmd_xclear(Command *cmd, ShellContext *ctx);

// xhelp 命令：显示帮助信息
// 功能：
//   1. 显示所有命令的列表
//   2. 显示特定命令的详细帮助
// 对应系统命令：help
// 用法：xhelp [command]
int cmd_xhelp(Command *cmd, ShellContext *ctx);

// xtype 命令：显示命令类型
// 功能：
//   1. 显示命令是内置命令、别名还是外部命令
// 对应系统命令：type
// 用法：xtype <command>...
int cmd_xtype(Command *cmd, ShellContext *ctx);

// xwhich 命令：显示命令路径
// 功能：
//   1. 在PATH中搜索命令的完整路径
// 对应系统命令：which
// 用法：xwhich <command>...
int cmd_xwhich(Command *cmd, ShellContext *ctx);

// xsleep 命令：休眠指定秒数
// 功能：
//   1. 暂停执行指定的秒数
// 对应系统命令：sleep
// 用法：xsleep <seconds>
int cmd_xsleep(Command *cmd, ShellContext *ctx);

// xcalc 命令：简单计算器
// 功能：
//   1. 计算简单的数学表达式
//   2. 支持 +, -, *, /, % 运算
// 对应系统命令：bc, expr
// 用法：xcalc <expression>
int cmd_xcalc(Command *cmd, ShellContext *ctx);

// xtree 命令：树形显示目录结构
// 功能：
//   1. 以树形结构递归显示目录内容
//   2. 支持限制显示深度
// 对应系统命令：tree
// 用法：xtree [path] [-L level]
int cmd_xtree(Command *cmd, ShellContext *ctx);

// ===== 进程和任务管理命令 =====

// xsource 命令：执行脚本文件
// 功能：
//   1. 读取脚本文件并逐行执行命令
// 对应系统命令：source, .
// 用法：xsource <file>
int cmd_xsource(Command *cmd, ShellContext *ctx);

// xtime 命令：测量命令执行时间
// 功能：
//   1. 执行命令并测量其执行时间
// 对应系统命令：time
// 用法：xtime <command> [args...]
int cmd_xtime(Command *cmd, ShellContext *ctx);

// xkill 命令：终止进程
// 功能：
//   1. 向指定进程发送信号（默认SIGTERM）
// 对应系统命令：kill
// 用法：xkill <pid> [-s signal]
int cmd_xkill(Command *cmd, ShellContext *ctx);

// xjobs 命令：显示后台任务
// 功能：
//   1. 显示当前Shell的所有后台任务
// 对应系统命令：jobs
// 用法：xjobs
int cmd_xjobs(Command *cmd, ShellContext *ctx);

// xfg 命令：将后台任务调到前台
// 功能：
//   1. 将指定的后台任务调到前台执行
// 对应系统命令：fg
// 用法：xfg [job_id]
int cmd_xfg(Command *cmd, ShellContext *ctx);

// xbg 命令：将任务放到后台
// 功能：
//   1. 将指定的已停止任务放到后台继续执行
// 对应系统命令：bg
// 用法：xbg [job_id]
int cmd_xbg(Command *cmd, ShellContext *ctx);

// xsysmon 命令：系统监控
// 功能：
//   1. 实时显示 CPU、内存、磁盘使用情况
//   2. 显示系统负载和运行时间
// 对应系统命令：top (简化版)
// 用法：xsysmon
int cmd_xsysmon(struct Command *cmd, struct ShellContext *ctx);
void xsysmon(void);

#endif // BUILTIN_H  // 头文件保护结束

