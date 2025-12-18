#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

static int evaluate_expression(const char *expr, double *result) {
    // 创建副本并去除引号
    char clean_expr[1024];
    strncpy(clean_expr, expr, sizeof(clean_expr) - 1);
    clean_expr[sizeof(clean_expr) - 1] = '\0';
    trim_quotes(clean_expr);
    
    printf("Original: [%s]\n", expr);
    printf("Cleaned:  [%s]\n", clean_expr);
    
    // 简单实现：只支持 num1 op num2 格式
    double num1, num2;
    char op;
    int matched = sscanf(clean_expr, "%lf %c %lf", &num1, &op, &num2);
    
    if (matched != 3) {
        // 尝试单个数字
        matched = sscanf(clean_expr, "%lf", &num1);
        if (matched == 1) {
            *result = num1;
            return 0;
        }
        return -1;
    }
    
    // 计算结果
    switch (op) {
        case '+': *result = num1 + num2; break;
        case '-': *result = num1 - num2; break;
        case '*': case 'x': case 'X': *result = num1 * num2; break;
        case '/': 
            if (num2 == 0) return -1;
            *result = num1 / num2; 
            break;
        case '%': 
            if (num2 == 0) return -1;
            *result = (int)num1 % (int)num2; 
            break;
        default: return -1;
    }
    
    return 0;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <expression>\n", argv[0]);
        return 1;
    }
    
    // 将所有参数合并
    char expression[1024] = "";
    for (int i = 1; i < argc; i++) {
        if (i > 1) strcat(expression, " ");
        strcat(expression, argv[i]);
    }
    
    double result;
    if (evaluate_expression(expression, &result) != 0) {
        printf("Invalid expression: %s\n", expression);
        return 1;
    }
    
    if (result == (int)result) {
        printf("%d\n", (int)result);
    } else {
        printf("%.6g\n", result);
    }
    
    return 0;
}
