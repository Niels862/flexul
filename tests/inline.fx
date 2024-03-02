include core;

inline add(x, y): x + y;
inline add3(x, y, z): add(add(x, y), z);
inline f(x, y): add3(x, y, add(x, y));

fn main() {
    return f(5, 7);
}
