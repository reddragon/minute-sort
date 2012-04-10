#include <iostream>
#include <cstdlib>
#include <fstream>
#include <stdint.h>

typedef uint64_t inttype;

void
generate_test_case(int n, char * file_name) {
    srand(time(0));
    int a1 = rand() % n;
    int a2 = n - a1;
    
    std::ofstream of(file_name);
    of << a1  << " " << a2 << std::endl;
    inttype start = 0;
    for (int i = 0; i < a1; i++) {
       start = start + (rand() % 128);
       of << start << '\n';
    }

    start = 0;
    for (int i = 0; i < a2; i++) {
       start = start + (rand() % 128);
       of << start << '\n';
    }

    of.close();
}


int
main(int argc, char ** argv) {
    generate_test_case(atoi(argv[1]), argv[2]);               
}
