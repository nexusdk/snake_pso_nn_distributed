#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include "snake.h"


const size_t game_play_count = 100;


int main() {
    srand(time(NULL));
    for (size_t grid_size = 2; grid_size < 10; grid_size++) {
        gw = grid_size;
        gh = grid_size;
        grid = malloc(sizeof(char *) * gw);
        for (size_t x = 0; x < gw; x++) {
            grid[x] = malloc(sizeof(char) * gh);
        }
        for (size_t run = 0; run < 50; run++) {
            double score_average = 0;
            for (size_t game_counter = 0; game_counter < game_play_count; game_counter++) {
                reset_grid();
                size_t move_counter = 0;
                double current_score = 0;
                char current_state = moved;
                size_t snake_length = 1;
                while (current_state != won && current_state != lost && move_counter < max_game_moves) {
                    int mask = 0;
                    size_t x[] = {hx - 1, hx - 1, hx - 1, hx + 1, hx + 1, hx + 1, hx    , hx    };
                    size_t y[] = {hy - 1, hy    , hy + 1, hy - 1, hy    , hy + 1, hy - 1, hy + 1};
                    int adds[] = {1     , 2     , 4     , 8     , 16    , 32    , 64   , 128};
                    for (char i = 0; i < 8; i++) {
                        if (x[i] >= gw || y[i] >= gh || grid[x[i]][y[i]] < empty) {
                            mask += adds[i];
                        }
                    }
                    char option_preference[4] = {0,0,0,0};
                    if (mask & 1  || mask & 128) option_preference[left]  = 1;
                    if (mask & 2  || mask & 4  ) option_preference[up]    = 1;
                    if (mask & 8  || mask & 16 ) option_preference[right] = 1;
                    if (mask & 32 || mask & 64 ) option_preference[down]  = 1;
                    for (char i = 0; i < 4; i++)
                        if (xmod[i] != 0)
                            option_preference[i] += hx + xmod[i] > hx ? fx > hx : fx < hx;
                        else
                            option_preference[i] += hy + ymod[i] > hy ? fy > hy : fy < hy;
                    char directions[4];
                    char choice = 0;
                    char options = 0;
                    for (char i = 0; i < 4; i++)
                        if (hx + xmod[i] < gw && hy + ymod[i] < gh)
                            if (grid[hx + xmod[i]][hy + ymod[i]] > wall)
                                directions[options++] = i;
                    if (options > 0) {
                        choice = rand() % options;
                        char max = options + choice;
                        for (char o = choice; o < max; o++) {
                            if (option_preference[o % options] > option_preference[choice]) {
                                choice = o % options;
                            }
                        }
                    }
                    current_state = move_snake(directions[choice]);
                    current_score += score_multiplier[current_state] * (double) snake_length;
                    if (current_state == ate) {
                        snake_length++;
                    }
                    move_counter++;
                }
                score_average += current_score;
            }
            char save_name[100];
            sprintf(save_name, "output_wall_hugger/results_size-%ld_run-%ld", grid_size, run);
            FILE *data_file = fopen(save_name, "w");
            fprintf(data_file, "%lf\n", score_average / (double) game_play_count);
            fclose(data_file);
        }
        for (size_t x = 0; x < gw; x++) {
            free(grid[x]);
        }
        free(grid);
    }
}
