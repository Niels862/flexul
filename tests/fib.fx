fn fib(n) {
    return n ? (n - 1 ? fib(n - 2) + fib(n - 1) : 1) : 0;
}

fn main() {
    return fib(34);
}
