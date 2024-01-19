fn f(x) {
    return x + 1;
}

fn f(x, y) {
    return f(x) + y;
}

fn main() {
    return f(5, 6);
}
