include core;

fn fac(n) {
    return n ? n * fac(n - 1) : 1;
}

fn main() {
    return fac(12);
}
