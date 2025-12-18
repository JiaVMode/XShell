/*
 * xcalc.c - 简单计算器
 * 
 * 功能：计算简单的数学表达式
 * 用法：xcalc <expression>
 */

#include "builtin.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

// 去掉字符串两端的引号和空白字符
static void trim_quotes(char *str) {
    int len = strlen(str);
    int start = 0, end = len - 1;
    
    // 跳过开头的空白字符和引号
    while (start < len && (str[start] == ' ' || str[start] == '\t' || 
           str[start] == '"' || str[start] == '\'')) {
        start++;
    }
    
    // 跳过末尾的空白字符和引号
    while (end >= start && (str[end] == ' ' || str[end] == '\t' || 
           str[end] == '"' || str[end] == '\'' || str[end] == '\n')) {
        end--;
    }
    
    // 移动字符串内容
    if (start > 0 || end < len - 1) {
        int i;
        for (i = 0; i <= end - start; i++) {
            str[i] = str[start + i];
        }
        str[i] = '\0';
    }
}

// 错误类型定义
#define XCALC_ERROR_INVALID_FORMAT -1
#define XCALC_ERROR_DIVISION_BY_ZERO -2
#define XCALC_ERROR_INVALID_OPERATOR -3

// 表达式解析器状态
typedef struct {
    const char *pos;
    ShellContext *ctx;
} ParseState;

// 前向声明
static int parse_expression(ParseState *state, double *result);

// 跳过空白字符
static void skip_whitespace(ParseState *state) {
    while (*state->pos && (*state->pos == ' ' || *state->pos == '\t')) {
        state->pos++;
    }
}

// 解析数字
static int parse_number(ParseState *state, double *result) {
    skip_whitespace(state);
    
    char *endptr;
    *result = strtod(state->pos, &endptr);
    
    if (endptr == state->pos) {
        return XCALC_ERROR_INVALID_FORMAT;
    }
    
    state->pos = endptr;
    return 0;
}

// 解析因子（数字或括号表达式）
static int parse_factor(ParseState *state, double *result) {
    skip_whitespace(state);
    
    if (*state->pos == '(') {
        // 括号表达式
        state->pos++;
        int ret = parse_expression(state, result);
        if (ret != 0) return ret;
        
        skip_whitespace(state);
        if (*state->pos != ')') {
            XSHELL_LOG_ERROR(state->ctx, "xcalc: missing closing parenthesis\n");
            return XCALC_ERROR_INVALID_OPERATOR;  // 使用不同的错误代码避免重复信息
        }
        state->pos++;
        return 0;
    } else if (*state->pos == '-') {
        // 负数
        state->pos++;
        int ret = parse_factor(state, result);
        if (ret != 0) return ret;
        *result = -*result;
        return 0;
    } else if (*state->pos == '+') {
        // 正数（跳过+号）
        state->pos++;
        return parse_factor(state, result);
    } else {
        // 普通数字
        return parse_number(state, result);
    }
}

// 解析项（处理 *, /, %）
static int parse_term(ParseState *state, double *result) {
    int ret = parse_factor(state, result);
    if (ret != 0) return ret;
    
    while (1) {
        skip_whitespace(state);
        char op = *state->pos;
        
        if (op != '*' && op != '/' && op != '%' && op != 'x' && op != 'X') {
            break;
        }
        
        state->pos++;
        double right;
        ret = parse_factor(state, &right);
        if (ret != 0) return ret;
        
        switch (op) {
            case '*':
            case 'x':
            case 'X':
                *result = *result * right;
                break;
            case '/':
                if (right == 0) {
                    XSHELL_LOG_ERROR(state->ctx, "xcalc: division by zero\n");
                    return XCALC_ERROR_DIVISION_BY_ZERO;
                }
                *result = *result / right;
                break;
            case '%':
                if (right == 0) {
                    XSHELL_LOG_ERROR(state->ctx, "xcalc: division by zero\n");
                    return XCALC_ERROR_DIVISION_BY_ZERO;
                }
                *result = (int)*result % (int)right;
                break;
        }
    }
    
    return 0;
}

// 解析表达式（处理 +, -）
static int parse_expression(ParseState *state, double *result) {
    int ret = parse_term(state, result);
    if (ret != 0) return ret;
    
    while (1) {
        skip_whitespace(state);
        char op = *state->pos;
        
        if (op != '+' && op != '-') {
            break;
        }
        
        state->pos++;
        double right;
        ret = parse_term(state, &right);
        if (ret != 0) return ret;
        
        switch (op) {
            case '+':
                *result = *result + right;
                break;
            case '-':
                *result = *result - right;
                break;
        }
    }
    
    return 0;
}

// 主表达式求值函数
// 支持：+, -, *, /, %, 括号, 运算符优先级
// 返回值：0=成功, XCALC_ERROR_*=不同类型的错误
static int evaluate_expression(const char *expr, double *result, ShellContext *ctx) {
    // 创建副本并去除引号
    char clean_expr[1024];
    strncpy(clean_expr, expr, sizeof(clean_expr) - 1);
    clean_expr[sizeof(clean_expr) - 1] = '\0';
    trim_quotes(clean_expr);
    
    // 初始化解析状态
    ParseState state = { clean_expr, ctx };
    
    // 解析表达式
    int ret = parse_expression(&state, result);
    if (ret != 0) return ret;
    
    // 检查是否还有未解析的字符
    skip_whitespace(&state);
    if (*state.pos != '\0') {
        XSHELL_LOG_ERROR(ctx, "xcalc: unexpected characters: '%s'\n", state.pos);
        return XCALC_ERROR_INVALID_FORMAT;
    }
    
    return 0;
}

int cmd_xcalc(Command *cmd, ShellContext *ctx) {
    // 显示帮助信息
    if (cmd->arg_count >= 2 && strcmp(cmd->args[1], "--help") == 0) {
        printf("xcalc - 简单计算器\n\n");
        printf("用法:\n");
        printf("  xcalc <expression>\n\n");
        printf("说明:\n");
        printf("  计算简单的数学表达式。\n");
        printf("  Calculator - 计算器。\n\n");
        printf("参数:\n");
        printf("  expression  数学表达式（支持括号和运算符优先级）\n\n");
        printf("选项:\n");
        printf("  --help      显示此帮助信息\n\n");
        printf("支持的运算符:\n");
        printf("  +           加法\n");
        printf("  -           减法\n");
        printf("  * 或 x      乘法\n");
        printf("  /           除法\n");
        printf("  %%           取模（整数）\n\n");
        printf("示例:\n");
        printf("  xcalc '10 + 5'             # 基本运算：15\n");
        printf("  xcalc '2 + 3 * 4'          # 运算符优先级：14\n");
        printf("  xcalc '(2 + 3) * 4'        # 括号优先：20\n");
        printf("  xcalc '((2 + 3) * 4) / 2'  # 复杂表达式：10\n");
        printf("  xcalc '17 %% 5'            # 取模：2\n");
        printf("  xcalc '-5 + 3'             # 负数：-2\n");
        printf("  xcalc 3.14                 # 单个数字：3.14\n\n");
        printf("特性:\n");
        printf("  • 支持运算符优先级（* / %% 优先于 + -）\n");
        printf("  • 支持括号改变优先级\n");
        printf("  • 支持复杂嵌套表达式\n");
        printf("  • 支持负数和浮点数\n");
        printf("  • 取模运算会将操作数转为整数\n\n");
        printf("对应系统命令: bc, expr\n");
        return 0;
    }
    
    // 检查参数
    if (cmd->arg_count < 2) {
        XSHELL_LOG_ERROR(ctx, "xcalc: missing expression\n");
        XSHELL_LOG_ERROR(ctx, "Try 'xcalc --help' for more information.\n");
        return -1;
    }
    
    // 将所有参数合并成一个表达式
    char expression[1024] = "";
    for (int i = 1; i < cmd->arg_count; i++) {
        if (i > 1) {
            strcat(expression, " ");
        }
        strcat(expression, cmd->args[i]);
    }
    
    // 计算表达式
    double result;
    int eval_result = evaluate_expression(expression, &result, ctx);
    if (eval_result != 0) {
        // 根据错误类型决定是否显示额外的错误信息
        if (eval_result == XCALC_ERROR_INVALID_FORMAT) {
            XSHELL_LOG_ERROR(ctx, "xcalc: invalid expression '%s'\n", expression);
            XSHELL_LOG_ERROR(ctx, "Try 'xcalc --help' for more information.\n");
        }
        // 对于除零错误和无效操作符，evaluate_expression已经显示了错误信息
        return -1;
    }
    
    // 如果结果是整数，以整数格式输出
    if (result == (int)result) {
        printf("%d\n", (int)result);
    } else {
        printf("%.6g\n", result);
    }
    
    return 0;
}




