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
                    unsigned directions[4];
                    unsigned choice = 0;
                    unsigned options = 0;
                    for (size_t i = 0; i < 4; i++)
                        if (hx + xmod[i] < gw && hy + ymod[i] < gh)
                            if (grid[hx + xmod[i]][hy + ymod[i]] > wall)
                                directions[options++] = i;
                    if (options > 0) choice = rand() % options;
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
            sprintf(save_name, "output_random/results_size-%ld_run-%ld", grid_size, run);
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
