int main() {
    int A[100], B[100], C[100];
    int i;
    for (i = 0; i < 100; ++i) {
        B[i] = i;
        C[i] = i * 2;
    }
    for (i = 0; i < 100; ++i) {
        A[i] = B[i] + C[i];
    }
    return 0;
}
