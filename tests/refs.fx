fn div(a, b) {
    swap(&a, &b, 0);
    return b / a;
}

fn swap(px, py, t) {
    t = *px;
    *px = *py;
    *py = t;
}

fn main() {
    return div(32, 8);
}
