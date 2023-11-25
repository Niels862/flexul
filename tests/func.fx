fn f(x) {
    return x + (lambda: 1)();
}

fn main() {
    return f(5) && 0;
}
