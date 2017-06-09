#include <stdlib.h>

char **grid;
size_t gw = 0, gh = 0, hx = 0, hy = 0, tx = 0, ty = 0, fx = 0, fy = 0;
enum gc {
    left, right, up, down, wall, empty, food
};
char xmod[] = {-1, 1, 0, 0};
char ymod[] = {0, 0, -1, 1};
enum gs {
    moved, ate, lost, won
};
double score_multiplier[] = {0, 10, 0, 100};
const size_t max_game_moves = 1000;

double *get_inputs();
char move_snake(char direction);
void reset_grid();
double calc_max_score();
