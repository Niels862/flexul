include core;

var arr[100];

fn fib(n) {
    if (n <= 1) {
        return n;
    }
    if (arr[n] != -1) {
        return arr[n];
    }
    arr[n] = fib(n - 1) + fib(n - 2);
    return arr[n];
}

fn main() {
    var i;
    for (i = 0; i < 100; i = i + 1) {
        arr[i] = -1;
    }
    return fib(2);
}
