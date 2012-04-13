#if !defined INCLUDE_HPP
#define INCLUDE_HPP

#include <iostream>
#include <vector>
#include <algorithm>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>


using namespace std;

typedef uint64_t inttype;

#if !defined __cilkplusplus
#define cilk_spawn
#define cilk_sync
#define MAIN main
#else
#include <cilkview.h>
#define MAIN cilk_main
#endif

#endif // INCLUDE_HPP
