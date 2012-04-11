#include <iostream>
#include <cstdio>
#include <vector>
#include <iterator>
#include <algorithm>
using namespace std;

#define BASE_CASE 1
void 
bottom_up_merge_sort(vector<int> &v) {
    size_t s = v.size();
    vector<int> scratch(s);
    
    // Assuming log(s) < 100
    int chunk_size[100], chunk_start[100];
    int top = 0, i = 0;
    chunk_size[top] = 1;
    chunk_start[top] = 0;
    

    while(chunk_size[0] < s) {
        i++;
        top++;
        chunk_size[top] = 1;
        chunk_start[top] = chunk_start[top-1] + chunk_size[top-1];

        while (top-1 >= 0 && (i == s || chunk_size[top] == chunk_size[top-1])) {
            // Merge top and top-1
            int p1 = 0, p2 = 0, s1 = chunk_size[top-1], s2 = chunk_size[top];
            int cs1 = chunk_start[top-1], cs2 = chunk_start[top];
            int t = 0;

            while (t < s1 + s2) {
                if (p1 == s1) {
                    for (; p2 < s2; p2++)
                        scratch[cs1+t++] = v[cs2+p2];        
                }
                else if (p2 == s2) {
                    for(; p1 < s1; p1++)
                        scratch[cs1+t++] = v[cs1+p1];
                }
                else {
                    if (v[cs1+p1] < v[cs2+p2])
                        scratch[cs1+t++] = v[cs1+p1++];
                    else
                        scratch[cs1+t++] = v[cs2+p2++];
                }
            }
            // Copy from the scratch to the array
            std::copy(scratch.begin() + cs1, scratch.begin() + cs1 + t, v.begin() + cs1);
            chunk_size[top-1] += chunk_size[top];
            top--;
        }
    }
}

int
main() {
    vector<int> r;
    for (int i = 0; i < 100; i++)
        r.push_back(100-i);
    bottom_up_merge_sort(r);
    for (int i = 0; i < r.size(); i++)
        std::cout << r[i] << " ";
    std::cout << "\n";
}
