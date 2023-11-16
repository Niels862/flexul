fn add3(a, b, c) {
    return add(a, add(b, c));
}

fn add(a, b) {
    return a + b;
}

fn main() {
    return add3(1, 2, 3);
}
