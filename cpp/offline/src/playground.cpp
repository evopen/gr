#include <iostream>
#include <omp.h>
using namespace std;

int main()
{
#if _OPENMP
    cout << " support openmp " << endl;
#else
    cout << " not support openmp" << endl;
#endif

#pragma omp parallel for num_threads(4)  // NEW ADD
    for (int i = 0; i < 10; i++)
    {
        cout << i << endl;
    }
    return 0;
}
