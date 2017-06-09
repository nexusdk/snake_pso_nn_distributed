#include <stdio.h>

int main(size_t argc, char *args[]) {
    // Deal with wrong args number
    double fitness;
    FILE *data_file = fopen(args[1], "rb");
    fread(&fitness, sizeof(fitness), 1, data_file);
    fclose(data_file);
    printf("%lf\n", fitness);
}