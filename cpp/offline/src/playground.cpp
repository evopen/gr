#include "library.h"

#include <iostream>

int main()
{
    try
    {
        std::mt19937 rng;
        std::uniform_real_distribution<double> uni(-0.001, 0.001);
        gsl_integration_workspace* w = gsl_integration_workspace_alloc(1000);

        auto start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++)
        {
            Integrate(200 + uni(rng) + i, 20, 10);
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++)
        {
            ode23(200 + uni(rng) + i, 20, 1, 10);
        }
        end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;

        start = std::chrono::high_resolution_clock::now();
        for (int i = 0; i < 1000; i++)
        {
            rkf45(200 + uni(rng) + i, 20, 1, 10);
        }
        end = std::chrono::high_resolution_clock::now();
        std::cout << std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() << std::endl;
    }
    catch (std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}
