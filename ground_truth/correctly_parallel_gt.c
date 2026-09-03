int main() {
  int arr1[100], arr2[100], result[100];
  int i;
  int _ret_val_0;
#pragma cetus private(i)
#pragma loop name main #0
#pragma cetus parallel
  /*
  Disabled due to low profitability: #pragma omp parallel for private(i)
  */
  for (i = 0; i < 100; ++i) {
    arr1[i] = (i * 2);
    arr2[i] = (i + 5);
  }
#pragma cetus private(i)
#pragma loop name main #1
#pragma cetus parallel
  /*
  Disabled due to low profitability: #pragma omp parallel for private(i)
  */
  for (i = 0; i < 100; ++i) {
    result[i] = (arr1[i] + arr2[i]);
  }
  _ret_val_0 = 0;
  return _ret_val_0;
}
