#!/bin/bash
# ============================================
# XShell 基础测试脚本
# 功能：验证核心功能是否可用
# 用法：./tests/run_tests.sh
# ============================================

set -e  # 遇到错误立即退出

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'  # No Color

# 测试计数
PASS=0
FAIL=0

# 测试结果函数
pass() {
    echo -e "${GREEN}[PASS]${NC} $1"
    PASS=$((PASS + 1))
}

fail() {
    echo -e "${RED}[FAIL]${NC} $1"
    FAIL=$((FAIL + 1))
}

info() {
    echo -e "${YELLOW}[INFO]${NC} $1"
}

# 检查 xshell 可执行文件是否存在
XSHELL="./xshell"
if [ ! -x "$XSHELL" ]; then
    echo "Error: $XSHELL not found or not executable"
    echo "Please run 'make' first"
    exit 1
fi

info "Starting XShell basic tests..."
echo ""

# ============================================
# 测试 1: 内置命令存在性测试
# ============================================
info "Test 1: Built-in commands existence"

# 测试 xhelp 命令（应该返回 0 并输出帮助信息）
if echo "xhelp" | $XSHELL 2>/dev/null | grep -q "XShell"; then
    pass "xhelp command works"
else
    fail "xhelp command failed"
fi

# 测试 xpwd 命令
if echo "xpwd" | $XSHELL 2>/dev/null | grep -q "/"; then
    pass "xpwd command works"
else
    fail "xpwd command failed"
fi

# 测试 xecho 命令
if echo 'xecho Hello World' | $XSHELL 2>/dev/null | grep -q "Hello World"; then
    pass "xecho command works"
else
    fail "xecho command failed"
fi

# ============================================
# 测试 2: 重定向测试
# ============================================
info "Test 2: Redirection"

# 创建临时目录
TMPDIR=$(mktemp -d)
trap "rm -rf $TMPDIR" EXIT

# 测试输出重定向 >
echo "xecho RedirectTest > $TMPDIR/out.txt" | $XSHELL 2>/dev/null
if [ -f "$TMPDIR/out.txt" ] && grep -q "RedirectTest" "$TMPDIR/out.txt"; then
    pass "Output redirection (>) works"
else
    fail "Output redirection (>) failed"
fi

# 测试追加重定向 >>
echo "xecho AppendTest >> $TMPDIR/out.txt" | $XSHELL 2>/dev/null
if grep -q "AppendTest" "$TMPDIR/out.txt"; then
    pass "Append redirection (>>) works"
else
    fail "Append redirection (>>) failed"
fi

# ============================================
# 测试 3: 管道测试
# ============================================
info "Test 3: Pipeline"

# 测试简单管道
if echo 'xecho "line1\nline2\nline3" | xwc -l' | $XSHELL 2>/dev/null | grep -q "[0-9]"; then
    pass "Simple pipeline works"
else
    fail "Simple pipeline failed (or xwc not available)"
fi

# ============================================
# 测试 4: 日志文件测试
# ============================================
info "Test 4: Error log file"

# 删除旧日志文件
rm -f .xshell_error

# 触发一个错误（访问不存在的文件）
echo "xcat /nonexistent_file_12345" | $XSHELL 2>/dev/null

# 检查日志文件是否生成
if [ -f ".xshell_error" ]; then
    pass "Error log file (.xshell_error) created"
    # 检查日志内容格式
    if grep -q "PID=" ".xshell_error"; then
        pass "Log file contains PID information"
    else
        fail "Log file missing PID information"
    fi
else
    fail "Error log file (.xshell_error) not created"
fi

# ============================================
# 测试 5: 脚本执行测试 (xsource)
# ============================================
info "Test 5: Script execution (xsource)"

# 创建测试脚本
cat > "$TMPDIR/test_script.sh" << 'EOF'
# 这是注释，应该被忽略
xecho ScriptLine1
xecho ScriptLine2
EOF

# 执行脚本
OUTPUT=$(echo "xsource $TMPDIR/test_script.sh" | $XSHELL 2>/dev/null)
if echo "$OUTPUT" | grep -q "ScriptLine1" && echo "$OUTPUT" | grep -q "ScriptLine2"; then
    pass "xsource script execution works"
else
    fail "xsource script execution failed"
fi

# ============================================
# 测试 6: 历史记录测试
# ============================================
info "Test 6: History"

# 执行一些命令然后查看历史
OUTPUT=$(echo -e "xecho test1\nxecho test2\nxhistory" | $XSHELL 2>/dev/null)
if echo "$OUTPUT" | grep -q "xecho"; then
    pass "xhistory command works"
else
    fail "xhistory command failed"
fi

# ============================================
# 测试结果汇总
# ============================================
echo ""
echo "============================================"
echo -e "Test Results: ${GREEN}$PASS passed${NC}, ${RED}$FAIL failed${NC}"
echo "============================================"

if [ $FAIL -gt 0 ]; then
    exit 1
fi

exit 0
