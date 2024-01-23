include core;

inline putc(x): __putc__(x);
inline getc(x): __getc__(x);

fn print_number(x) {
    if (x == 0) {
        putc('0');
        putc('\n');
        return 0;
    }
    if (x < 0) {
        putc('-');
        x = -x;
    }
    var zeros = 0;
    var rev = 0;
    while (x) {
        var d = x % 10;
        rev = 10 * rev + d;
        x = x / 10;
        if (d == 0 && rev == 0) {
            zeros = zeros + 1;
        }
    }
    while (rev) {
        putc('0' + rev % 10);
        rev = rev / 10;
    }
    while (zeros > 0) {
        putc('0');
        zeros = zeros - 1;
    }
    putc('\n');
}
