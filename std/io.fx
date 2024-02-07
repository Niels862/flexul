include core;

inline putc(x): __putc__(x);
inline getc(x): __getc__(x);

fn print_string(s) {
    var i = 0;
    while (1) {
        var c = (*s)[i];
        if (c) {
            putc(c);
        } else {
            return 0;
        }
        i = i + 1;
    }
}

fn print_number(x) {
    var string[12];
    if (x == 0) {
        putc('0');
        putc('\n');
        return 0;
    }
    if (x < 0) {
        putc('-');
        x = -x;
    }
    string[11] = '\0';
    var i = 10;
    while (x) {
        var d = x % 10;
        x = x / 10;
        string[i] = 48 + d;
        i = i - 1;
    }
    print_string(&string + i + 1);
    putc('\n');
}
