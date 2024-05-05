include core;

fn f() {
    return 0;
}

fn f(x) {
    return 1;
}

fn f(x, y) {
    return 2;
}

fn main() {
    return f(f() + f(0));
}
