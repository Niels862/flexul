fn main() {
    arr = reserve 32;
    for (i = 0; i < 32; i = i + 1) {
        arr[i] = 2 * i;
    }
    return arr[31];
}
