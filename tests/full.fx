fn f(x, y) {
    var a, b = 5;
    to42(&a);
    return (lambda a, b: a + b)(a, b) + x % y;
}

fn to42(x) {
    *x = 42;
}

fn loop(x) {
    var i = 0;
    alias y for x;

    while (x < 1000) {
        y = x * f(x, x / 2);
    }
    if (x % 2) {
        y = x + 1;
    } else {
        y = x - 1;
    }
    return y;
}

# main vvv
fn main() {
    return loop(5);
}
