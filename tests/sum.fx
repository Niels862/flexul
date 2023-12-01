fn acc(start, end, f) {
    var a = 0;
    var x;
    for (x = start; x <= end; x = x + 1) {
        a = f(a, x);
    }
    return a;
}

fn main() {
    return acc(1, 10000, lambda a, x: a + x);
}
