// input_files/parallelization/recurrence_dependency.c

int main() {
    int a[100];
    int i;

    // Initialize the first element
    a[0] = 1;

    // This loop has a loop-carried flow dependency (a[i] depends on a[i-1])
    // The value of a[i] in the current iteration depends on the value
    // of a[i-1] which was written in the *previous* iteration.
    // Therefore, this loop is inherently sequential and cannot be parallelized.
    for (i = 1; i < 100; ++i) {
        a[i] = a[i-1] + 1;
    }

    return 0;
}