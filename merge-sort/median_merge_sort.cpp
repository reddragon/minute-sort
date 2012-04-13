#include "include.hpp"
#include "median_merge_sort.hpp"
#include "is_sorted.hpp"


void
test(int *a1, int *a2, int i1, int j1, int i2, int j2) {
    size_t s1 = j1-i1, s2=j2-i2;
    vector<int> out(s1 + s2);

    vector<int> v(a1 + i1, a1 + j1);
    v.insert(v.end(), a2 + i2, a2 + j2);
    sort(v.begin(), v.end());

    median_t<int> m = median(a1, a2, i1, j1, i2, j2);

#if 0
    printf("Median: %s[%d] = %d (rank: %d)\n", (m.array == a1 ? "a1" : "a2"), m.index, m(), m.rank);
    printf("Median by sorting: %d, #elements = %d (%d)\n", v[v.size()/2], v.size(), s1+s2);

    if (m.array == a2) {
        std::swap(a1, a2);
        std::swap(i1, i2);
        std::swap(j1, j2);
    }

    size_t i = m.index + 1;
    size_t j = m.rank - (m.index - i1) + i2;

    printf("a1[%d] = %d, a2[%d] = %d\n", i, a1[i], j, a2[j]);
    std::merge(a1 + i1, a1+i,  a2 + i2, a2 + j, out.begin());
    std::merge(a1 + i,  a1+j1, a2 + j,  a2 + j2, out.begin() + m.rank + 1);
#else
    parallel_merge(a1, a2, &(*out.begin()), 0, i1, j1, i2, j2);
#endif
    
    /*
    for (size_t k = 0; k < out.size(); ++k) {
        printf("%2d ", out[k]);
    }
    printf("\n\n");
    */

    for (int i = 0; i < (int)out.size(); i++) {
        assert(out[i] == v[i]);
    }

    assert(out[(s1+s2)/2] == m());
    assert(m.rank == (s1+s2)/2);
    printf("s1: %d, s2: %d, (s1+s2)/2: %d\n", s1, s2, (s1+s2)/2);
    printf("Median: %d, 1 before median: %d, Median by index: %d\n", m(), out[(s1+s2)/2 -1], out[(s1+s2)/2]);
}


int
MAIN(int argc, char *argv[]) {

    if (argc > 1) {
        freopen(argv[1], "r", stdin);
    }

    /*
    int a1[] = { 23, 45, 55, 56, 57, 58, 59, 65, 75, 85 };
    int a2[] = { 10, 20, 30, 40, 50, 60, 70 };
    int s1 = sizeof(a1)/sizeof(int), s2 = sizeof(a2)/sizeof(int);

    printf("A1::%p, A2::%p\n", a1, a2);
    test(a1, a2, 0, s1, 0, s2);
    test(a2, a1, 0, s2, 0, s1);
    test(a1, a2, 1, s1, 1, s2);
    test(a2, a1, 1, s2, 1, s1);
    test(a1, a2, 2, s1, 2, s2);
    test(a2, a1, 2, s2, 2, s1);
    test(a1, a2, 3, s1, 3, s2);
    test(a2, a1, 3, s2, 3, s1);
    */

    vector<inttype> *v1 = new vector<inttype>, *v2 = new vector<inttype>;
    int s1, s2;
    // std::cin >> s1 >> s2;
    scanf("%d %d", &s1, &s2);
    v1->resize(s1);
    v2->resize(s2);
    for (int i = 0; i < s1; i++) {
        scanf("%llu", &((*v1)[i]));
        // std::cin >> v1[i];
    }

    for (int i = 0; i < s2; i++) {
        scanf("%llu", &((*v2)[i]));
        // std::cin >> v2[i];
    }

    inttype* a1 = &v1->front(), *a2 = &v2->front();
#if 0
    test(a1, a2, 0, s1, 0, s2);
#else
    vector<inttype> *out = new vector<inttype>(s1 + s2);
    // parallel_merge(a1, a2, &(*out->begin()), 0, 0, s1, 0, s2);
    // assert(is_sorted(out->begin(), out->end()));
    parallel_merge_sort(a1, &(*out->begin()), 0, s1, 0);
#endif
}
