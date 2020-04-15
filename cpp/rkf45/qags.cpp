#include <iostream>
#include <cmath>
#include <limits>

static inline int test_positivity(double result, double resabs)
{
    int status = (fabs(result) >= (1 - 50 * DBL_EPSILON) * resabs);

    return status;
}

/* integration/qelg.c
 *
 * Copyright (C) 1996, 1997, 1998, 1999, 2000, 2007 Brian Gough
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

struct extrapolation_table
{
    size_t n;
    double rlist2[52];
    size_t nres;
    double res3la[3];
};

static void initialise_table(struct extrapolation_table* table);

static void append_table(struct extrapolation_table* table, double y);

static void initialise_table(struct extrapolation_table* table)
{
    table->n    = 0;
    table->nres = 0;
}
#ifdef JUNK
static void initialise_table(struct extrapolation_table* table, double y)
{
    table->n         = 0;
    table->rlist2[0] = y;
    table->nres      = 0;
}
#endif
static void append_table(struct extrapolation_table* table, double y)
{
    size_t n;
    n                = table->n;
    table->rlist2[n] = y;
    table->n++;
}

/* static inline void
   qelg (size_t * n, double epstab[],
   double * result, double * abserr,
   double res3la[], size_t * nres); */

static inline void qelg(struct extrapolation_table* table, double* result, double* abserr);

static inline void qelg(struct extrapolation_table* table, double* result, double* abserr)
{
    double* epstab = table->rlist2;
    double* res3la = table->res3la;
    const size_t n = table->n - 1;

    const double current = epstab[n];

    double absolute = DBL_MAX;
    double relative = 5 * DBL_EPSILON * fabs(current);

    const size_t newelm = n / 2;
    const size_t n_orig = n;
    size_t n_final      = n;
    size_t i;

    const size_t nres_orig = table->nres;

    *result = current;
    *abserr = DBL_MAX;

    if (n < 2)
    {
        *result = current;
        *abserr = std::fmax(absolute, relative);
        return;
    }

    epstab[n + 2] = epstab[n];
    epstab[n]     = DBL_MAX;

    for (i = 0; i < newelm; i++)
    {
        double res = epstab[n - 2 * i + 2];
        double e0  = epstab[n - 2 * i - 2];
        double e1  = epstab[n - 2 * i - 1];
        double e2  = res;

        double e1abs  = fabs(e1);
        double delta2 = e2 - e1;
        double err2   = fabs(delta2);
        double tol2   = std::fmax(fabs(e2), e1abs) * DBL_EPSILON;
        double delta3 = e1 - e0;
        double err3   = fabs(delta3);
        double tol3   = std::fmax(e1abs, fabs(e0)) * DBL_EPSILON;

        double e3, delta1, err1, tol1, ss;

        if (err2 <= tol2 && err3 <= tol3)
        {
            /* If e0, e1 and e2 are equal to within machine accuracy,
               convergence is assumed.  */

            *result  = res;
            absolute = err2 + err3;
            relative = 5 * DBL_EPSILON * fabs(res);
            *abserr  = std::fmax(absolute, relative);
            return;
        }

        e3                = epstab[n - 2 * i];
        epstab[n - 2 * i] = e1;
        delta1            = e1 - e3;
        err1              = fabs(delta1);
        tol1              = std::fmax(e1abs, fabs(e3)) * DBL_EPSILON;

        /* If two elements are very close to each other, omit a part of
           the table by adjusting the value of n */

        if (err1 <= tol1 || err2 <= tol2 || err3 <= tol3)
        {
            n_final = 2 * i;
            break;
        }

        ss = (1 / delta1 + 1 / delta2) - 1 / delta3;

        /* Test to detect irregular behaviour in the table, and
           eventually omit a part of the table by adjusting the value of
           n. */

        if (fabs(ss * e1) <= 0.0001)
        {
            n_final = 2 * i;
            break;
        }

        /* Compute a new element and eventually adjust the value of
           result. */

        res               = e1 + 1 / ss;
        epstab[n - 2 * i] = res;

        {
            const double error = err2 + fabs(res - e2) + err3;

            if (error <= *abserr)
            {
                *abserr = error;
                *result = res;
            }
        }
    }

    /* Shift the table */

    {
        const size_t limexp = 50 - 1;

        if (n_final == limexp)
        {
            n_final = 2 * (limexp / 2);
        }
    }

    if (n_orig % 2 == 1)
    {
        for (i = 0; i <= newelm; i++)
        {
            epstab[1 + i * 2] = epstab[i * 2 + 3];
        }
    }
    else
    {
        for (i = 0; i <= newelm; i++)
        {
            epstab[i * 2] = epstab[i * 2 + 2];
        }
    }

    if (n_orig != n_final)
    {
        for (i = 0; i <= n_final; i++)
        {
            epstab[i] = epstab[n_orig - n_final + i];
        }
    }

    table->n = n_final + 1;

    if (nres_orig < 3)
    {
        res3la[nres_orig] = *result;
        *abserr           = DBL_MAX;
    }
    else
    { /* Compute error estimate */
        *abserr = (fabs(*result - res3la[2]) + fabs(*result - res3la[1]) + fabs(*result - res3la[0]));

        res3la[0] = res3la[1];
        res3la[1] = res3la[2];
        res3la[2] = *result;
    }

    /* In QUADPACK the variable table->nres is incremented at the top of
       qelg, so it increases on every call. This leads to the array
       res3la being accessed when its elements are still undefined, so I
       have moved the update to this point so that its value more
       useful. */

    table->nres = nres_orig + 1;

    *abserr = std::fmax(*abserr, 5 * DBL_EPSILON * fabs(*result));

    return;
}



static int qags(const gsl_function* f, const double a, const double b, const double epsabs, const double epsrel,
    const size_t limit, gsl_integration_workspace* workspace, double* result, double* abserr, gsl_integration_rule* q)
{
    double area, errsum;
    double res_ext, err_ext;
    double result0, abserr0, resabs0, resasc0;
    double tolerance;

    double ertest                     = 0;
    double error_over_large_intervals = 0;
    double reseps = 0, abseps = 0, correc = 0;
    size_t ktmin       = 0;
    int roundoff_type1 = 0, roundoff_type2 = 0, roundoff_type3 = 0;
    int error_type = 0, error_type2 = 0;

    size_t iteration = 0;

    int positive_integrand     = 0;
    int extrapolate            = 0;
    int disallow_extrapolation = 0;

    struct extrapolation_table table;

    *result = 0;
    *abserr = 0;

    /* Test on accuracy */

    if (epsabs <= 0 && (epsrel < 50 * DBL_EPSILON || epsrel < 0.5e-28))
    {
        GSL_ERROR("tolerance cannot be achieved with given epsabs and epsrel", GSL_EBADTOL);
    }

    /* Perform the first integration */

    q(f, a, b, &result0, &abserr0, &resabs0, &resasc0);

    set_initial_result(workspace, result0, abserr0);

    tolerance = std::fmax(epsabs, epsrel * std::abs(result0));

    if (abserr0 <= 100 * DBL_EPSILON * resabs0 && abserr0 > tolerance)
    {
        *result = result0;
        *abserr = abserr0;

        throw std::runtime_error("cannot reach tolerance because of roundoff error"
                                 "on first attempt");
    }
    else if ((abserr0 <= tolerance && abserr0 != resasc0) || abserr0 == 0.0)
    {
        *result = result0;
        *abserr = abserr0;

        return GSL_SUCCESS;
    }
    else if (limit == 1)
    {
        *result = result0;
        *abserr = abserr0;

        throw std::runtime_error("a maximum of one iteration was insufficient");
    }

    /* Initialization */

    initialise_table(&table);
    append_table(&table, result0);

    area   = result0;
    errsum = abserr0;

    res_ext = result0;
    err_ext = DBL_MAX;

    positive_integrand = test_positivity(result0, resabs0);

    iteration = 1;

    do
    {
        size_t current_level;
        double a1, b1, a2, b2;
        double a_i, b_i, r_i, e_i;
        double area1 = 0, area2 = 0, area12 = 0;
        double error1 = 0, error2 = 0, error12 = 0;
        double resasc1, resasc2;
        double resabs1, resabs2;
        double last_e_i;

        /* Bisect the subinterval with the largest error estimate */

        retrieve(workspace, &a_i, &b_i, &r_i, &e_i);

        current_level = workspace->level[workspace->i] + 1;

        a1 = a_i;
        b1 = 0.5 * (a_i + b_i);
        a2 = b1;
        b2 = b_i;

        iteration++;

        q(f, a1, b1, &area1, &error1, &resabs1, &resasc1);
        q(f, a2, b2, &area2, &error2, &resabs2, &resasc2);

        area12   = area1 + area2;
        error12  = error1 + error2;
        last_e_i = e_i;

        /* Improve previous approximations to the integral and test for
           accuracy.
           We write these expressions in the same way as the original
           QUADPACK code so that the rounding errors are the same, which
           makes testing easier. */

        errsum = errsum + error12 - e_i;
        area   = area + area12 - r_i;

        tolerance = std::fmax(epsabs, epsrel * fabs(area));

        if (resasc1 != error1 && resasc2 != error2)
        {
            double delta = r_i - area12;

            if (fabs(delta) <= 1.0e-5 * fabs(area12) && error12 >= 0.99 * e_i)
            {
                if (!extrapolate)
                {
                    roundoff_type1++;
                }
                else
                {
                    roundoff_type2++;
                }
            }
            if (iteration > 10 && error12 > e_i)
            {
                roundoff_type3++;
            }
        }

        /* Test for roundoff and eventually set error flag */

        if (roundoff_type1 + roundoff_type2 >= 10 || roundoff_type3 >= 20)
        {
            error_type = 2; /* round off error */
        }

        if (roundoff_type2 >= 5)
        {
            error_type2 = 1;
        }

        /* set error flag in the case of bad integrand behaviour at
           a point of the integration range */

        if (subinterval_too_small(a1, a2, b2))
        {
            error_type = 4;
        }

        /* append the newly-created intervals to the list */

        update(workspace, a1, b1, area1, error1, a2, b2, area2, error2);

        if (errsum <= tolerance)
        {
            goto compute_result;
        }

        if (error_type)
        {
            break;
        }

        if (iteration >= limit - 1)
        {
            error_type = 1;
            break;
        }

        if (iteration == 2) /* set up variables on first iteration */
        {
            error_over_large_intervals = errsum;
            ertest                     = tolerance;
            append_table(&table, area);
            continue;
        }

        if (disallow_extrapolation)
        {
            continue;
        }

        error_over_large_intervals += -last_e_i;

        if (current_level < workspace->maximum_level)
        {
            error_over_large_intervals += error12;
        }

        if (!extrapolate)
        {
            /* test whether the interval to be bisected next is the
               smallest interval. */

            if (large_interval(workspace))
                continue;

            extrapolate      = 1;
            workspace->nrmax = 1;
        }

        if (!error_type2 && error_over_large_intervals > ertest)
        {
            if (increase_nrmax(workspace))
                continue;
        }

        /* Perform extrapolation */

        append_table(&table, area);

        qelg(&table, &reseps, &abseps);

        ktmin++;

        if (ktmin > 5 && err_ext < 0.001 * errsum)
        {
            error_type = 5;
        }

        if (abseps < err_ext)
        {
            ktmin   = 0;
            err_ext = abseps;
            res_ext = reseps;
            correc  = error_over_large_intervals;
            ertest  = std::fmax(epsabs, epsrel * fabs(reseps));
            if (err_ext <= ertest)
                break;
        }

        /* Prepare bisection of the smallest interval. */

        if (table.n == 1)
        {
            disallow_extrapolation = 1;
        }

        if (error_type == 5)
        {
            break;
        }

        /* work on interval with largest error */

        reset_nrmax(workspace);
        extrapolate                = 0;
        error_over_large_intervals = errsum;

    } while (iteration < limit);

    *result = res_ext;
    *abserr = err_ext;

    if (err_ext == DBL_MAX)
        goto compute_result;

    if (error_type || error_type2)
    {
        if (error_type2)
        {
            err_ext += correc;
        }

        if (error_type == 0)
            error_type = 3;

        if (res_ext != 0.0 && area != 0.0)
        {
            if (err_ext / fabs(res_ext) > errsum / fabs(area))
                goto compute_result;
        }
        else if (err_ext > errsum)
        {
            goto compute_result;
        }
        else if (area == 0.0)
        {
            goto return_error;
        }
    }

    /*  Test on divergence. */

    {
        double max_area = std::fmax(fabs(res_ext), fabs(area));

        if (!positive_integrand && max_area < 0.01 * resabs0)
            goto return_error;
    }

    {
        double ratio = res_ext / area;

        if (ratio < 0.01 || ratio > 100.0 || errsum > fabs(area))
            error_type = 6;
    }

    goto return_error;

compute_result:

    *result = sum_results(workspace);
    *abserr = errsum;

return_error:

    if (error_type > 2)
        error_type--;
}

int gsl_integration_qags(const gsl_function* f, double a, double b, double epsabs, double epsrel, size_t limit,
    gsl_integration_workspace* workspace, double* result, double* abserr)
{
    int status = qags(f, a, b, epsabs, epsrel, limit, workspace, result, abserr, &gsl_integration_qk21);
    return status;
}