fn f() {
    return g(1, 2) + g(3, 4);
}

fn main() {
    return f();
}

fn g(a, b) {
    return h() + a + b;
}

fn h() {
    return 2 * 5;
}
