var pool[1024];
var used[256];
var min_free;
var token;

fn setup() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        used[i] = 0;
    }
    min_free = 0;
}

fn alloc() {
    var i;
    for (i = min_free; i < 256; i = i + 1) {
        if (used[i] == 0) {
            used[i] = 1;
            min_free = i + 1;
            return &pool + 4 * i;
        }
    }
    __exit__(1);
}

fn free(p) {
    var i = (p - &pool) / 4;
    if (i < 0 || i >= 256) {
        __exit__(1);
    }
    used[i] = 0;
    if (i < min_free) {
        min_free = i;
    }
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

fn tree_type(node) {
    return *node;
}

fn tree_data(node) {
    return *(node + 1);
}

fn tree_left(node) {
    return *(node + 2);
}

fn tree_right(node) {
    return *(node + 3);
}

fn tree_new(type, data, left, right) {
    var node = alloc();
    *(node) = type;
    *(node + 1) = data;
    *(node + 2) = left;
    *(node + 3) = right;
    return node;
}

fn tree_abstraction(ident, body) {
    return tree_new(0, 0, tree_new(2, ident, 0, 0), body);
}

fn tree_application(left, right) {
    return tree_new(1, 0, left, right);
}

fn tree_variable(ident) {
    return tree_new(2, ident, 0, 0);
}

fn tree_print_rec(node) {
    var type = tree_type(node);
    if (type == 0) {
        __putc__('(');
        __putc__('\\');
        tree_print_rec(tree_left(node));
        __putc__(' ');
        tree_print_rec(tree_right(node));
        __putc__(')');
        return 0;
    }
    if (type == 1) {
        __putc__('(');
        tree_print_rec(tree_left(node));
        __putc__(' ');
        tree_print_rec(tree_right(node));
        __putc__(')');
        return 0;
    }
    __putc__(tree_data(node));    
}

fn tree_print(node) {
    tree_print_rec(node);
    __putc__('\n');
}

fn tree_destruct(node) {
    if (node == 0) {
        return 0;
    }
    free(node);
    tree_destruct(*(node + 2));
    tree_destruct(*(node + 3));
}

fn check_leaks() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        if (used[i]) {
            __exit__(2);
        }
    }
}

fn is_lower(c) {
    return c >= 'a' && c <= 'z';
}

fn next_token() {
    if (token == '\n') {
        return '\n';
    }
    token = __getc__();
    while (token == ' ') {
        token = __getc__();
    }
    return token;
}

fn expect_token(expected) {
    var saved = token;
    if (token == expected) {
        next_token();
        return saved;
    }
    __exit__(1);
}

fn accept_token(accepted) {
    var saved = token;
    if (token == accepted) {
        next_token();
        return saved;
    }
    return 0;
}

fn assert_token(f) {
    var saved = token;
    if (f(token)) {
        next_token();
        return saved;
    }
    __exit__(1);
}

fn parse_application() {
    var left = parse_abstraction();
    while (token != '\n' && token != ')') {
        left = tree_application(left, parse_abstraction());
    }
    return left;
}

fn parse_abstraction() {
    if (accept_token('\\')) {
        var ident = assert_token(is_lower);
        if (accept_token('.')) {
            return tree_abstraction(ident, parse_application());
        }
        return tree_abstraction(ident, parse_body());
    }
    return parse_body();
}

fn parse_body() {
    if (accept_token('(')) {
        var node = parse_application();
        expect_token(')');
        return node;
    }
    if (token == '\\') {
        return parse_abstraction();
    }
    return tree_variable(assert_token(is_lower));
}

fn parse() {
    token = 0;
    next_token();
    var node = parse_application();
    return node;
}

fn main() {
    setup();

    var node = parse();
    tree_print(node);
    tree_destruct(node);

    check_leaks();

    return 0;
}
