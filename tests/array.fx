include core;

fn set(arr, i) {
    arr[i] = 15;
}

var arr[16];

fn main() {
    set(arr, 0);
    return arr[0];
}
