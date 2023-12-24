var pool[1024];
var used[256];
var min_free;

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

fn print_used() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        __putc__('0' + used[i]);
    }
    __putc__('\n');
}

fn tree_new(data, left, right) {
    var node = alloc();
    *(node + 1) = data;
    *(node + 2) = left;
    *(node + 3) = right;
    return node;
}

fn tree_print_rec(node) {
    if (node == 0) {
        __putc__('n');
        return 0;
    }
    __putc__(*(node + 1));
    __putc__(':');
    __putc__('[');
    tree_print_rec(*(node + 2));
    __putc__(',');
    tree_print_rec(*(node + 3));
    __putc__(']');
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

fn main() {
    setup();

    var node = tree_new('A', tree_new('B', 0, 0), tree_new('C', 0, tree_new('D', 0, 0)));
    tree_print(node);
    tree_destruct(node);

    check_leaks();

    return 0;
}
