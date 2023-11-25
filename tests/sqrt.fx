fn sqrt(n, x) {
    x = n;
    while (x * x > n) {
        x = x - 1;
    }
    return x;
}

fn main() {
    return sqrt(20000, 0);
}
