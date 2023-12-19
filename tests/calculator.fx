fn get_token(token) {
    if (*token == '\n') {
        return 0;
    }
    *token = __getc__();
    while (*token == ' ') {
        *token = __getc__();
    }
}

fn error(arg) {
    __putc__('E');
    __putc__('r');
    __putc__('r');
    __putc__('o');
    __putc__('r');
    __putc__('!');
    __putc__(' ');
    print_number(arg);
    __exit__(1);
}

fn is_digit(token) {
    return *token >= '0' && *token <= '9';
}

fn reverse_number(x, zeros) {
    var rev = 0;
    while (x) {
        var d = x % 10;
        rev = 10 * rev + d;
        x = x / 10;
        if (d == 0 && rev == 0) {
            *zeros = *zeros + 1;
        }
    }
    return rev;
}

fn print_number(x) {
    if (x == 0) {
        __putc__('0');
        __putc__('\n');
        return 0;
    }
    if (x < 0) {
        __putc__('-');
        x = -x;
    }
    var zeros = 0;
    var rev = reverse_number(x, &zeros);
    while (rev) {
        __putc__('0' + rev % 10);
        rev = rev / 10;
    }
    while (zeros > 0) {
        __putc__('0');
        zeros = zeros - 1;
    }
    __putc__('\n');
}

fn parse_sum(token, ans) {
    var left = parse_term(token, ans);
    if (*token == '+') {
        get_token(token);
        return left + parse_term(token, ans);
    }
    if (*token == '-') {
        get_token(token);
        return left - parse_term(token, ans);
    }
    return left;
}

fn parse_term(token, ans) {
    var left = parse_value(token, ans);
    var op = *token;
    while (op == '*' || op == '/' || op == '%') {
        get_token(token);
        var right = parse_value(token, ans);
        if (op == '*') {
            left = left * right;
        } else if (op == '/') {
            left = left / right;
        } else {
            left = left % right;
        }
        op = *token;
    }
    return left;
}

fn parse_value(token, ans) {
    if (is_digit(token)) {
        var num = *token - '0';
        get_token(token);
        while (is_digit(token)) {
            num = 10 * num + *token - '0';
            get_token(token);
        }
        return num;
    }
    if (*token == '(') {
        get_token(token);
        var num = parse_sum(token, ans);
        if (*token == ')') {
            get_token(token);
            return num;
        }
    }
    if (*token == 'x') {
        get_token(token);
        return ans;
    }
    if (*token == '-') {
        get_token(token);
        return -parse_value(token, ans);
    }
    if (*token == '+') {
        get_token(token);
        return parse_value(token, ans);
    }
    error(*token);
}

fn parse(ans) {
    var token = 0;
    get_token(&token);
    if (token == '.') {
        __exit__(0);
    }
    ans = parse_sum(&token, ans);
    if (token != '\n') {
        error(token);
    }
    return ans;
}

fn main() {
    var ans = 0;
    while (1) {
        __putc__('>');
        __putc__(' ');
        ans = parse(ans);
        print_number(ans);
    }
}
