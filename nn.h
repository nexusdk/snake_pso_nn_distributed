#include <malloc.h>

size_t *layout = 0;
size_t layers = 0;

void nn(double *inputs, double *weights, double *outputs, double (*activation)(double));