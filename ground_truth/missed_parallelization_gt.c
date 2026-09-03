// ground_truth/parallelization/missed_parallelization_gt.c

int main()
{
    int A[100], B[100], C[100];
    int i;
    int _ret_val_0; // Cetus often adds this

    // Cetus might or might not parallelize the initialization loop.
    // For this ground truth, let's assume it doesn't, but keeps internal pragmas.
    #pragma cetus private(i) // Cetus's internal pragma
    #pragma loop name main#0 // Cetus's internal pragma
    for (i = 0; i < 100; ++i) {
        B[i] = i;
        C[i] = i * 2;
    }

    // THIS IS THE CRUCIAL PART FOR THE TEST
    // Cetus *should* insert this OpenMP pragma for parallelization
    #pragma cetus private(i)
    #pragma omp parallel for // <-- This is the expected parallelization pragma!
    #pragma loop name main#1 // Cetus's internal pragma for the second loop
    for (i = 0; i < 100; ++i) {
        A[i] = (B[i] + C[i]);
    }

    _ret_val_0 = 0; // Cetus often adds this
    return _ret_val_0;
}