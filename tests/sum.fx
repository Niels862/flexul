fn sum(x, y, n) {
    x = y = 0;
    while (x <= n) {
        y = y + x;
        x = x + 1;
    }
    return y;
}

fn main() {
    return sum(0, 0, 100);
}
