#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include "snake.h"
#include "nn.h"


struct sockaddr_in *workers = 0;
size_t worker_count = 0;
int socket_fd;
const size_t game_play_count = 100;

void failure(const char *message) {
    fprintf(stderr, "Error : %s\n", message);
    close(socket_fd);
    exit(EXIT_FAILURE);
}

void read_from_socket(int fd, void *target, size_t expected_size, char *fail_message) {
    ssize_t n = 0;
    ssize_t offset = 0;
    while (expected_size > offset) {
        n = read(fd, target + offset, expected_size - offset);
        if (n == -1) {
            failure(fail_message);
        }
        offset += n;
    }
}

void write_to_socket(int fd, void *source, size_t expected_size, char *fail_message) {
    ssize_t n = 0;
    ssize_t offset = 0;
    while (expected_size > offset) {
        n = write(fd, source + offset, expected_size - offset);
        if (n == -1) {
            failure(fail_message);
        }
        offset += n;
    }
}

double nn_fitness(double *weights, size_t w, size_t h) {
    if (gw != w || gh != h) {
        if (grid) {
            for (size_t x = 0; x < gw; x++) {
                free(grid[x]);
            }
            free(grid);
        }
        gw = w;
        gh = h;
        grid = malloc(sizeof(char *) * gw);
        for (size_t x = 0; x < gw; x++) {
            grid[x] = malloc(sizeof(char) * gh);
        }
    }
    double score_average = 0;
    for (size_t game_counter = 0; game_counter < game_play_count; game_counter++) {
        reset_grid();
        size_t move_counter = 0;
        char current_state = moved;
        double current_score = 0;
        while (current_state != won && current_state != lost && move_counter < max_game_moves) {
            double *inputs = get_inputs();
            double *outputs = malloc(sizeof(double) * layout[layers - 1]);
            nn(inputs, weights, outputs, tanh);
            char direction = 0;
            for (char i = 1; i < 4; i++) {
                if (outputs[i] > outputs[direction]) {
                    direction = i;
                }
            }
            free(outputs);
            current_state = move_snake(direction);
            if (current_state == ate || current_state == won) current_score++;
            move_counter++;
        }
        score_average += current_score;
    }
    return score_average / (double) game_play_count;
}


size_t send_work(int remote_fd, size_t send_index, double **positions, size_t dimensions) {
    char message_type = 'w';
    write_to_socket(remote_fd, &message_type, sizeof(message_type), "Did not write w successfully");
    write_to_socket(remote_fd, &send_index, sizeof(send_index), "Did not write work index successfully");
    write_to_socket(remote_fd, positions[send_index], sizeof(double) * dimensions, "Did not write pos successfully");
    write_to_socket(remote_fd, &gw, sizeof(gw), "Did not write gw successfully");
    write_to_socket(remote_fd, &gh, sizeof(gh), "Did not write gh successfully");
    return send_index + 1;
}

void calculate_fitness(size_t particles, double *fitness, size_t dimensions, double **positions) {
    size_t result_count = 0;
    size_t send_index = 0;
    // WARNING: Here we assume we have less workers than particles.
    for (size_t i = 0; i < worker_count; i++) {
        int remote_socket_fd;
        if ((remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            failure("Could not create socket");
        }
        if (connect(remote_socket_fd, (struct sockaddr *) &workers[i], sizeof(workers[i])) < 0) {
            // TODO: Perhaps retry or remove worker?
            failure("Connect failed at bulk work send");
        }
        send_index = send_work(remote_socket_fd, send_index, positions, dimensions);
        close(remote_socket_fd);
    }
    while (result_count < particles) {
        struct sockaddr_in incoming;
        memset(&incoming, 0, sizeof(incoming));
        socklen_t socket_size = sizeof(incoming);
        int new_connection = accept(socket_fd, (struct sockaddr *) &incoming, &socket_size);
        char message_type;
        read_from_socket(new_connection, &message_type, sizeof(message_type), "Read the wrong message size");
        if (message_type == 'r') {
            size_t result_index;
            read_from_socket(new_connection, &result_index, sizeof(result_index),
                             "Read the wrong message size on result index");
            read_from_socket(new_connection, &fitness[result_index], sizeof(fitness[result_index]),
                             "Read the wrong message size on fitness value");
            result_count++;
        } else if (message_type == 'n') {
            struct sockaddr_in *tmp = malloc(sizeof(struct sockaddr_in) * (worker_count + 1));
            for (size_t i = 0; i < worker_count; i++) {
                tmp[i] = workers[i];
            }
            tmp[worker_count] = incoming;
            read_from_socket(new_connection, &tmp[worker_count].sin_port, sizeof(tmp[worker_count].sin_port),
                             "Read the wrong message size");
            if (workers) {
                free(workers);
            }
            workers = tmp;
            worker_count++;
        }
        if (send_index < particles) { // This should be in a separate thread if we want to deal with a slow network.
            send_index = send_work(new_connection, send_index, positions, dimensions);
        } else {
            message_type = 's';
            write_to_socket(new_connection, &message_type, sizeof(message_type), "Did not write s successfully");
        }
        close(new_connection);
    }
}

void pso(
        size_t particles, size_t iterations, size_t dimensions, double w, double c1, double c2,
        double *lower_limits, double *upper_limits, size_t run, size_t size, double max_score
) {
    double **positions = malloc(sizeof(double *) * particles);
    double **velocities = malloc(sizeof(double *) * particles);
    double *fitness = malloc(sizeof(double) * particles);
    double *cognitive_fitness = malloc(sizeof(double) * particles);
    double **cognitive = malloc(sizeof(double *) * particles);
    double *social = malloc(sizeof(double) * dimensions);
    double social_fitness = -1;
    for (size_t i = 0; i < particles; i++) {
        positions[i] = malloc(sizeof(double) * dimensions);
        cognitive[i] = malloc(sizeof(double) * dimensions);
        velocities[i] = malloc(sizeof(double) * dimensions);
        for (size_t d = 0; d < dimensions; d++) {
            positions[i][d] =
                    lower_limits[d] + ((upper_limits[d] - lower_limits[d]) * ((double) rand() / (double) RAND_MAX));
            cognitive[i][d] = positions[i][d];
            velocities[i][d] = 0;
        }
    }
    calculate_fitness(particles, fitness, dimensions, positions);
    for (size_t i = 0; i < particles; i++) {
        cognitive_fitness[i] = fitness[i];
        if (social_fitness < fitness[i]) {
            social_fitness = fitness[i];
            for (size_t d = 0; d < dimensions; d++) {
                social[d] = positions[i][d];
            }
        }
    }
//    size_t stag = 0;
    for (size_t it = 0; it < iterations; it++) {
        printf("Now at iteration %ld with score %lf\n", it, social_fitness);
        // Velocity and position update
        for (size_t i = 0; i < particles; i++) {
            for (size_t d = 0; d < dimensions; d++) {
                velocities[i][d] = (w * velocities[i][d]) +
                                   (c1 * ((double) rand() / (double) RAND_MAX) * (cognitive[i][d] - positions[i][d])) +
                                   (c2 * ((double) rand() / (double) RAND_MAX) * (social[d] - positions[i][d]));
                positions[i][d] += velocities[i][d];
//                if (positions[i][d] > upper_limits[d]) positions[i][d] = upper_limits[d];
//                else if (positions[i][d] < lower_limits[d]) positions[i][d] = lower_limits[d];
            }
        }
//        if (stag++ == 100) {
//            printf("Scrambling the brains..\n");
//            for (size_t i = 0; i < particles; i++) {
//                for (size_t d = 0; d < dimensions; d++) {
//                    positions[i][d] =
//                            lower_limits[d] + ((upper_limits[d] - lower_limits[d]) * ((double) rand() / (double) RAND_MAX));
//                    velocities[i][d] = 0;
//                }
//            }
//            stag = 0;
//        }
        calculate_fitness(particles, fitness, dimensions, positions);
        // Cognitive and social update
        for (size_t i = 0; i < particles; i++) {
            if (fitness[i] > cognitive_fitness[i]) {
                for (size_t d = 0; d < dimensions; d++) {
                    cognitive[i][d] = positions[i][d];
                }
                cognitive_fitness[i] = fitness[i];
                if (fitness[i] > social_fitness) {
                    for (size_t d = 0; d < dimensions; d++) {
                        social[d] = positions[i][d];
                    }
                    social_fitness = fitness[i];
//                    stag = 0;
                }
            }
        }
        // extra stopping power
        if (max_score == social_fitness) {
            it = iterations;
        }
    }
    printf("Social best calculated as: %lf at position:\n", social_fitness);
    for (size_t d = 0; d < dimensions; d++) {
        printf("%lf,", social[d]);
    }
    printf("\n");
    char save_name[100];
    sprintf(save_name, "results_size-%ld_run-%ld.bin", size, run);
    FILE *data_file = fopen(save_name, "wb");
    fwrite(&social_fitness, sizeof(social_fitness), 1, data_file);
    fwrite(&dimensions, sizeof(dimensions), 1, data_file);
    fwrite(social, sizeof(double), dimensions, data_file);
    fwrite(&layers, sizeof(layers), 1, data_file);
    for (size_t i = 0; i < layers; i++) {
        fwrite(&layout[i], sizeof(layout[i]), 1, data_file);
    }
    fwrite(&gw, sizeof(gw), 1, data_file);
    fwrite(&gh, sizeof(gh), 1, data_file);
    fclose(data_file);
    printf("Data saved to %s\n", save_name);

    free(fitness);
    free(cognitive_fitness);
    free(social);
    for (size_t i = 0; i < particles; i++) {
        free(positions[i]);
        free(cognitive[i]);
        free(velocities[i]);
    }
    free(velocities);
    free(positions);
    free(cognitive);
}

char read_work(
        int remote_socket_fd, size_t *my_index, double *particle_position, size_t dimensions, size_t *w, size_t *h
) {
    char message_type;
    read_from_socket(remote_socket_fd, &message_type, sizeof(message_type), "Read failed for message on read work");
    if (message_type == 'w') {
        read_from_socket(remote_socket_fd, &*my_index, sizeof(*my_index), "Read the wrong message size for index rw");
        read_from_socket(remote_socket_fd, particle_position, sizeof(double) * dimensions,
                         "Read the wrong message size for position rw");
        read_from_socket(remote_socket_fd, &*w, sizeof(*w), "Read the wrong message size for gw rw");
        read_from_socket(remote_socket_fd, &*h, sizeof(*h), "Read the wrong message size for gh rw");
    }
    return message_type;
}

int main(int argc, char *argv[]) {
    srand((unsigned) time(NULL));
    layers = 3;
    layout = malloc(sizeof(size_t) * layers);
    layout[0] = 10;
    layout[1] = 10;
    layout[2] = 4;
    size_t dimensions = 0;
    for (size_t l = 0; l < layers - 1; l++) {
        dimensions += (layout[l] + 1) * layout[l + 1];
    }
    double *lower_limits = malloc(sizeof(double) * dimensions);
    double *upper_limits = malloc(sizeof(double) * dimensions);

    size_t w = 0;
    for (size_t l = 0; l < layers - 1; l++) {
        for (size_t o = 0; o < layout[l + 1]; o++) {
            for (size_t i = 0; i < layout[l] + 1; i++) {
                lower_limits[w] = -1.0 / sqrt(layout[l] + 1);
                upper_limits[w++] = 1.0 / sqrt(layout[l] + 1);
            }
        }
    }
    /*
    for (size_t d = 0; d < dimensions; d++) {
        upper_limits[d] = 2;
        lower_limits[d] = -2;
    }
     */
    int opt;
    char type = 0;
    struct sockaddr_in listen_address;
    struct sockaddr_in remote_address;
    memset(&listen_address, 0, sizeof(listen_address));
    memset(&remote_address, 0, sizeof(remote_address));
    listen_address.sin_family = AF_INET;
    remote_address.sin_family = AF_INET;
    listen_address.sin_addr.s_addr = htonl(INADDR_ANY);
    if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        failure("Could not create socket");
    }
    while ((opt = getopt(argc, argv, "sl:p:c:")) != -1) {
        switch (opt) {
            case 's': {
                type += 1;
                break;
            }
            case 'p': {
                remote_address.sin_port = htons((uint16_t) atoi(optarg));
                break;
            }
            case 'l': {
                listen_address.sin_port = htons((uint16_t) atoi(optarg));
                break;
            }
            case 'c': {
                type += 2;
                if (inet_pton(AF_INET, optarg, &remote_address.sin_addr) <= 0) {
                    failure("inet_pton error");
                }
                break;
            }
            default: {
                fprintf(stderr, "Usage: %s [-s -l <port>]/[-c <address> -p <port> -l <port>]\n", argv[0]);
                exit(EXIT_FAILURE);
            }
        }
    }
    if (listen_address.sin_port == 0) {
        failure("-l argument is required and must be non-zero");
    }
    if (type == 1 || type == 2) {
        int yes = 1;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) == -1) {
            failure("Could not set socket options");
        }
        if (bind(socket_fd, (struct sockaddr *) &listen_address, sizeof(listen_address)) < 0) {
            failure("Could not bind");
        }
        listen(socket_fd, 100000);
        if (type == 1) {
            // Remote address is not used here.
            for (size_t size = 2; size < 10; size++) {
                gw = size;
                gh = size;
                double max_score = calc_max_score();
                printf("Started server. Max score is: %lf\n", max_score);
                for (size_t run = 0; run < 50; run++) {
                    pso(100, 1000, dimensions, 0.72, 1.42, 1.42, lower_limits, upper_limits, run, size, max_score);
                }
            }
            for (size_t i = 0; i < worker_count; i++) {
                int remote_socket_fd;
                if ((remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    failure("Could not create socket");
                }
                if (connect(remote_socket_fd, (struct sockaddr *) &workers[i], sizeof(workers[i])) < 0) {
                    // TODO: Perhaps retry or remove worker?
                    failure("Connect failed at bulk work send");
                }
                char message_type = 'd';
                write_to_socket(remote_socket_fd, &message_type, sizeof(message_type), "Did not write d successfully");
                close(remote_socket_fd);
            }
        } else {
            if (remote_address.sin_port == 0) {
                failure("-p argument is required with -c and must be non-zero");
            }
            double *particle_position = malloc(sizeof(double) * dimensions);
            size_t my_index;
            double result;
            int remote_socket_fd;
            if ((remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                failure("Could not create socket");
            }
            if (connect(remote_socket_fd, (struct sockaddr *) &remote_address, sizeof(remote_address)) < 0) {
                // TODO: Perhaps retry?
                failure("Connect failed");
            }
            char message_type = 'n';
            write_to_socket(remote_socket_fd, &message_type, sizeof(message_type), "Did not write n successfully");
            write_to_socket(remote_socket_fd, &listen_address.sin_port, sizeof(listen_address.sin_port),
                            "Did not write port successfully");
            char running = 1;
            size_t h;
            while (running) {
                while ((message_type = read_work(remote_socket_fd, &my_index, particle_position, dimensions, &h, &w)) ==
                       'w') {
                    close(remote_socket_fd);
                    result = nn_fitness(particle_position, w, h);
                    if ((remote_socket_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                        failure("Could not create socket");
                    }
                    message_type = 'r';
                    if (connect(remote_socket_fd, (struct sockaddr *) &remote_address, sizeof(remote_address)) < 0) {
                        // TODO: Perhaps retry?
                        failure("Connect failed");
                    }
                    write_to_socket(remote_socket_fd, &message_type, sizeof(message_type),
                                    "Did not write successfully on message");
                    write_to_socket(remote_socket_fd, &my_index, sizeof(my_index),
                                    "Did not write successfully on result index");
                    write_to_socket(remote_socket_fd, &result, sizeof(result), "Did not write successfully on result");
                }
                if (message_type == 's') {
                    // TODO: Perhaps respond to the one who sent you work..?
                    close(remote_socket_fd);
                    remote_socket_fd = accept(socket_fd, (struct sockaddr *) NULL, NULL);
                } else {
                    running = 0;
                }
            }
        }
        close(socket_fd);
    } else {
        failure("Must specify only one of -s or -c");
    }
    free(layout);
    free(lower_limits);
    free(upper_limits);
    return 0;
}