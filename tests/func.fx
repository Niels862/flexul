#!/home/niels/proj/flexul/fx

fn +(x: IStream, y: Int) -> IStream {
    __putc__(y);
    return x;
}

fn main() {
    0 + 'H' + 'e' + 'l' + 'l' + 'o' + '\n';
    return ((lambda(): 5) - 1)();
}
