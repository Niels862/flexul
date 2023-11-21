fn func(f) {
    return f()();
}

fn main() {
    return func(lambda: lambda: 2 + 3);
}
