fn number() {
    var c = __getc__();
    var n = 0;
    while (c != 10) {
        if (c >= 48 && c <= 58) {
            n = 10 * n + c - 48;
        }
        c = __getc__();
    }
    return n;
}

fn main() {
    return number();
}
