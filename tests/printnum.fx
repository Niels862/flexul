
fn reverse_number(x, zeros) {
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

fn print_number(x) {
    if (x == 0) {
        __putc__('0');
        __putc__('\n');
        return 0;
    }
    if (x < 0) {
        __putc__('-');
        x = -x;
    }
    var zeros = 0;
    var rev = reverse_number(x, &zeros);
    while (rev) {
        __putc__('0' + rev % 10);
        rev = rev / 10;
    }
    while (zeros > 0) {
        __putc__('0');
        zeros = zeros - 1;
    }
    __putc__('\n');
}