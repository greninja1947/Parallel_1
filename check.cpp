#include <omp.h>
#include <stdio.h>

int main() {
    printf("Threads: %d\n", omp_get_max_threads());
}
