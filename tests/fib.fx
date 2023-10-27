fn fib(n) {
    if (n == 0 || n == 1) {
        return n;
    }
    return f(n - 2) + f(n - 1);
}

fn main() {
    return fib(6);
}
