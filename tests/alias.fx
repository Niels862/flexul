fn f(x) { return 2 * x; }

fn main() {
    var x = 6;
    alias g for f, h for g;
    return h(x + y);
}
