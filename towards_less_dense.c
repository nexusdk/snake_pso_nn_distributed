// NOTE: From the available options, this agent would prefer the direction of the food or less dense areas
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
            double total_eaten = 0;
            double total_won = 0;
            double total_moves = 0;
            for (size_t game_counter = 0; game_counter < game_play_count; game_counter++) {
                reset_grid();
                size_t move_counter = 0;
                double eaten = 0;
                char current_state = moved;
                while (current_state != won && current_state != lost && move_counter < max_game_moves) {
                    char directions[4];
                    char choice = 0;
                    char options = 0;
                    for (char i = 0; i < 4; i++)
                        if (hx + xmod[i] < gw && hy + ymod[i] < gh)
                            if (grid[hx + xmod[i]][hy + ymod[i]] > wall)
                                directions[options++] = i;
                    if (options > 0) {
                        char option_densities[options];
                        for (char o = 0; o < options; o++) {
                            if (xmod[directions[o]] != 0) {
                                option_densities[o] = hx + xmod[directions[o]] > hx ? (fx > hx) * -1 : (fx < hx) * -1;
                                if (hy + 1 < gh)
                                    option_densities[o] += (grid[hx + xmod[directions[o]]][hy + 1] < empty);
                                else
                                    option_densities[o] += 1;
                                if (hy - 1 < gh)
                                    option_densities[o] += (grid[hx + xmod[directions[o]]][hy - 1] < empty);
                                else
                                    option_densities[o] += 1;
                            } else {
                                option_densities[o] = hy + ymod[directions[o]] > hy ? (fy > hy) * -1 : (fy < hy) * -1;
                                if (hx + 1 < gw)
                                    option_densities[o] += (grid[hx + 1][hy + ymod[directions[o]]] < empty);
                                else
                                    option_densities[o] += 1;
                                if (hx - 1 < gw)
                                    (grid[hx - 1][hy + ymod[directions[o]]] < empty);
                                else
                                    option_densities[o] += 1;
                            }
                        }
                        choice = rand() % options;
                        char max = options + choice;
                        for (char o = choice; o < max; o++) {
                            if (option_densities[o % options] < option_densities[choice]) {
                                choice = o % options;
                            }
                        }
                    }
                    current_state = move_snake(directions[choice]);
                    if (current_state == ate || current_state == won) eaten++;
                    move_counter++;
                }
                total_eaten += eaten;
                total_moves += move_counter;
                if (current_state == won) total_won += 1;
            }
            char save_name[100];
            sprintf(save_name, "output_towards_less_dense/results_size-%ld_run-%ld", grid_size, run);
            FILE *data_file = fopen(save_name, "w");
            fprintf(data_file, "%lf %lf %lf\n",
                    total_eaten / (double) game_play_count,
                    total_won / (double) game_play_count,
                    total_moves / (double) game_play_count
            );
            fclose(data_file);
        }
        for (size_t x = 0; x < gw; x++) {
            free(grid[x]);
        }
        free(grid);
    }
}
