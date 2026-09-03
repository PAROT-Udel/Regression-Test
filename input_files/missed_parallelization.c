// input_files/parallelization/missed_parallelization.c

int main() {
    int A[100], B[100], C[100];
    int i;

    // Initialize arrays (optional, but good practice for completeness)
    for (i = 0; i < 100; ++i) {
        B[i] = i;
        C[i] = i * 2;
    }

    // This loop is perfectly parallelizable.
    // Each iteration (i) only reads/writes A[i], B[i], C[i],
    // so there are no dependencies between iterations.
    for (i = 0; i < 100; ++i) {
        A[i] = B[i] + C[i];
    }

    return 0;
}