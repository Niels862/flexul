fn func(f, x) {
    var z = 6;
    return f(x, z);
}

fn main() {
    return func(lambda a, b: a + b, 2);
}
