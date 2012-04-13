#include <iostream>
#include <cstdio>
#include <vector>
#include <iterator>
#include <algorithm>
#include <cstdio>
#include <cassert>
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
    

    while(chunk_size[0] < (int)s) {
        i++;
        top++;
        chunk_size[top] = 1;
        chunk_start[top] = chunk_start[top-1] + chunk_size[top-1];

        while (top-1 >= 0 && (i == (int)s || chunk_size[top] == chunk_size[top-1])) {
            // Merge top and top-1
            int s1 = chunk_size[top-1], s2 = chunk_size[top];
            int cs1 = chunk_start[top-1], cs2 = chunk_start[top];
            std::merge(v.begin() + cs1, v.begin() + cs1 + s1, v.begin() + cs2, v.begin() + cs2 + s2, scratch.begin());
            // Copy from the scratch to the array
            std::copy(scratch.begin(), scratch.begin() + s1 + s2, v.begin() + cs1);
            chunk_size[top-1] += chunk_size[top];
            top--;
        }
    }
}

int
main(int argc, char ** argv) {
    vector<int> r;
    r.resize(30000000);
    bottom_up_merge_sort(r);
}
