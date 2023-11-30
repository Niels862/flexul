fn sum(n) {
    var x = 0;
    var y = 0;
    while (x <= n) {
        y = y + x;
        x = x + 1;
    }
    return y;
}

fn main() {
    return sum(100);
}
