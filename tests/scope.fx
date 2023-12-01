fn main() {
    var x = 5;
    {
        var x = 42;
    }
    return x;
}
