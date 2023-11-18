fn f() {
    return g() + g();
}

fn main() {
    return f();
}

fn g() {
    return 21;
}
