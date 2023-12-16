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

fn reverse(x, zeros) {
    var rev = 0;
    while (x) {
        var d = x % 10;
        rev = 10 * rev + d;
        x = x / 10;
        if (d == 0 && rev == 0) {
            *zeros = *zeros + 1;
        }
    }
    return rev;
}

fn putnum(x) {
    var zeros = 0;
    var rev = reverse(x, &zeros);
    while (rev) {
        __putc__('0' + rev % 10);
        rev = rev / 10;
    }
    while (zeros > 0) {
        __putc__('0');
        zeros = zeros - 1;
    }
}

fn main() {
    var x = getnum();
    putnum(x);
    __putc__('\n');
    return 0;
}
