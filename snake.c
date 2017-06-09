#include "snake.h"
#include "nn.h"

double calc_max_score() {
    double max_score = 0;
    for (size_t l = 1; l < (gw * gh) - 1; l++) max_score += l * score_multiplier[ate];
    max_score += ((gw * gh) - 1) * score_multiplier[won];
    return max_score;
}

double *get_inputs() {
    double *result = malloc(sizeof(double) * layout[0]);
    size_t target_input = 0;
    result[target_input++] = (hx == fx) ? 0 : (hx < fx ? 1 : -1);
    result[target_input++] = (hy == fy) ? 0 : (hy < fy ? 1 : -1);
//    result[target_input] = 0;
//    for (size_t x = 0; x < gw; x++) {
//        for (size_t y = 0; y < gh; y++) {
//            result[target_input] += grid[x][y] < wall;
//        }
//    }
//    result[target_input++] /= gw * gh;
//    result[target_input++] = grid[hx][hy] == right - grid[hx][hy] == left;
//    result[target_input++] = grid[hx][hy] == up - grid[hx][hy] == down;


//    result[target_input++] = grid[hx][hy] == left;
//    result[target_input++] = grid[hx][hy] == right;
//
//    result[target_input++] = grid[hx][hy] == up;
//    result[target_input++] = grid[hx][hy] == down;

    // TODO: Make this less expensive!!
//    for (size_t x = hx - 1; x <= hx + 1; x++) {
//        for (size_t y = hy - 1; y <= hy + 1; y++) {
//            if (x != hx || y != hy) {
//                if (x >= gw || y >= gh) {
//                    result[target_input++] = -1;
//                } else {
//                    result[target_input++] = (
//                            grid[x][y] == empty ? 0.5 : (grid[x][y] < empty ? (grid[x][y] == wall ? -1 : -0.5) : 1)
//                    );
//                }
//            }
//        }
//    }
    size_t x[] = {hx - 1, hx + 1, hx, hx, hx - 2, hx - 2, hx + 2, hx + 2};
    size_t y[] = {hy, hy, hy - 1, hy + 1, hy - 2, hy + 2, hy - 2, hy + 2};
    for (char i = 0; i < 8; i++) {
        if (x[i] >= gw || y[i] >= gh) {
            result[target_input++] = -1;
        } else {
            result[target_input++] = (
                    grid[x[i]][y[i]] == empty ? 0.5 : (grid[x[i]][y[i]] < empty ? (grid[x[i]][y[i]] == wall ? -1 : -0.5)
                                                                                : 1)
            );
        }
    }
    return result;
}

char add_food() {
    // Counting first with the hope that it will be less expensive.
    size_t target = 0;
    for (size_t x = 0; x < gw; x++) {
        for (size_t y = 0; y < gh; y++) {
            target += (grid[x][y] == empty);
        }
    }
    // This is to avoid allocation of empty cell positions
    if (target > 0) {
        target = rand() % target;
        for (size_t x = 0; x < gw; x++) {
            for (size_t y = 0; y < gh; y++) {
                if (grid[x][y] == empty) {
                    if (target == 0) {
                        grid[x][y] = food;
                        fx = x;
                        fy = y;
                        return ate;
                    } else {
                        target--;
                    }
                }
            }
        }
    }
    return won;
}

void reset_grid() {
    for (size_t x = 0; x < gw; x++) {
        for (size_t y = 0; y < gh; y++) {
            grid[x][y] = empty;
        }
    }
    grid[gw / 2][gh / 2] = left;
    hx = tx = gw / 2;
    hy = ty = gh / 2;
    add_food();
}

char move_snake(char direction) {
    grid[hx][hy] = direction;
    hx += xmod[direction];
    hy += ymod[direction];
    if (hx >= gw || hy >= gh) return lost;
    switch (grid[hx][hy]) {
        case empty: {
            grid[hx][hy] = direction;
            char old_tail = grid[tx][ty];
            grid[tx][ty] = empty;
            tx += xmod[old_tail];
            ty += ymod[old_tail];
            return moved;
        }
        case food: {
            grid[hx][hy] = direction;
            return add_food();
        }
            // This does not allow for turn around with snake length of 2
        default: {
            return lost;
        }
    }
}