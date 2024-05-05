typedef Int like 0;
typedef Float;
typedef Bool like true, false;

fn f(x: Int) {
    return x;
}

fn f(x: Float) {
    return 0;
}

fn main() -> Int {
    var i: Int = 4;
    return &i;
}
