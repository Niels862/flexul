fn fib(n) {
    if (n != 0 && n != 1) {
        return fib(n - 1) + fib(n - 2);
    } else {
        return n;
    }
}

fn main() {
    return fib(34);
}
