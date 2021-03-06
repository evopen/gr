#include "library.h"
#include "../../rkf45/rkf45.h"

#include <benchmark/benchmark.h>
#include <boost/math/tools/roots.hpp>

using boost::math::tools::bisect;
using boost::math::tools::bracket_and_solve_root;

static void BM_Geodesic(benchmark::State& state)
{
    for (auto _ : state)
    {
        Geodesic(state.max_iterations, state.max_iterations);
    }
}

static void BM_bisection(benchmark::State& state)
{
    int digits = std::numeric_limits<double>::digits;
    boost::math::tools::eps_tolerance<double> tol((digits * 3) / 4);
    double r0 = 20.;
    double b  = 10.;
    for (auto _ : state)
    {

        std::pair<double, double> result = bisect([b](double r) { return r / std::sqrt(1 - 2 / r) - b; }, 2.1, r0, tol);
        double root                      = (result.first + result.second) / 2;
    }
}

static void BM_bracket_and_solve_root(benchmark::State& state)
{
    int digits = std::numeric_limits<double>::digits;
    boost::math::tools::eps_tolerance<double> tol((digits * 3) / 4);

    for (auto _ : state)
    {
        // boost::math::tools::bracket_and_solve_root(&equation, 20, 2, false, tol, it);

        double r0           = 20.;
        double b            = 10.;
        boost::uintmax_t it = 1000000;
        std::pair<double, double> result =
            bracket_and_solve_root([b](double r) { return r / std::sqrt(1 - 2 / r) - b; }, r0, 2.0, true, tol, it);
        double root = (result.first + result.second) / 2;
    }
}

static void BM_integrate(benchmark::State& state)
{
    gsl_integration_workspace* w = gsl_integration_workspace_alloc(1000);
    for (auto _ : state)
    {
        Integrate(200, 20, 10, w);
    }
}



static void BM_ode23(benchmark::State& state)
{
    for (auto _ : state)
    {
        ode23(200, 20, 1, 10);
    }
}


static void BM_rkf45(benchmark::State& state)
{
    for (auto _ : state)
    {
        rkf45(200, 20, 1, 10);
    }
}

void geo(double r, double y[], double yp[])
{
    double b = 10;
    yp[0]    = 1 / (r * r * sqrt(1 / (b * b) - 1 / (r * r) + 2 / (r * r * r)));

    return;
}

double rkf45_integrate(double start, double end)
{
    double abserr = 1e-5;
    int flag;
    double relerr = 1e-5;
    double t;
    double t_out;
    double y[1];
    double yp[1];

    flag = 1;

    y[0] = 0.0;

    t = start;
    t_out = end;

    r8_rkf45(geo, 1, y, yp, &t, t_out, &relerr, abserr, flag);
    return y[0];
}

static void BM_r8_rkf45(benchmark::State& state)
{
    for (auto _ : state)
    {
        rkf45_integrate(2000, 20);
    }
}


BENCHMARK(BM_Geodesic);
BENCHMARK(BM_bisection);
BENCHMARK(BM_bracket_and_solve_root);
BENCHMARK(BM_integrate);
BENCHMARK(BM_ode23);
BENCHMARK(BM_rkf45);
BENCHMARK(BM_r8_rkf45);

BENCHMARK_MAIN();