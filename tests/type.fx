typedef Int like 0;
typedef Float;
typedef Bool like true, false;

fn f(x: Int) {
    return x;
}

fn f(x: Float) {
    return __iadd__(x, 1);
}

fn main() -> Int {
    var i: Int = 4;
    var d: Float;
    d = 4;
    var a: (Int, Int) -> Int = lambda(a: Int, b: Int) -> Int: a;
    return f(d);
}
