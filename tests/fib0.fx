fn fib(n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 2) + fib(n - 1);
}

fn main() {
    return fib(42);
}
