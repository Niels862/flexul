fn sum(x, y, n) {
    y = 0;
    for (x = 0; x <= n; x = x + 1) {
        y = y + x;
    }
    return y;
}

fn main() {
    return sum(0, 0, 100);
}
