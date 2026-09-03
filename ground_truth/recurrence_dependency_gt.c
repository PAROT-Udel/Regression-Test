int main() {
    int a[100];
    int i;
    a[0] = 1;
    for (i = 1; i < 100; ++i) {
        a[i] = a[i-1] + 1;
    }
    return 0;
}