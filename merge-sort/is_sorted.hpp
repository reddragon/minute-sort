#if !defined IS_SORTED_HPP
#define IS_SORTED_HPP

#include <stdio.h>

template <typename Iter>
bool
is_sorted(Iter f, Iter l) {
    Iter next = f;
    int i = 0;
    while (f != l) {
        ++next;
        if (next != l && *f > *next) {
            printf("Mismatch at index %d<->%d (%llu > %llu)\n", i, i+1, *f, *next);
            int k = i < 7 ? -i : -7;
            printf("Before: { ");
            while (k != 0) {
                printf("%llu,  ", f[k++]);
            }
            printf("}, After: { ");
            k = 0;
            while (k != 7 && f+k != l) {
                printf("%llu,  ", f[k++]);
            }
            printf(" }\n");
            return false;
        }
        ++i;
        f = next;
    }
    return true;
}

#endif // IS_SORTED_HPP
