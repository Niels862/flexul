include io;

inline writeback +=(x, y): __iadd__(x, y);

inline writeback ++(x): __iadd__(x, 1);

fn main() {
    var a = 4;
    a += 7;
    a++;
    return a;
}
