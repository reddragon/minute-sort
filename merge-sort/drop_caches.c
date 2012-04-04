#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int
main() {
    seteuid(0);
    system("echo '3' > /proc/sys/vm/drop_caches");
    return 0;
}
