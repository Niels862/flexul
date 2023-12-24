var pool[1024];
var used[256];
var min_free;

fn setup() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        used[i] = 0;
    }
    min_free = 0;
}

fn alloc() {
    var i;
    for (i = min_free; i < 256; i = i + 1) {
        if (used[i] == 0) {
            used[i] = 1;
            min_free = i + 1;
            return &pool + 4 * i;
        }
    }
    __exit__(1);
}

fn free(p) {
    var i = (p - &pool) / 4;
    if (i < 0 || i >= 256) {
        __exit__(1);
    }
    used[i] = 0;
    if (i < min_free) {
        min_free = i;
    }
}

fn print_used() {
    var i;
    for (i = 0; i < 256; i = i + 1) {
        __putc__('0' + used[i]);
    }
    __putc__('\n');
}

fn main() {
    setup();

    var a = alloc();

    *a = 42;

    free(a);

    return *alloc();
}
