#include <stdio.h>

int main(size_t argc, char *args[]) {
    // Deal with wrong args number
    double fitness;
    double won;
    double moves;
    FILE *data_file = fopen(args[1], "rb");
    fread(&fitness, sizeof(fitness), 1, data_file);
    fread(&won, sizeof(won), 1, data_file);
    fread(&moves, sizeof(moves), 1, data_file);
    fclose(data_file);
    printf("%lf %lf %lf\n", fitness, won, moves);
}