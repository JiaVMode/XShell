# XShell 功能测试文档

## 测试说明

本文档包含 XShell 所有内置命令的测试用例，每个命令提供 3 个测试命令。

**测试环境准备：**
```bash
cd /path/to/XShell
make clean && make
./xshell
```

---

## 一、基础命令

### 1. xpwd - 显示当前工作目录
```bash
# 测试1：显示当前目录
xpwd

# 测试2：显示帮助信息
xpwd --help

# 测试3：切换目录后再显示
xcd /tmp && xpwd
```

### 2. xcd - 切换目录
```bash
# 测试1：切换到根目录
xcd /

# 测试2：切换到用户主目录
xcd ~

# 测试3：切换到上一级目录
xcd ..
```

### 3. xls - 列出文件和目录
```bash
# 测试1：列出当前目录
xls

# 测试2：详细列表模式
xls -la

# 测试3：人性化大小显示
xls -lh /var/log
```

### 4. xecho - 输出字符串
```bash
# 测试1：简单输出
xecho Hello World

# 测试2：不换行输出
xecho -n "Hello " && xecho World

# 测试3：转义字符
xecho -e "Line1\nLine2\tTabbed"
```

### 5. xclear - 清屏
```bash
# 测试1：清除屏幕
xclear

# 测试2：显示帮助
xclear --help

# 测试3：清屏后执行命令
xclear && xpwd
```

### 6. quit - 退出 Shell
```bash
# 测试1：显示帮助（不退出）
quit --help

# 测试2：正常退出（在测试最后执行）
# quit

# 测试3：使用 Ctrl+D 退出（按键测试）
# 按 Ctrl+D
```

---

## 二、文件操作

### 7. xtouch - 创建文件
```bash
# 测试1：创建单个文件
xtouch /tmp/test1.txt

# 测试2：创建多个文件
xtouch /tmp/test2.txt /tmp/test3.txt

# 测试3：更新已存在文件的时间戳
xtouch /tmp/test1.txt && xls -l /tmp/test1.txt
```

### 8. xcat - 显示文件内容
```bash
# 测试1：显示文件内容
xecho "Hello World" > /tmp/cat_test.txt && xcat /tmp/cat_test.txt

# 测试2：显示行号
xcat -n /etc/passwd | xhead -5

# 测试3：显示帮助
xcat --help
```

### 9. xrm - 删除文件
```bash
# 测试1：删除单个文件
xtouch /tmp/rm_test.txt && xrm /tmp/rm_test.txt

# 测试2：递归删除目录
xmkdir -p /tmp/rm_dir/sub && xrm -r /tmp/rm_dir

# 测试3：删除多个文件
xtouch /tmp/a.txt /tmp/b.txt && xrm /tmp/a.txt /tmp/b.txt
```

### 10. xcp - 复制文件
```bash
# 测试1：复制单个文件
xecho "content" > /tmp/cp_src.txt && xcp /tmp/cp_src.txt /tmp/cp_dst.txt

# 测试2：递归复制目录
xmkdir -p /tmp/cp_dir/sub && xcp -r /tmp/cp_dir /tmp/cp_dir_copy

# 测试3：复制并验证
xcp /etc/hostname /tmp/hostname_copy && xcat /tmp/hostname_copy
```

### 11. xmv - 移动/重命名文件
```bash
# 测试1：重命名文件
xtouch /tmp/mv_old.txt && xmv /tmp/mv_old.txt /tmp/mv_new.txt

# 测试2：移动文件到目录
xmkdir -p /tmp/mv_dir && xtouch /tmp/mv_file.txt && xmv /tmp/mv_file.txt /tmp/mv_dir/

# 测试3：移动并验证
xmv /tmp/mv_new.txt /tmp/mv_final.txt && xls /tmp/mv_final.txt
```

### 12. xstat - 显示文件信息
```bash
# 测试1：显示文件详细信息
xstat /etc/passwd

# 测试2：显示目录信息
xstat /tmp

# 测试3：显示帮助
xstat --help
```

### 13. xfile - 显示文件类型
```bash
# 测试1：检测文本文件
xfile /etc/passwd

# 测试2：检测二进制文件
xfile /bin/ls

# 测试3：检测目录
xfile /tmp
```

### 14. xreadlink - 读取符号链接
```bash
# 测试1：创建并读取符号链接
xln -s /etc/passwd /tmp/passwd_link && xreadlink /tmp/passwd_link

# 测试2：显示帮助
xreadlink --help

# 测试3：处理普通文件
xreadlink /etc/passwd
```

### 15. xrealpath - 显示绝对路径
```bash
# 测试1：显示相对路径的绝对路径
xrealpath .

# 测试2：显示文件绝对路径
xrealpath /tmp/../etc/passwd

# 测试3：显示帮助
xrealpath --help
```

### 16. xbasename - 提取文件名
```bash
# 测试1：提取文件名
xbasename /path/to/file.txt

# 测试2：去除后缀
xbasename /path/to/file.txt .txt

# 测试3：处理目录路径
xbasename /var/log/
```

### 17. xdirname - 提取目录名
```bash
# 测试1：提取目录部分
xdirname /path/to/file.txt

# 测试2：处理根目录
xdirname /file.txt

# 测试3：显示帮助
xdirname --help
```

---

## 三、目录操作

### 18. xmkdir - 创建目录
```bash
# 测试1：创建单个目录
xmkdir /tmp/mkdir_test

# 测试2：递归创建目录
xmkdir -p /tmp/a/b/c/d

# 测试3：创建多个目录
xmkdir /tmp/dir1 /tmp/dir2 /tmp/dir3
```

### 19. xrmdir - 删除空目录
```bash
# 测试1：删除空目录
xmkdir /tmp/empty_dir && xrmdir /tmp/empty_dir

# 测试2：尝试删除非空目录（应失败）
xmkdir -p /tmp/nonempty/sub && xrmdir /tmp/nonempty

# 测试3：删除多个目录
xmkdir /tmp/e1 /tmp/e2 && xrmdir /tmp/e1 /tmp/e2
```

### 20. xtree - 树形显示目录
```bash
# 测试1：显示当前目录树
xtree

# 测试2：限制深度
xtree -L 2 /etc

# 测试3：显示指定目录
xtree /var/log
```

### 21. xfind - 查找文件
```bash
# 测试1：按名称查找
xfind /etc -name "*.conf"

# 测试2：查找当前目录
xfind . -name "*.c"

# 测试3：显示帮助
xfind --help

# 测试4：查找不存在的文件（错误处理）
xfind /tmp -name "nonexistent_file_pattern_*"

# 测试5：在深层目录中查找
xmkdir -p /tmp/deep/a/b/c && xtouch /tmp/deep/a/b/c/target.txt && xfind /tmp/deep -name "target.txt"
```

### 22. xdu - 显示目录大小
```bash
# 测试1：显示目录大小
xdu /tmp

# 测试2：人性化显示
xdu -h /var

# 测试3：显示帮助
xdu --help
```

### 23. xdf - 显示磁盘空间
```bash
# 测试1：显示所有磁盘
xdf

# 测试2：人性化显示
xdf -h

# 测试3：显示帮助
xdf --help
```

---

## 四、权限与链接

### 24. xchmod - 修改文件权限
```bash
# 测试1：八进制模式
xtouch /tmp/chmod_test && xchmod 755 /tmp/chmod_test

# 测试2：符号模式
xchmod u+x /tmp/chmod_test

# 测试3：验证权限
xchmod 644 /tmp/chmod_test && xls -l /tmp/chmod_test

# 测试4：混合符号模式
xchmod u+x,g-w,o=r /tmp/chmod_test && xls -l /tmp/chmod_test

# 测试5：递归修改权限（假设支持 -R）
xmkdir -p /tmp/perm_dir/sub && xtouch /tmp/perm_dir/file.txt && xchmod -R 777 /tmp/perm_dir && xls -l /tmp/perm_dir/file.txt
```

### 25. xchown - 修改文件所有者
```bash
# 测试1：修改所有者（需要 root）
xchown root /tmp/chmod_test

# 测试2：修改所有者和组
xchown root:root /tmp/chmod_test

# 测试3：显示帮助
xchown --help
```

### 26. xln - 创建链接
```bash
# 测试1：创建硬链接
xtouch /tmp/ln_src.txt && xln /tmp/ln_src.txt /tmp/ln_hard.txt

# 测试2：创建符号链接
xln -s /tmp/ln_src.txt /tmp/ln_sym.txt

# 测试3：验证链接
xls -l /tmp/ln_*.txt
```

---

## 五、文本处理

### 27. xgrep - 搜索文本
```bash
# 测试1：简单搜索
xgrep root /etc/passwd

# 测试2：忽略大小写
xgrep -i ROOT /etc/passwd

# 测试3：显示行号
xgrep -n bash /etc/passwd

# 测试4：反向匹配
xgrep -v nologin /etc/passwd | xhead -5

# 测试5：统计匹配行数
xgrep -c sh /etc/passwd
```

### 28. xwc - 统计行数/字数/字节数
```bash
# 测试1：统计所有信息
xwc /etc/passwd

# 测试2：只统计行数
xwc -l /etc/passwd

# 测试3：统计多个文件
xwc /etc/passwd /etc/group
```

### 29. xhead - 显示文件前N行
```bash
# 测试1：显示前10行（默认）
xhead /etc/passwd

# 测试2：显示前5行
xhead -n 5 /etc/passwd

# 测试3：显示帮助
xhead --help
```

### 30. xtail - 显示文件后N行
```bash
# 测试1：显示后10行（默认）
xtail /etc/passwd

# 测试2：显示后3行
xtail -n 3 /etc/passwd

# 测试3：显示帮助
xtail --help
```

### 31. xsort - 排序文件内容
```bash
# 测试1：正序排序
xecho -e "c\na\nb" > /tmp/sort_test.txt && xsort /tmp/sort_test.txt

# 测试2：逆序排序
xsort -r /tmp/sort_test.txt

# 测试3：数值排序
xecho -e "10\n2\n1\n20" > /tmp/num.txt && xsort -n /tmp/num.txt

# 测试4：去重排序
xecho -e "b\na\nb\nc" > /tmp/dup.txt && xsort -u /tmp/dup.txt

# 测试5：逆序数值排序
xecho -e "10\n2\n1\n20" > /tmp/num.txt && xsort -nr /tmp/num.txt
```

### 32. xuniq - 去除重复行
```bash
# 测试1：去除重复行
xecho -e "a\na\nb\nb\nc" > /tmp/uniq_test.txt && xsort /tmp/uniq_test.txt | xuniq

# 测试2：显示重复次数
xsort /tmp/uniq_test.txt | xuniq -c

# 测试3：只显示重复行
xsort /tmp/uniq_test.txt | xuniq -d
```

### 33. xdiff - 比较文件差异
```bash
# 测试1：比较两个文件
xecho "line1" > /tmp/diff1.txt && xecho "line2" > /tmp/diff2.txt && xdiff /tmp/diff1.txt /tmp/diff2.txt

# 测试2：统一格式输出
xdiff -u /tmp/diff1.txt /tmp/diff2.txt

# 测试3：显示帮助
xdiff --help
```

### 34. xcut - 提取列
```bash
# 测试1：提取第一列
xecho "a:b:c" | xcut -d: -f1

# 测试2：提取多列
xcut -d: -f1,3 /etc/passwd | xhead -3

# 测试3：显示帮助
xcut --help
```

### 35. xpaste - 合并文件行
```bash
# 测试1：合并两个文件
xecho -e "1\n2\n3" > /tmp/p1.txt && xecho -e "a\nb\nc" > /tmp/p2.txt && xpaste /tmp/p1.txt /tmp/p2.txt

# 测试2：显示帮助
xpaste --help

# 测试3：使用自定义分隔符
xpaste -d, /tmp/p1.txt /tmp/p2.txt
```

### 36. xtr - 字符转换
```bash
# 测试1：大写转小写
xecho "HELLO" | xtr A-Z a-z

# 测试2：删除字符
xecho "hello123world" | xtr -d 0-9

# 测试3：显示帮助
xtr --help
```

---

## 六、系统信息

### 37. xuname - 系统信息
```bash
# 测试1：显示全部信息
xuname -a

# 测试2：显示内核版本
xuname -r

# 测试3：显示机器类型
xuname -m
```

### 38. xhostname - 主机名
```bash
# 测试1：显示主机名
xhostname

# 测试2：显示帮助
xhostname --help

# 测试3：与 xuname -n 对比
xuname -n
```

### 39. xwhoami - 当前用户
```bash
# 测试1：显示当前用户
xwhoami

# 测试2：显示帮助
xwhoami --help

# 测试3：与环境变量对比
xenv | xgrep USER
```

### 40. xdate - 日期时间
```bash
# 测试1：显示当前时间
xdate

# 测试2：显示 UTC 时间
xdate -u

# 测试3：显示帮助
xdate --help
```

### 41. xuptime - 系统运行时间
```bash
# 测试1：显示运行时间
xuptime

# 测试2：显示帮助
xuptime --help

# 测试3：重复执行
xuptime && xsleep 1 && xuptime
```

### 42. xps - 进程信息
```bash
# 测试1：显示所有进程
xps

# 测试2：显示帮助
xps --help

# 测试3：搜索特定进程
xps | xgrep xshell
```

---

## 七、环境变量和别名

### 43. xenv - 显示环境变量
```bash
# 测试1：显示所有环境变量
xenv

# 测试2：搜索 PATH
xenv | xgrep PATH

# 测试3：显示帮助
xenv --help
```

### 44. xexport - 设置环境变量
```bash
# 测试1：设置变量
xexport MYVAR=hello

# 测试2：显示变量
xexport MYVAR

# 测试3：验证设置
xexport TEST=world && xenv | xgrep TEST
```

### 45. xunset - 删除环境变量
```bash
# 测试1：删除变量
xexport DELME=temp && xunset DELME

# 测试2：验证删除
xenv | xgrep DELME

# 测试3：显示帮助
xunset --help
```

### 46. xalias - 设置别名
```bash
# 测试1：设置别名
xalias ll='xls -la'

# 测试2：显示所有别名
xalias

# 测试3：使用别名
ll
```

### 47. xunalias - 删除别名
```bash
# 测试1：删除别名
xunalias ll

# 测试2：验证删除
xalias ll

# 测试3：显示帮助
xunalias --help
```

---

## 八、进程与作业控制

### 48. xkill - 终止进程
```bash
# 测试1：显示帮助
xkill --help

# 测试2：发送信号（使用无效 PID 测试错误处理）
xkill 99999

# 测试3：使用信号名
xkill -s TERM 99999
```

### 49. xjobs - 显示后台任务
```bash
# 测试1：显示后台任务
xjobs

# 测试2：创建后台任务后显示
xsleep 10 & 
xjobs

# 测试3：显示帮助
xjobs --help

# 测试4：运行并立即查看
xsleep 5 &
xjobs

# 测试5：多个后台任务
xsleep 10 &
xsleep 15 &
xjobs
```

### 50. xfg - 将后台任务调到前台
```bash
# 测试1：显示帮助
xfg --help

# 测试2：无后台任务时测试
xfg

# 测试3：将任务调到前台
xsleep 5 &
xfg 1
```

### 51. xbg - 将任务放到后台
```bash
# 测试1：显示帮助
xbg --help

# 测试2：无停止任务时测试
xbg

# 测试3：组合测试
xjobs
```

---

## 九、实用工具

### 52. xhelp - 显示帮助
```bash
# 测试1：显示所有命令
xhelp

# 测试2：显示特定命令帮助
xhelp xls

# 测试3：显示 xhelp 自身帮助
xhelp --help
```

### 53. xtype - 显示命令类型
```bash
# 测试1：检查内置命令
xtype xls

# 测试2：检查外部命令
xtype ls

# 测试3：检查不存在的命令
xtype nonexistent_cmd
```

### 54. xwhich - 显示命令路径
```bash
# 测试1：查找命令路径
xwhich ls

# 测试2：查找多个命令
xwhich ls cat grep

# 测试3：查找不存在的命令
xwhich nonexistent_cmd
```

### 55. xsleep - 休眠
```bash
# 测试1：休眠1秒
xsleep 1

# 测试2：休眠3秒
xsleep 3

# 测试3：显示帮助
xsleep --help
```

### 56. xcalc - 简单计算器
```bash
# 测试1：加法
xcalc "1 + 2"

# 测试2：复杂表达式
xcalc "(10 + 5) * 2"

# 测试3：除法
xcalc "100 / 4"
```

### 57. xtime - 测量命令执行时间
```bash
# 测试1：测量 xls 时间
xtime xls

# 测试2：测量 xsleep 时间
xtime xsleep 1

# 测试3：显示帮助
xtime --help
```

### 58. xsource - 执行脚本
```bash
# 测试1：创建并执行脚本
xecho -e "xpwd\nxdate" > /tmp/test.sh && xsource /tmp/test.sh

# 测试2：显示帮助
xsource --help

# 测试3：执行不存在的脚本
xsource /tmp/nonexistent.sh
```

### 59. xtec - Tee 功能
```bash
# 测试1：输出到文件和屏幕
xecho "hello" | xtec /tmp/tec_out.txt

# 测试2：追加模式
xecho "world" | xtec -a /tmp/tec_out.txt

# 测试3：验证输出
xcat /tmp/tec_out.txt
```

### 60. xhistory - 命令历史
```bash
# 测试1：显示历史记录
xhistory

# 测试2：显示帮助
xhistory --help

# 测试3：执行几个命令后显示
xpwd && xdate && xhistory
```

---

## 十、特色功能

### 61. xui - 交互式 UI 界面
```bash
# 测试1：启动 UI
xui

# 测试2：显示帮助
xui --help

# 测试3：在 UI 中按 Q 退出
# 启动后按 Q 键退出
```

### 62. xmenu - 交互式菜单
```bash
# 测试1：启动菜单
xmenu

# 测试2：显示帮助
xmenu --help

# 测试3：使用方向键选择后按 Q 退出
# 启动后操作
```

### 63. xweb - 网页浏览器
```bash
# 测试1：启动浏览器
xweb

# 测试2：直接获取网页
xweb http://example.com

# 测试3：显示帮助
xweb --help
```

### 64. xsysmon - 系统监控
```bash
# 测试1：启动系统监控
xsysmon

# 测试2：显示帮助
xsysmon --help

# 测试3：按 Q 退出
# 启动后按 Q 键退出
```

### 65. xsnake - 贪吃蛇游戏
```bash
# 测试1：启动游戏
xsnake

# 测试2：显示帮助
xsnake --help

# 测试3：玩游戏后按 Q 退出
# 使用方向键控制
```

### 66. xtetris - 俄罗斯方块
```bash
# 测试1：启动游戏
xtetris

# 测试2：显示帮助
xtetris --help

# 测试3：玩游戏
# 使用方向键控制
```

### 67. x2048 - 2048 游戏
```bash
# 测试1：启动游戏
x2048

# 测试2：显示帮助
x2048 --help

# 测试3：玩游戏
# 使用方向键控制
```

---

## 十一、后台任务功能

### 68. 后台执行命令
```bash
# 测试1：在后台执行命令
xsleep 10 &

# 测试2：查看后台任务
xjobs

# 测试3：等待后台任务完成
xsleep 2 &
xjobs
xsleep 3
xjobs
```

---

## 十二、信号处理测试

### 69. Ctrl+C 测试
```bash
# 测试1：在命令行按 Ctrl+C（不应退出 Shell）
# 按 Ctrl+C

# 测试2：在执行命令时按 Ctrl+C
xsleep 10
# 按 Ctrl+C 中断

# 测试3：在交互程序中按 Ctrl+C
xweb
# 按 Ctrl+C
```

---

## 十三、高级 Shell 特性测试

### 70. 管道操作 (|)
```bash
# 测试1：简单管道
xls /etc | xgrep conf

# 测试2：多重管道
xcat /etc/passwd | xhead -10 | xtail -5

# 测试3：管道与内置/外部命令混合
xjobs | xgrep Running | xwc -l
```

### 71. 标准输出重定向 (>)
```bash
# 测试1：将命令输出写入文件
xecho "Hello File" > /tmp/out_test.txt && xcat /tmp/out_test.txt

# 测试2：覆盖文件内容
xecho "New Content" > /tmp/out_test.txt && xcat /tmp/out_test.txt

# 测试3：外部命令重定向
/bin/ls -l / > /tmp/ls_out.txt && xhead /tmp/ls_out.txt
```

### 72. 追加重定向 (>>)
```bash
# 测试1：追加内容到文件
xecho "Line 1" > /tmp/append.txt && xecho "Line 2" >> /tmp/append.txt

# 测试2：验证追加结果
xcat /tmp/append.txt

# 测试3：多次追加
xdate >> /tmp/log.txt && xsleep 1 && xdate >> /tmp/log.txt && xcat /tmp/log.txt
```

### 73. 标准错误重定向 (2>)
```bash
# 测试1：重定向错误信息
xls /nonexistent_dir 2> /tmp/err.txt

# 测试2：验证错误文件内容
xcat /tmp/err.txt

# 测试3：同时重定向输出和错误 (ls exist nonexist > out 2> err)
xls /tmp /nonexistent_dir > /tmp/std_out.txt 2> /tmp/std_err.txt
```

### 74. 外部程序调用
```bash
# 测试1：调用系统 ls
/bin/ls -F /

# 测试2：调用 vim (交互式程序测试，按 :q 退出)
# /usr/bin/vim /tmp/test_vim.txt

# 测试3：复杂外部命令参数
/bin/grep -r "root" /etc/passwd
```

---

## 测试完成清理

```bash
# 清理测试文件
xrm -r /tmp/test*.txt /tmp/mkdir_test /tmp/a /tmp/dir* /tmp/cp_* /tmp/mv_* /tmp/ln_* /tmp/sort_* /tmp/uniq_* /tmp/diff* /tmp/p*.txt /tmp/tec_* /tmp/chmod_* 2>/dev/null

# 退出 Shell
quit
```

---

## 测试结果记录

| 序号 | 命令 | 测试1 | 测试2 | 测试3 | 状态 |
|------|------|-------|-------|-------|------|
| 1 | xpwd | ☐ | ☐ | ☐ | |
| 2 | xcd | ☐ | ☐ | ☐ | |
| 3 | xls | ☐ | ☐ | ☐ | |
| ... | ... | ... | ... | ... | ... |

**测试状态说明：**
- ☐ 未测试
- ✓ 通过
- ✗ 失败

---

*文档生成时间：2024年12月*
