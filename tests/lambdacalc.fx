include core;
include io;

var pool[1024];
var used[256];
var min_free;
var token;

fn setup() {
    var i;
    for (i = 0; i < 256; i++) {
        used[i] = 0;
    }
    min_free = 0;
}

fn alloc() {
    var i;
    for (i = min_free; i < 256; i++) {
        if (used[i] == 0) {
            used[i] = 1;
            min_free = i + 1;
            return pool + 4 * i;
        }
    }
    exit(1);
}

fn free(p) {
    var i = (p - pool) / 4;
    if (i < 0 || i >= 256) {
        exit(1);
    }
    used[i] = 0;
    if (i < min_free) {
        min_free = i;
    }
}

inline tree_type(node): *node;

inline tree_data(node): *(node + 1);

inline tree_left(node): *(node + 2);

inline tree_right(node): *(node + 3);

fn tree_new(type, data, left, right) {
    var node = alloc();
    *(node) = type;
    *(node + 1) = data;
    *(node + 2) = left;
    *(node + 3) = right;
    return node;
}

inline tree_abstraction(ident, body): tree_new(0, 0, tree_new(2, ident, 0, 0), body);

inline tree_application(left, right): tree_new(1, 0, left, right);

inline tree_variable(ident): tree_new(2, ident, 0, 0);

fn tree_print_rec(node) {
    var type = tree_type(node);
    if (type == 0) {
        putc('(');
        putc('\\');
        tree_print_rec(tree_left(node));
        putc(' ');
        tree_print_rec(tree_right(node));
        putc(')');
        return 0;
    }
    if (type == 1) {
        putc('(');
        tree_print_rec(tree_left(node));
        putc(' ');
        tree_print_rec(tree_right(node));
        putc(')');
        return 0;
    }
    putc(tree_data(node));    
}

fn tree_print(node) {
    tree_print_rec(node);
    putc('\n');
}

fn tree_destruct(node) {
    if (node == 0) {
        return 0;
    }
    free(node);
    tree_destruct(tree_left(node));
    tree_destruct(tree_right(node));
}

fn check_leaks() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        if (used[i]) {
            exit(2);
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
    token = getc();
    while (token == ' ') {
        token = getc();
    }
    return token;
}

fn expect_token(expected) {
    var saved = token;
    if (token == expected) {
        next_token();
        return saved;
    }
    exit(1);
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
    exit(1);
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
    putc('>');
    putc(' ');
    var node = parse();
    tree_print(node);
    tree_destruct(node);

    check_leaks();

    return 0;
}
