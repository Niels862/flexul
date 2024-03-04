include core;

inline putc(x): __putc__(x);
inline getc(): __getc__();

fn print_string(str) {
    var i = 0;
    while (1) {
        var c = str[i];
        if (c) {
            putc(c);
        } else {
            return 0;
        }
        i = i + 1;
    }
}

fn print_number(x) {
    var str[12];
    if (x == 0) {
        putc('0');
        putc('\n');
        return 0;
    }
    if (x < 0) {
        putc('-');
        x = -x;
    }
    str[11] = '\0';
    var i = 10;
    while (x) {
        var d = x % 10;
        x = x / 10;
        str[i] = 48 + d;
        i = i - 1;
    }
    print_string(str + i + 1);
    putc('\n');
}
