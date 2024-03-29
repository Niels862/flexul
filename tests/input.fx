include core;
include io;

fn getnum() {
    var c = __getc__();
    var n = 0;
    while (c != '\n') {
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
    if (x == 0) {
        __putc__('0');
        return 0;
    }
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
    var i;
    for (i = 0; i < x; i = i + 1) {
        putnum(i);
        __putc__('\n');
    }
    return 0;
}
