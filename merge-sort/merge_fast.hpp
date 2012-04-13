#if !defined MERGE_FAST_HPP
#define MERGE_FAST_HPP

#include <algorithm>

// Assume that f1..l1 & f2..l2 are contiguous buffer laid out one
// after another.
template <typename T>
void
merge_fast(T *f1, T *l1, T *f2, T *l2, T *buff) {
    // copy [f1..l1) to buff
    std::copy(f1, l1, buff);
    T *bend = buff + (l1 - f1);
    while (f2 != l2 && buff != bend) {
        if (*buff < *f2) {
            *f1++ = *buff++;
        } else {
            *f1++ = *f2++;
        }
    }
    while (f2 != l2) {
        *f1++ = *f2++;
    }
    while (buff != bend) {
        *f1++ = *buff++;
    }
}

#endif // MERGE_FAST_HPP
