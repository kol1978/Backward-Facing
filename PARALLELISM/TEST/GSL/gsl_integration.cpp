#include <gsl/gsl_integration.h>
#include <omp.h>
#include <stdio.h>

double f(double x, void *params) {
    return x * x;
}

int main() {
    const int N = 8;                        // число потоков
    const double a = 0.0, b = 1.0;
    const double h = (b - a) / N;
    double total = 0.0;

    #pragma omp parallel for reduction(+:total) num_threads(N)
    for (int i = 0; i < N; i++) {
        double local_a = a + i * h;
        double local_b = a + (i + 1) * h;
        double result, error;

        // У каждого потока — свой workspace (GSL не потокобезопасен глобально)
        gsl_integration_workspace *w = gsl_integration_workspace_alloc(1000);
        gsl_function F;
        F.function = &f;
        F.params = NULL;

        gsl_integration_qags(&F, local_a, local_b, 0, 1e-7, 1000, w, &result, &error);
        total += result;

        gsl_integration_workspace_free(w);
    }

    printf("Result: %.15f\n", total);
    return 0;
}

