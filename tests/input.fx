fn getnum() {
    var c = __getc__();
    var n = 0;
    while (c != 10) {
        if (c >= '0' && c <= '9') {
            n = 10 * n + c - '0';
        }
        c = __getc__();
    }
    return n;
}

fn reverse(x) {
    var rev = 0;
    while (x) {
        rev = 10 * rev + x % 10;
        x = x / 10;
    }
    return rev;
}

fn putnum(x) {
    var rev = reverse(x);
    while (rev) {
        __putc__('0' + rev % 10);
        rev = rev / 10;
    }
}

fn main() {
    var x = getnum();
    putnum(x);
    __putc__('\n');
    return 0;
}
