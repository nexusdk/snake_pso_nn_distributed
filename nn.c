#include "nn.h"

void nn(double *inputs, double *weights, double *outputs, double (*activation)(double)) {
    size_t w = 0;
    double *outputs_tmp = 0;
    for (size_t l = 0; l < layers - 1; l++) {
        outputs_tmp = malloc(sizeof(double) * layout[l + 1]);
        for (size_t o = 0; o < layout[l + 1]; o++) {
            outputs_tmp[o] = 0;
            for (size_t i = 0; i < layout[l]; i++) {
                outputs_tmp[o] += inputs[i] * weights[w++];
            }
            outputs_tmp[o] += -1 * weights[w++];
            outputs_tmp[o] = activation(outputs_tmp[o]);
        }
        // NOTE: We are freeing the inputs here...
        free(inputs);
        inputs = outputs_tmp;
    }
    for (size_t o = 0; o < layout[layers - 1]; o++) {
        outputs[o] = inputs[o];
    }
    free(inputs);
}