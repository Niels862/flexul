typedef Int like 0;

inline -(x: Int) -> Int: __ineg__(x);
inline +(x: Int): x;
inline +(x, y): __iadd__(x, y);
inline -(x, y): __isub__(x, y);
inline *(x, y): __imul__(x, y);
inline /(x, y): __idiv__(x, y);
inline %(x, y): __imod__(x, y);
inline ==(x, y): __ieq__(x, y);
inline !=(x, y): __ineq__(x, y);
inline <(x, y): __ilt__(x, y);
inline <=(x, y): __ile__(x, y);
inline >(x, y): __ilt__(y, x);
inline >=(x, y): __ile__(y, x);

inline exit(x): __exit__(x);
