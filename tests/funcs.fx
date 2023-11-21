fn f() {
    return g(1, 2) + g(3, 4);
}

fn main() {
    return f();
}

fn g(a, b) {
    a = b = 5;
    return h() + a + b;
}

fn h() {
    return 2 * 5;
}
