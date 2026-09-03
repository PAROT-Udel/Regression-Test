// input_files/parallelization/correctly_parallel.c

int main() {
    int arr1[100], arr2[100], result[100];
    int i;

    // Initialize arrays
    for (i = 0; i < 100; ++i) {
        arr1[i] = i * 2;
        arr2[i] = i + 5;
    }

    // This loop is perfectly parallelizable (element-wise addition)
    for (i = 0; i < 100; ++i) {
        result[i] = arr1[i] + arr2[i];
    }

    return 0;
}