var arr[5];

fn main() {
    var i, sum = 0;
    for (i = 0; i < 5; i = i + 1) {
        arr[i] = i;
    }
    for (i = 0; i < 5; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
}
