# ============================================
# XShell 项目 Makefile 构建脚本
# 功能：自动化编译 C 源文件，生成可执行文件 xshell
# 用法：make [目标名]
# ============================================

# ==================== 编译器配置 ====================
# 编译器：使用 GCC (GNU Compiler Collection)
CC = gcc

# 编译选项（CFLAGS = Compiler FLAGS）
# -Wall: 开启所有常用警告（Wall = Warnings all）
# -Wextra: 开启额外的警告信息
# -std=c99: 使用 C99 标准（C 语言 1999 版本标准）
# -g: 生成调试信息（用于 GDB 调试）
# -I./include: 指定头文件搜索路径（Include directory）
CFLAGS = -Wall -Wextra -std=c99 -g -I./include

# 链接选项（LDFLAGS = LinKer FLAGS，暂时为空）
# 如果需要链接库，可以添加：-lm (数学库), -lpthread (线程库) 等
LDFLAGS = 

# ==================== 目录定义 ====================
# 头文件目录（存放 .h 文件）
INCLUDE_DIR = include

# 源文件目录（存放 .c 文件）
SRC_DIR = src

# 内置命令源文件目录（存放内置命令的 .c 文件）
# $(变量名) 表示引用变量的值
BUILTIN_DIR = $(SRC_DIR)/builtin

# 目标文件目录（存放编译生成的 .o 文件）
OBJ_DIR = obj

# 可执行文件目录（. 表示当前目录）
BIN_DIR = .

# ==================== 目标文件定义 ====================
# 最终生成的可执行文件路径和名称
TARGET = $(BIN_DIR)/xshell

# ==================== 源文件列表 ====================
# 主要源文件列表（\ 表示续行，将多行连接成一行）
MAIN_SRCS = $(SRC_DIR)/main.c \
            $(SRC_DIR)/xshell.c \
            $(SRC_DIR)/parser.c \
            $(SRC_DIR)/executor.c \
            $(SRC_DIR)/utils.c \
            $(SRC_DIR)/completion.c \
            $(SRC_DIR)/input.c \
            $(SRC_DIR)/history.c \
            $(SRC_DIR)/alias.c

# 内置命令源文件列表（将来添加新命令时，在这里添加）
BUILTIN_SRCS = $(BUILTIN_DIR)/xpwd.c \
               $(BUILTIN_DIR)/xcd.c \
               $(BUILTIN_DIR)/xls.c \
               $(BUILTIN_DIR)/xecho.c \
               $(BUILTIN_DIR)/xtouch.c \
               $(BUILTIN_DIR)/xcat.c \
               $(BUILTIN_DIR)/xrm.c \
               $(BUILTIN_DIR)/xcp.c \
               $(BUILTIN_DIR)/xmv.c \
               $(BUILTIN_DIR)/xhistory.c \
               $(BUILTIN_DIR)/xtec.c \
               $(BUILTIN_DIR)/xmkdir.c \
               $(BUILTIN_DIR)/xrmdir.c \
               $(BUILTIN_DIR)/xln.c \
               $(BUILTIN_DIR)/xchmod.c \
               $(BUILTIN_DIR)/xchown.c \
               $(BUILTIN_DIR)/xfind.c \
               $(BUILTIN_DIR)/xuname.c \
               $(BUILTIN_DIR)/xhostname.c \
               $(BUILTIN_DIR)/xwhoami.c \
               $(BUILTIN_DIR)/xdate.c \
               $(BUILTIN_DIR)/xuptime.c \
               $(BUILTIN_DIR)/xps.c \
               $(BUILTIN_DIR)/xbasename.c \
               $(BUILTIN_DIR)/xdirname.c \
               $(BUILTIN_DIR)/xreadlink.c \
               $(BUILTIN_DIR)/xcut.c \
               $(BUILTIN_DIR)/xpaste.c \
               $(BUILTIN_DIR)/xtr.c \
               $(BUILTIN_DIR)/xcomm.c \
               $(BUILTIN_DIR)/xstat.c \
               $(BUILTIN_DIR)/xfile.c \
               $(BUILTIN_DIR)/xdu.c \
               $(BUILTIN_DIR)/xdf.c \
               $(BUILTIN_DIR)/xsplit.c \
               $(BUILTIN_DIR)/xjoin.c \
               $(BUILTIN_DIR)/xrealpath.c \
               $(BUILTIN_DIR)/xmenu.c \
               $(BUILTIN_DIR)/xdiff.c \
               $(BUILTIN_DIR)/xgrep.c \
               $(BUILTIN_DIR)/xwc.c \
               $(BUILTIN_DIR)/xhead.c \
               $(BUILTIN_DIR)/xtail.c \
               $(BUILTIN_DIR)/xsort.c \
               $(BUILTIN_DIR)/xuniq.c \
               $(BUILTIN_DIR)/xenv.c \
               $(BUILTIN_DIR)/xexport.c \
               $(BUILTIN_DIR)/xunset.c \
               $(BUILTIN_DIR)/xalias.c \
               $(BUILTIN_DIR)/xunalias.c \
               $(BUILTIN_DIR)/xclear.c \
               $(BUILTIN_DIR)/xhelp.c \
               $(BUILTIN_DIR)/xtype.c \
               $(BUILTIN_DIR)/xwhich.c \
               $(BUILTIN_DIR)/xsleep.c \
               $(BUILTIN_DIR)/xcalc.c \
               $(BUILTIN_DIR)/xtree.c \
               $(BUILTIN_DIR)/xsource.c \
               $(BUILTIN_DIR)/xtime.c \
               $(BUILTIN_DIR)/xkill.c \
               $(BUILTIN_DIR)/xjobs.c \
               $(BUILTIN_DIR)/xfg.c \
               $(BUILTIN_DIR)/xbg.c

# 所有源文件（主源文件 + 内置命令源文件）
SRCS = $(MAIN_SRCS) $(BUILTIN_SRCS)

# ==================== 目标文件列表 ====================
# 主要目标文件列表（.o 是编译后的二进制目标文件）
# 每个 .c 源文件编译后生成对应的 .o 文件
MAIN_OBJS = $(OBJ_DIR)/main.o \
            $(OBJ_DIR)/xshell.o \
            $(OBJ_DIR)/parser.o \
            $(OBJ_DIR)/executor.o \
            $(OBJ_DIR)/utils.o \
            $(OBJ_DIR)/completion.o \
            $(OBJ_DIR)/input.o \
            $(OBJ_DIR)/history.o \
            $(OBJ_DIR)/alias.o

# 内置命令目标文件列表
BUILTIN_OBJS = $(OBJ_DIR)/builtin/xpwd.o \
               $(OBJ_DIR)/builtin/xcd.o \
               $(OBJ_DIR)/builtin/xls.o \
               $(OBJ_DIR)/builtin/xecho.o \
               $(OBJ_DIR)/builtin/xtouch.o \
               $(OBJ_DIR)/builtin/xcat.o \
               $(OBJ_DIR)/builtin/xrm.o \
               $(OBJ_DIR)/builtin/xcp.o \
               $(OBJ_DIR)/builtin/xmv.o \
               $(OBJ_DIR)/builtin/xhistory.o \
               $(OBJ_DIR)/builtin/xtec.o \
               $(OBJ_DIR)/builtin/xmkdir.o \
               $(OBJ_DIR)/builtin/xrmdir.o \
               $(OBJ_DIR)/builtin/xln.o \
               $(OBJ_DIR)/builtin/xchmod.o \
               $(OBJ_DIR)/builtin/xchown.o \
               $(OBJ_DIR)/builtin/xfind.o \
               $(OBJ_DIR)/builtin/xuname.o \
               $(OBJ_DIR)/builtin/xhostname.o \
               $(OBJ_DIR)/builtin/xwhoami.o \
               $(OBJ_DIR)/builtin/xdate.o \
               $(OBJ_DIR)/builtin/xuptime.o \
               $(OBJ_DIR)/builtin/xps.o \
               $(OBJ_DIR)/builtin/xbasename.o \
               $(OBJ_DIR)/builtin/xdirname.o \
               $(OBJ_DIR)/builtin/xreadlink.o \
               $(OBJ_DIR)/builtin/xcut.o \
               $(OBJ_DIR)/builtin/xpaste.o \
               $(OBJ_DIR)/builtin/xtr.o \
               $(OBJ_DIR)/builtin/xcomm.o \
               $(OBJ_DIR)/builtin/xstat.o \
               $(OBJ_DIR)/builtin/xfile.o \
               $(OBJ_DIR)/builtin/xdu.o \
               $(OBJ_DIR)/builtin/xdf.o \
               $(OBJ_DIR)/builtin/xsplit.o \
               $(OBJ_DIR)/builtin/xjoin.o \
               $(OBJ_DIR)/builtin/xrealpath.o \
               $(OBJ_DIR)/builtin/xmenu.o \
               $(OBJ_DIR)/builtin/xdiff.o \
               $(OBJ_DIR)/builtin/xgrep.o \
               $(OBJ_DIR)/builtin/xwc.o \
               $(OBJ_DIR)/builtin/xhead.o \
               $(OBJ_DIR)/builtin/xtail.o \
               $(OBJ_DIR)/builtin/xsort.o \
               $(OBJ_DIR)/builtin/xuniq.o \
               $(OBJ_DIR)/builtin/xenv.o \
               $(OBJ_DIR)/builtin/xexport.o \
               $(OBJ_DIR)/builtin/xunset.o \
               $(OBJ_DIR)/builtin/xalias.o \
               $(OBJ_DIR)/builtin/xunalias.o \
               $(OBJ_DIR)/builtin/xclear.o \
               $(OBJ_DIR)/builtin/xhelp.o \
               $(OBJ_DIR)/builtin/xtype.o \
               $(OBJ_DIR)/builtin/xwhich.o \
               $(OBJ_DIR)/builtin/xsleep.o \
               $(OBJ_DIR)/builtin/xcalc.o \
               $(OBJ_DIR)/builtin/xtree.o \
               $(OBJ_DIR)/builtin/xsource.o \
               $(OBJ_DIR)/builtin/xtime.o \
               $(OBJ_DIR)/builtin/xkill.o \
               $(OBJ_DIR)/builtin/xjobs.o \
               $(OBJ_DIR)/builtin/xfg.o \
               $(OBJ_DIR)/builtin/xbg.o

# 所有目标文件（用于最终链接）
OBJS = $(MAIN_OBJS) $(BUILTIN_OBJS)

# ============================================
# 编译规则（Make Rules）
# ============================================

# .PHONY 声明伪目标（不对应实际文件的目标）
# 这些目标总是会被执行，不会因为同名文件存在而跳过
.PHONY: all clean run help test lint

# ==================== 默认目标 ====================
# 默认目标：直接执行 make 时运行此目标
# 依赖 $(TARGET)，即 ./xshell 可执行文件
all: $(TARGET)

# ==================== 链接规则 ====================
# 目标：生成可执行文件 xshell
# 依赖：所有 .o 目标文件 $(OBJS)
# | $(BIN_DIR) 是 order-only 依赖，只确保目录存在，不影响重新编译
$(TARGET): $(OBJS) | $(BIN_DIR)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# ==================== 编译规则（模式规则）====================
# 模式规则：% 是通配符，匹配任意字符串
# 规则：将 src/*.c 编译为 obj/*.o

# 编译主源文件的规则
# $< 表示第一个依赖（源文件）
# $@ 表示目标（.o 文件）
# -c 表示只编译不链接
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# 编译内置命令源文件的规则
# 规则：将 src/builtin/*.c 编译为 obj/builtin/*.o
$(OBJ_DIR)/builtin/%.o: $(BUILTIN_DIR)/%.c | $(OBJ_DIR)/builtin
	$(CC) $(CFLAGS) -c $< -o $@

# ==================== 目录创建规则 ====================
# 这些规则确保编译前目录存在
# -p 参数：如果目录已存在不报错，如果父目录不存在则自动创建

# 创建目标文件目录
$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

# 创建内置命令目标文件目录
$(OBJ_DIR)/builtin:
	mkdir -p $(OBJ_DIR)/builtin

# 创建可执行文件目录（当前目录，通常已存在）
$(BIN_DIR):
	mkdir -p $(BIN_DIR)

# ==================== 清理目标 ====================
# 删除所有编译生成的文件
# -r: 递归删除目录
# -f: 强制删除，不提示确认
clean:
	rm -rf $(OBJ_DIR) $(TARGET)
	@echo "Clean complete"

# ==================== 运行目标 ====================
# 先编译（依赖 $(TARGET)），然后运行程序
run: $(TARGET)
	./$(TARGET)

# ==================== 帮助目标 ====================
# 显示 Makefile 的使用说明
help:
	@echo "XShell Makefile"
	@echo "Usage:"
	@echo "  make         - Build the project"
	@echo "  make clean   - Remove build files"
	@echo "  make run     - Build and run XShell"
	@echo "  make test    - Run basic tests"
	@echo "  make lint    - Run static analysis (gcc -Wall -Wextra)"
	@echo "  make help    - Show this help message"

# ==================== 测试目标 ====================
# 运行基础测试脚本（不依赖 expect，纯 shell）
test: $(TARGET)
	@echo "Running XShell basic tests..."
	@chmod +x tests/run_tests.sh 2>/dev/null || true
	@tests/run_tests.sh

# ==================== Lint 目标 ====================
# 使用 gcc 进行静态检查（更严格的警告）
lint:
	$(CC) -Wall -Wextra -Wpedantic -std=c99 -I./include -fsyntax-only $(SRCS)
	@echo "Lint complete: no errors found"
