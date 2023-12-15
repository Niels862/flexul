fn f(x) { return 2 * x; }

fn main() {
    var x = 6;

    alias y for x;
    alias h for f;

    return h(x + y);
}
