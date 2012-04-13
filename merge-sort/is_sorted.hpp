#if !defined IS_SORTED_HPP
#define IS_SORTED_HPP

#include <stdio.h>
#include <iostream>

template <typename Iter>
bool
is_sorted(Iter f, Iter l) {
    Iter next = f;
    int i = 0;
    while (f != l) {
        ++next;
        if (next != l && *f > *next) {
            printf("Mismatch at index %d<->%d ", i, i+1);
            cout<<"("<<*f<<" > "<<*next<<")"<<endl;

            int k = i < 7 ? -i : -7;
            printf("Before: { ");
            while (k != 0) {
                cout<<f[k++]<<",  ";
            }
            printf("}, After: { ");
            k = 0;
            while (k != 7 && f+k != l) {
                cout<<f[k++]<<",  ";
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
