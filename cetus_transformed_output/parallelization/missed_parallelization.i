int main()
{
	int A[100], B[100], C[100];
	int i;
	int _ret_val_0;
	#pragma cetus private(i) 
	#pragma loop name main#0 
	#pragma cetus parallel 
	/*
	Disabled due to low profitability: #pragma omp parallel for private(i)
	*/
	for (i=0; i<100;  ++ i)
	{
		B[i]=i;
		C[i]=(i*2);
	}
	#pragma cetus private(i) 
	#pragma loop name main#1 
	#pragma cetus parallel 
	/*
	Disabled due to low profitability: #pragma omp parallel for private(i)
	*/
	for (i=0; i<100;  ++ i)
	{
		A[i]=(B[i]+C[i]);
	}
	_ret_val_0=0;
	return _ret_val_0;
}
