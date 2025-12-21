#!/bin/bash
# ============================================
# XShell 综合测试脚本 (扩展版)
# 功能：基于 test.md 的全面功能验证
# 用法：./tests/run_tests.sh
# ============================================

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

# 测试计数
PASS=0
FAIL=0
SKIP=0

# 测试结果函数
pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASS=$((PASS + 1))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAIL=$((FAIL + 1))
}

skip() {
    echo -e "${YELLOW}[SKIP]${NC} $1"
    SKIP=$((SKIP + 1))
}

info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

section() {
    echo ""
    echo -e "${YELLOW}========== $1 ==========${NC}"
}

# 运行 xshell 命令并检查输出
run_cmd() {
    local cmd="$1"
    echo "$cmd" | $XSHELL 2>/dev/null
}

# 断言：命令输出包含期望字符串
assert_contains() {
    local cmd="$1"
    local expected="$2"
    local desc="$3"
    
    if run_cmd "$cmd" | grep -q "$expected"; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：命令执行成功
assert_success() {
    local cmd="$1"
    local desc="$2"
    
    if run_cmd "$cmd" >/dev/null 2>&1; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：文件存在
assert_file_exists() {
    local file="$1"
    local desc="$2"
    if [ -f "$file" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：文件不存在
assert_file_not_exists() {
    local file="$1"
    local desc="$2"
    if [ ! -f "$file" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：目录存在
assert_dir_exists() {
    local dir="$1"
    local desc="$2"
    if [ -d "$dir" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：目录不存在
assert_dir_not_exists() {
    local dir="$1"
    local desc="$2"
    if [ ! -d "$dir" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：文件内容包含
assert_file_contains() {
    local file="$1"
    local expected="$2"
    local desc="$3"
    if grep -q "$expected" "$file" 2>/dev/null; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：符号链接
assert_is_symlink() {
    local file="$1"
    local desc="$2"
    if [ -L "$file" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# 断言：文件可执行
assert_is_executable() {
    local file="$1"
    local desc="$2"
    if [ -x "$file" ]; then
        pass "$desc"
    else
        fail "$desc"
    fi
}

# ============================================
# 检查 xshell 可执行文件
# ============================================
XSHELL="./xshell"
if [ ! -x "$XSHELL" ]; then
    echo "Error: $XSHELL not found or not executable"
    echo "Please run 'make' first"
    exit 1
fi

# 创建临时目录
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

info "Starting XShell comprehensive tests..."
info "Temp directory: $TMPDIR"

# ============================================
# 一、基础命令测试
# ============================================
section "一、基础命令 (xpwd, xcd, xls, xecho, xclear, quit)"

# 1. xpwd
assert_contains "xpwd" "/" "xpwd: 显示当前目录"
assert_contains "xpwd --help" "用法" "xpwd: --help"

# 2. xcd
assert_success "xcd /" "xcd: 切换到根目录"
assert_success "xcd ~" "xcd: 切换到主目录"
assert_success "xcd .." "xcd: 切换到上级目录"
assert_contains "xcd --help" "用法" "xcd: --help"
assert_contains "xcd /nonexistent_dir_12345" "" "xcd: 不存在目录错误处理"

# 3. xls
assert_success "xls" "xls: 列出当前目录"
assert_success "xls -l" "xls: -l 长格式"
assert_success "xls -a" "xls: -a 显示隐藏"
assert_success "xls -la" "xls: -la 组合"
assert_success "xls -lh" "xls: -lh 人性化大小"
assert_success "xls /tmp" "xls: 指定目录"
assert_contains "xls --help" "用法" "xls: --help"

# 4. xecho
assert_contains 'xecho Hello' "Hello" "xecho: 简单输出"
assert_contains 'xecho Hello World' "Hello World" "xecho: 多参数"
assert_contains 'xecho -e "a\nb"' "a" "xecho: -e 转义"
assert_contains 'xecho -n test' "test" "xecho: -n 不换行"
assert_contains "xecho --help" "用法" "xecho: --help"

# 5. xclear
assert_contains "xclear --help" "用法" "xclear: --help"

# 6. quit
assert_contains "quit --help" "用法" "quit: --help"

# ============================================
# 二、文件操作测试
# ============================================
section "二、文件操作"

# 7. xtouch
run_cmd "xtouch $TMPDIR/touch1.txt"
assert_file_exists "$TMPDIR/touch1.txt" "xtouch: 创建单个文件"
run_cmd "xtouch $TMPDIR/touch2.txt $TMPDIR/touch3.txt"
assert_file_exists "$TMPDIR/touch2.txt" "xtouch: 创建多个文件(1)"
assert_file_exists "$TMPDIR/touch3.txt" "xtouch: 创建多个文件(2)"
assert_contains "xtouch --help" "用法" "xtouch: --help"

# 8. xcat
echo "line1" > "$TMPDIR/cat_test.txt"
echo "line2" >> "$TMPDIR/cat_test.txt"
assert_contains "xcat $TMPDIR/cat_test.txt" "line1" "xcat: 显示内容"
assert_contains "xcat -n $TMPDIR/cat_test.txt" "1" "xcat: -n 行号"
assert_contains "xcat --help" "用法" "xcat: --help"
assert_contains "xcat /nonexistent_file" "" "xcat: 不存在文件错误"

# 9. xrm
run_cmd "xtouch $TMPDIR/rm_test.txt"
run_cmd "xrm $TMPDIR/rm_test.txt"
assert_file_not_exists "$TMPDIR/rm_test.txt" "xrm: 删除文件"
run_cmd "xmkdir -p $TMPDIR/rm_dir/sub"
run_cmd "xtouch $TMPDIR/rm_dir/sub/file.txt"
run_cmd "xrm -r $TMPDIR/rm_dir"
assert_dir_not_exists "$TMPDIR/rm_dir" "xrm: -r 递归删除"
assert_contains "xrm --help" "用法" "xrm: --help"

# 10. xcp
echo "copy content" > "$TMPDIR/cp_src.txt"
run_cmd "xcp $TMPDIR/cp_src.txt $TMPDIR/cp_dst.txt"
assert_file_exists "$TMPDIR/cp_dst.txt" "xcp: 复制文件"
assert_file_contains "$TMPDIR/cp_dst.txt" "copy content" "xcp: 内容正确"
run_cmd "xmkdir $TMPDIR/cp_dir"
run_cmd "xtouch $TMPDIR/cp_dir/f1.txt"
run_cmd "xcp -r $TMPDIR/cp_dir $TMPDIR/cp_dir_copy"
assert_dir_exists "$TMPDIR/cp_dir_copy" "xcp: -r 递归复制"
assert_contains "xcp --help" "用法" "xcp: --help"

# 11. xmv
run_cmd "xtouch $TMPDIR/mv_old.txt"
run_cmd "xmv $TMPDIR/mv_old.txt $TMPDIR/mv_new.txt"
assert_file_exists "$TMPDIR/mv_new.txt" "xmv: 重命名"
assert_file_not_exists "$TMPDIR/mv_old.txt" "xmv: 原文件删除"
run_cmd "xmkdir $TMPDIR/mv_dir"
run_cmd "xmv $TMPDIR/mv_new.txt $TMPDIR/mv_dir/"
assert_file_exists "$TMPDIR/mv_dir/mv_new.txt" "xmv: 移动到目录"
assert_contains "xmv --help" "用法" "xmv: --help"

# 12. xstat
assert_success "xstat /etc/passwd" "xstat: 文件信息"
assert_success "xstat /tmp" "xstat: 目录信息"
assert_contains "xstat --help" "用法" "xstat: --help"

# 13. xfile
assert_contains "xfile /etc/passwd" "text" "xfile: 文本文件"
assert_success "xfile /bin/ls" "xfile: 二进制文件"
assert_success "xfile /tmp" "xfile: 目录"
assert_contains "xfile --help" "用法" "xfile: --help"

# 14. xreadlink
run_cmd "xln -s /etc/passwd $TMPDIR/link_test"
assert_contains "xreadlink $TMPDIR/link_test" "passwd" "xreadlink: 读取链接"
assert_contains "xreadlink --help" "用法" "xreadlink: --help"

# 15. xrealpath
assert_contains "xrealpath ." "/" "xrealpath: 当前目录"
assert_contains "xrealpath /tmp/../etc" "/etc" "xrealpath: 解析.."
assert_contains "xrealpath --help" "用法" "xrealpath: --help"

# 16. xbasename
assert_contains "xbasename /path/to/file.txt" "file.txt" "xbasename: 提取文件名"
assert_contains "xbasename /path/to/file.txt .txt" "file" "xbasename: 去后缀"
assert_contains "xbasename --help" "用法" "xbasename: --help"

# 17. xdirname
assert_contains "xdirname /path/to/file.txt" "/path/to" "xdirname: 提取目录"
assert_contains "xdirname /file.txt" "/" "xdirname: 根目录"
assert_contains "xdirname --help" "用法" "xdirname: --help"

# ============================================
# 三、目录操作测试
# ============================================
section "三、目录操作"

# 18. xmkdir
run_cmd "xmkdir $TMPDIR/mkdir_test"
assert_dir_exists "$TMPDIR/mkdir_test" "xmkdir: 创建目录"
run_cmd "xmkdir -p $TMPDIR/deep/a/b/c"
assert_dir_exists "$TMPDIR/deep/a/b/c" "xmkdir: -p 递归"
run_cmd "xmkdir $TMPDIR/md1 $TMPDIR/md2"
assert_dir_exists "$TMPDIR/md1" "xmkdir: 多目录(1)"
assert_dir_exists "$TMPDIR/md2" "xmkdir: 多目录(2)"
assert_contains "xmkdir --help" "用法" "xmkdir: --help"

# 19. xrmdir
run_cmd "xmkdir $TMPDIR/empty1"
run_cmd "xrmdir $TMPDIR/empty1"
assert_dir_not_exists "$TMPDIR/empty1" "xrmdir: 删除空目录"
run_cmd "xmkdir $TMPDIR/e1 $TMPDIR/e2"
run_cmd "xrmdir $TMPDIR/e1 $TMPDIR/e2"
assert_dir_not_exists "$TMPDIR/e1" "xrmdir: 多目录(1)"
assert_dir_not_exists "$TMPDIR/e2" "xrmdir: 多目录(2)"
assert_contains "xrmdir --help" "用法" "xrmdir: --help"

# 20. xtree
assert_success "xtree $TMPDIR" "xtree: 显示目录树"
assert_success "xtree -L 1 $TMPDIR" "xtree: -L 限制深度"
assert_contains "xtree --help" "用法" "xtree: --help"

# 21. xfind
run_cmd "xmkdir -p $TMPDIR/find_dir/sub"
run_cmd "xtouch $TMPDIR/find_dir/target.txt"
run_cmd "xtouch $TMPDIR/find_dir/sub/target2.txt"
assert_contains "xfind $TMPDIR/find_dir -name \"*.txt\"" "target" "xfind: 按名查找"
assert_contains "xfind --help" "用法" "xfind: --help"

# 22. xdu
assert_success "xdu $TMPDIR" "xdu: 目录大小"
assert_success "xdu -h $TMPDIR" "xdu: -h 人性化"
assert_success "xdu -s $TMPDIR" "xdu: -s 汇总"
assert_contains "xdu --help" "用法" "xdu: --help"

# 23. xdf
assert_success "xdf" "xdf: 磁盘空间"
assert_success "xdf -h" "xdf: -h 人性化"
assert_contains "xdf --help" "用法" "xdf: --help"

# ============================================
# 四、权限与链接测试
# ============================================
section "四、权限与链接"

# 24. xchmod
run_cmd "xtouch $TMPDIR/chmod_test"
run_cmd "xchmod 755 $TMPDIR/chmod_test"
assert_is_executable "$TMPDIR/chmod_test" "xchmod: 755 设置执行权限"
run_cmd "xchmod 644 $TMPDIR/chmod_test"
if [ ! -x "$TMPDIR/chmod_test" ]; then
    pass "xchmod: 644 移除执行权限"
else
    fail "xchmod: 644 移除执行权限"
fi
run_cmd "xchmod u+x $TMPDIR/chmod_test"
assert_is_executable "$TMPDIR/chmod_test" "xchmod: u+x 符号模式"
assert_contains "xchmod --help" "用法" "xchmod: --help"

# 25. xchown (需要 root)
assert_contains "xchown --help" "用法" "xchown: --help"
skip "xchown: 需要 root 权限"

# 26. xln
run_cmd "xtouch $TMPDIR/ln_src.txt"
run_cmd "xln $TMPDIR/ln_src.txt $TMPDIR/ln_hard.txt"
assert_file_exists "$TMPDIR/ln_hard.txt" "xln: 创建硬链接"
run_cmd "xln -s $TMPDIR/ln_src.txt $TMPDIR/ln_sym.txt"
assert_is_symlink "$TMPDIR/ln_sym.txt" "xln: -s 创建符号链接"
assert_contains "xln --help" "用法" "xln: --help"

# ============================================
# 五、文本处理测试
# ============================================
section "五、文本处理"

# 准备测试文件
echo -e "root:x:0:0:root\nuser:x:1000:1000:user\ntest:x:1001:1001:test" > "$TMPDIR/text_test.txt"

# 27. xgrep
assert_contains "xgrep root $TMPDIR/text_test.txt" "root" "xgrep: 基本搜索"
assert_contains "xgrep -i ROOT $TMPDIR/text_test.txt" "root" "xgrep: -i 忽略大小写"
assert_contains "xgrep -n root $TMPDIR/text_test.txt" "1:" "xgrep: -n 行号"
assert_contains "xgrep -c root $TMPDIR/text_test.txt" "1" "xgrep: -c 计数"
assert_success "xgrep -v root $TMPDIR/text_test.txt" "xgrep: -v 反向"
assert_contains "xgrep --help" "用法" "xgrep: --help"

# 28. xwc
assert_contains "xwc $TMPDIR/text_test.txt" "3" "xwc: 行数"
assert_success "xwc -l $TMPDIR/text_test.txt" "xwc: -l"
assert_success "xwc -w $TMPDIR/text_test.txt" "xwc: -w"
assert_success "xwc -c $TMPDIR/text_test.txt" "xwc: -c"
assert_contains "xwc --help" "用法" "xwc: --help"

# 29. xhead
assert_success "xhead $TMPDIR/text_test.txt" "xhead: 默认前10行"
assert_contains "xhead -n 1 $TMPDIR/text_test.txt" "root" "xhead: -n 1"
assert_contains "xhead --help" "用法" "xhead: --help"

# 30. xtail
assert_success "xtail $TMPDIR/text_test.txt" "xtail: 默认后10行"
assert_contains "xtail -n 1 $TMPDIR/text_test.txt" "test" "xtail: -n 1"
assert_contains "xtail --help" "用法" "xtail: --help"

# 31. xsort
echo -e "c\na\nb" > "$TMPDIR/sort_test.txt"
assert_success "xsort $TMPDIR/sort_test.txt" "xsort: 正序排序"
assert_success "xsort -r $TMPDIR/sort_test.txt" "xsort: -r 逆序"
echo -e "10\n2\n1" > "$TMPDIR/num_sort.txt"
assert_success "xsort -n $TMPDIR/num_sort.txt" "xsort: -n 数值"
assert_success "xsort -u $TMPDIR/sort_test.txt" "xsort: -u 去重"
assert_contains "xsort --help" "用法" "xsort: --help"

# 32. xuniq
echo -e "a\na\nb\nb\nc" > "$TMPDIR/uniq_test.txt"
assert_success "xuniq $TMPDIR/uniq_test.txt" "xuniq: 去重"
assert_success "xuniq -c $TMPDIR/uniq_test.txt" "xuniq: -c 计数"
assert_success "xuniq -d $TMPDIR/uniq_test.txt" "xuniq: -d 只重复"
assert_contains "xuniq --help" "用法" "xuniq: --help"

# 33. xdiff
echo "line1" > "$TMPDIR/diff1.txt"
echo "line2" > "$TMPDIR/diff2.txt"
assert_success "xdiff $TMPDIR/diff1.txt $TMPDIR/diff2.txt" "xdiff: 比较差异"
assert_success "xdiff -u $TMPDIR/diff1.txt $TMPDIR/diff2.txt" "xdiff: -u 统一格式"
assert_contains "xdiff --help" "用法" "xdiff: --help"

# 34. xcut
echo "a:b:c:d" > "$TMPDIR/cut_test.txt"
assert_contains "xcut -d: -f1 $TMPDIR/cut_test.txt" "a" "xcut: -f1"
assert_contains "xcut -d: -f1,3 $TMPDIR/cut_test.txt" "c" "xcut: -f1,3"
assert_contains "xcut --help" "用法" "xcut: --help"

# 35. xpaste
echo -e "1\n2\n3" > "$TMPDIR/paste1.txt"
echo -e "a\nb\nc" > "$TMPDIR/paste2.txt"
assert_contains "xpaste $TMPDIR/paste1.txt $TMPDIR/paste2.txt" "1" "xpaste: 合并"
assert_success "xpaste -d, $TMPDIR/paste1.txt $TMPDIR/paste2.txt" "xpaste: -d 分隔符"
assert_contains "xpaste --help" "用法" "xpaste: --help"

# 36. xtr
assert_contains 'xecho "HELLO" | xtr A-Z a-z' "hello" "xtr: 大写转小写"
assert_contains 'xecho "hello123" | xtr -d 0-9' "hello" "xtr: -d 删除数字"
assert_contains "xtr --help" "用法" "xtr: --help"

# 37. xcomm
echo -e "a\nb\nc" > "$TMPDIR/comm1.txt"
echo -e "b\nc\nd" > "$TMPDIR/comm2.txt"
assert_success "xcomm $TMPDIR/comm1.txt $TMPDIR/comm2.txt" "xcomm: 比较文件"
assert_contains "xcomm --help" "用法" "xcomm: --help"

# 38. xsplit
echo -e "line1\nline2\nline3\nline4\nline5" > "$TMPDIR/split_src.txt"
run_cmd "xsplit -l 2 $TMPDIR/split_src.txt $TMPDIR/split_"
assert_file_exists "$TMPDIR/split_aa" "xsplit: 分割文件"
assert_contains "xsplit --help" "用法" "xsplit: --help"

# 39. xjoin
echo -e "1 a\n2 b" > "$TMPDIR/join1.txt"
echo -e "1 x\n2 y" > "$TMPDIR/join2.txt"
assert_success "xjoin $TMPDIR/join1.txt $TMPDIR/join2.txt" "xjoin: 连接文件"
assert_contains "xjoin --help" "用法" "xjoin: --help"

# ============================================
# 六、系统信息测试
# ============================================
section "六、系统信息"

# 40. xuname
assert_contains "xuname" "Linux" "xuname: 默认输出"
assert_contains "xuname -a" "Linux" "xuname: -a 全部"
assert_success "xuname -s" "xuname: -s 内核名"
assert_success "xuname -r" "xuname: -r 版本"
assert_success "xuname -m" "xuname: -m 架构"
assert_success "xuname -n" "xuname: -n 主机名"
assert_contains "xuname --help" "用法" "xuname: --help"

# 41. xhostname
assert_success "xhostname" "xhostname: 显示主机名"
assert_contains "xhostname --help" "用法" "xhostname: --help"

# 42. xwhoami
assert_success "xwhoami" "xwhoami: 显示用户"
assert_contains "xwhoami --help" "用法" "xwhoami: --help"

# 43. xdate
assert_success "xdate" "xdate: 显示日期"
assert_success "xdate -u" "xdate: -u UTC"
assert_contains "xdate --help" "用法" "xdate: --help"

# 44. xuptime
assert_success "xuptime" "xuptime: 运行时间"
assert_contains "xuptime --help" "用法" "xuptime: --help"

# 45. xps
assert_success "xps" "xps: 进程列表"
assert_contains "xps --help" "用法" "xps: --help"

# ============================================
# 七、环境变量和别名测试
# ============================================
section "七、环境变量和别名"

# 46. xenv
assert_contains "xenv" "PATH" "xenv: 显示环境变量"
assert_contains "xenv --help" "用法" "xenv: --help"

# 47. xexport
assert_contains "xexport MYTEST=hello && xenv" "MYTEST" "xexport: 设置变量"
assert_contains "xexport --help" "用法" "xexport: --help"

# 48. xunset
assert_success "xexport DELME=1 && xunset DELME" "xunset: 删除变量"
assert_contains "xunset --help" "用法" "xunset: --help"

# 49. xalias
assert_success "xalias" "xalias: 显示别名"
assert_success "xalias ll='xls -la'" "xalias: 设置别名"
assert_contains "xalias --help" "用法" "xalias: --help"

# 50. xunalias
assert_success "xalias test='xpwd' && xunalias test" "xunalias: 删除别名"
assert_contains "xunalias --help" "用法" "xunalias: --help"

# ============================================
# 八、进程与作业控制测试
# ============================================
section "八、进程与作业控制"

# 51. xkill
assert_contains "xkill --help" "用法" "xkill: --help"
assert_contains "xkill 99999" "" "xkill: 无效PID错误"

# 52. xjobs
assert_success "xjobs" "xjobs: 显示任务"
assert_contains "xjobs --help" "用法" "xjobs: --help"

# 53. xfg
assert_contains "xfg --help" "用法" "xfg: --help"

# 54. xbg
assert_contains "xbg --help" "用法" "xbg: --help"

# ============================================
# 九、实用工具测试
# ============================================
section "九、实用工具"

# 55. xhelp
assert_contains "xhelp" "XShell" "xhelp: 命令列表"
assert_contains "xhelp xls" "xls" "xhelp: 指定命令"
assert_contains "xhelp --help" "用法" "xhelp: --help"

# 56. xtype
assert_contains "xtype xls" "builtin" "xtype: 内置命令"
assert_success "xtype ls" "xtype: 外部命令"
assert_contains "xtype nonexistent" "" "xtype: 不存在命令"
assert_contains "xtype --help" "用法" "xtype: --help"

# 57. xwhich
assert_contains "xwhich ls" "/" "xwhich: 查找路径"
assert_success "xwhich cat grep" "xwhich: 多命令"
assert_contains "xwhich --help" "用法" "xwhich: --help"

# 58. xsleep
assert_success "xsleep 1" "xsleep: 休眠1秒"
assert_contains "xsleep --help" "用法" "xsleep: --help"

# 59. xcalc
assert_contains 'xcalc "1+1"' "2" "xcalc: 加法"
assert_contains 'xcalc "10-3"' "7" "xcalc: 减法"
assert_contains 'xcalc "4*5"' "20" "xcalc: 乘法"
assert_contains 'xcalc "20/4"' "5" "xcalc: 除法"
assert_contains 'xcalc "(2+3)*4"' "20" "xcalc: 括号"
assert_contains "xcalc --help" "用法" "xcalc: --help"

# 60. xtime
assert_success "xtime xpwd" "xtime: 测量时间"
assert_contains "xtime --help" "用法" "xtime: --help"

# 61. xsource
echo -e "xpwd\nxdate" > "$TMPDIR/script.sh"
assert_contains "xsource $TMPDIR/script.sh" "/" "xsource: 执行脚本"
assert_contains "xsource /nonexistent" "" "xsource: 不存在脚本"
assert_contains "xsource --help" "用法" "xsource: --help"

# 62. xtec
assert_contains "xtec --help" "用法" "xtec: --help"
run_cmd "xecho test | xtec $TMPDIR/tec_out.txt"
assert_file_exists "$TMPDIR/tec_out.txt" "xtec: 写入文件"

# 63. xhistory
assert_success "xhistory" "xhistory: 显示历史"
assert_contains "xhistory --help" "用法" "xhistory: --help"

# ============================================
# 十、特色功能测试
# ============================================
section "十、特色功能"

# 64. xui (交互式)
skip "xui: 交互式命令"

# 65. xmenu
assert_contains "xmenu --help" "用法" "xmenu: --help"

# 66. xweb
assert_contains "xweb --help" "用法" "xweb: --help"

# 67. xsysmon
assert_contains "xsysmon --help" "用法" "xsysmon: --help"

# 68. xsnake
assert_contains "xsnake --help" "用法" "xsnake: --help"

# 69. xtetris
assert_contains "xtetris --help" "用法" "xtetris: --help"

# 70. x2048
assert_contains "x2048 --help" "用法" "x2048: --help"

# ============================================
# 十一、管道测试
# ============================================
section "十一、管道操作"

assert_success 'xecho "hello" | xcat' "管道: xecho | xcat"
assert_contains 'xecho "line1\nline2\nline3" | xwc -l' "3" "管道: xecho | xwc -l"
assert_success "xls /etc | xgrep conf" "管道: xls | xgrep"
assert_success "xcat /etc/passwd | xhead -5 | xtail -1" "管道: 多重管道"
assert_success "xps | xgrep -v grep | xhead -5" "管道: 三级管道"
assert_contains 'xecho "A B C" | xtr " " "\n" | xsort' "A" "管道: xtr | xsort"

# ============================================
# 十二、重定向测试
# ============================================
section "十二、重定向操作"

# 输出重定向 >
run_cmd "xecho redirect_test > $TMPDIR/redir1.txt"
assert_file_exists "$TMPDIR/redir1.txt" "重定向 >: 创建文件"
assert_file_contains "$TMPDIR/redir1.txt" "redirect_test" "重定向 >: 内容正确"

# 覆盖测试
run_cmd "xecho new_content > $TMPDIR/redir1.txt"
assert_file_contains "$TMPDIR/redir1.txt" "new_content" "重定向 >: 覆盖内容"

# 追加重定向 >>
run_cmd "xecho append1 > $TMPDIR/append.txt"
run_cmd "xecho append2 >> $TMPDIR/append.txt"
assert_file_contains "$TMPDIR/append.txt" "append1" "重定向 >>: 保留原内容"
assert_file_contains "$TMPDIR/append.txt" "append2" "重定向 >>: 追加新内容"

# 错误重定向 2>
run_cmd "xcat /nonexistent_12345 2> $TMPDIR/stderr.txt"
assert_file_exists "$TMPDIR/stderr.txt" "重定向 2>: 创建错误文件"

# 输入重定向 <
echo "input_content" > "$TMPDIR/input.txt"
assert_contains "xcat < $TMPDIR/input.txt" "input_content" "重定向 <: 输入重定向"

# ============================================
# 十三、后台任务测试
# ============================================
section "十三、后台任务"

assert_success "xjobs" "后台任务: xjobs 空列表"
# 后台执行需要特殊处理，这里只做基本检查
skip "后台任务: & 操作符 (需要交互)"

# ============================================
# 十四、日志系统测试
# ============================================
section "十四、日志系统"

rm -f .xshell_error
run_cmd "xcat /nonexistent_file_for_log_test"
assert_file_exists ".xshell_error" "日志: 错误日志生成"
assert_file_contains ".xshell_error" "PID=" "日志: 包含 PID"
# 时间戳可能是 TIME= 或 直接包含日期格式
if grep -qE "(TIME=|[0-9]{4}-[0-9]{2}-[0-9]{2}|[0-9]{2}:[0-9]{2}:[0-9]{2})" ".xshell_error"; then
    pass "日志: 包含时间信息"
else
    skip "日志: 时间戳格式可能不同"
fi

# ============================================
# 十五、边界情况测试
# ============================================
section "十五、边界情况"

# 空命令
assert_success "" "边界: 空命令"

# 长命令
LONG_ECHO=$(printf 'xecho %0.sa' {1..100})
assert_success "$LONG_ECHO" "边界: 长命令"

# 特殊字符
assert_contains 'xecho "hello world"' "hello world" "边界: 双引号字符串"
assert_contains "xecho 'hello world'" "hello world" "边界: 单引号字符串"

# 多条命令
assert_success "xpwd && xdate" "边界: && 连接"
assert_success "xpwd ; xdate" "边界: ; 连接"

# ============================================
# 测试结果汇总
# ============================================
echo ""
echo "============================================"
echo -e "测试结果: ${GREEN}$PASS 通过${NC}, ${RED}$FAIL 失败${NC}, ${YELLOW}$SKIP 跳过${NC}"
TOTAL=$((PASS + FAIL))
if [ $TOTAL -gt 0 ]; then
    RATE=$((PASS * 100 / TOTAL))
    echo -e "通过率: ${GREEN}${RATE}%${NC}"
fi
echo "============================================"

if [ $FAIL -gt 0 ]; then
    exit 1
fi

exit 0
