#include <stdio.h>
#include <malloc.h>
#include <math.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include "nn.h"
#include "snake.h"

Display *dis;
int screen;
Window win;
GC gc;
size_t width = 700, height = 700;
size_t cw, ch;
XColor red, brown, blue, yellow, green;
unsigned long black, white;

void create_colormap() {
    Colormap cmap = DefaultColormap(dis, DefaultScreen(dis));
    Status rc;
    rc = XAllocNamedColor(dis, cmap, "red", &red, &red);
    if (rc == 0) {
        fprintf(stderr, "XAllocNamedColor - failed to allocated 'red' color.\n");
        exit(1);
    }
    rc = XAllocNamedColor(dis, cmap, "brown", &brown, &brown);
    if (rc == 0) {
        fprintf(stderr, "XAllocNamedColor - failed to allocated 'brown' color.\n");
        exit(1);
    }
    rc = XAllocNamedColor(dis, cmap, "blue", &blue, &blue);
    if (rc == 0) {
        fprintf(stderr, "XAllocNamedColor - failed to allocated 'blue' color.\n");
        exit(1);
    }
    rc = XAllocNamedColor(dis, cmap, "yellow", &yellow, &yellow);
    if (rc == 0) {
        fprintf(stderr, "XAllocNamedColor - failed to allocated 'yellow' color.\n");
        exit(1);
    }
    rc = XAllocNamedColor(dis, cmap, "green", &green, &green);
    if (rc == 0) {
        fprintf(stderr, "XAllocNamedColor - failed to allocated 'green' color.\n");
        exit(1);
    }
}

void init_x() {
    /* get the colors black and white (see section for details) */

    /* use the information from the environment variable DISPLAY
       to create the X connection:
    */
    dis = XOpenDisplay((char *) 0);
    screen = DefaultScreen(dis);
    black = BlackPixel(dis, screen),    /* get color black */
            white = WhitePixel(dis, screen);  /* get color white */

    /* once the display is initialized, create the window.
       This window will be have be 200 pixels across and 300 down.
       It will have the foreground white and background black
    */
    win = XCreateSimpleWindow(dis, DefaultRootWindow(dis), 0, 0,
                              700, 700, 5, white, black);

    /* here is where some properties of the window can be set.
       The third and fourth items indicate the name which appears
       at the top of the window and the name of the minimized window
       respectively.
    */
    XSetStandardProperties(dis, win, "My Window", "HI!", None, NULL, 0, NULL);

    /* this routine determines which types of input are allowed in
       the input.  see the appropriate section for details...
    */
    XSelectInput(dis, win, ExposureMask | ButtonPressMask | KeyPressMask);

    /* create the Graphics Context */
    gc = XCreateGC(dis, win, 0, 0);


    /* clear the window and bring it on top of the other windows */
    XClearWindow(dis, win);
    XMapRaised(dis, win);
    create_colormap();
};

void close_x() {
/* it is good programming practice to return system resources to the
   system...
*/
    XFreeGC(dis, gc);
    XDestroyWindow(dis, win);
    XCloseDisplay(dis);
    exit(1);
}

void draw(double current_score) {
    XFillRectangle(dis, win, gc, 20, 20, 10, 10);
    for (size_t y = 0; y < gh; y++) {
        for (size_t x = 0; x < gw; x++) {
            switch (grid[x][y]) {
                case left:
                case right:
                case up:
                case down:
                    XSetForeground(dis, gc, red.pixel);
                    break;
                case wall:
                    XSetForeground(dis, gc, blue.pixel);
                    break;
                case empty:
                    XSetForeground(dis, gc, white);
                    break;
                case food:
                    XSetForeground(dis, gc, green.pixel);
                    break;
                default:
                    XSetForeground(dis, gc, black);
                    printf("?");
                    break;
            }
            XFillRectangle(dis, win, gc, (x * cw) + 1, (y * ch) + 1, cw - 2, ch - 2);
        }
    }
    XSetForeground(dis, gc, black);
    XFillArc(dis, win, gc, (hx * cw) + 20, (hy * ch) + 20, cw - 40, ch - 40, 0, 360*64);
    printf("Score: %lf\n", current_score);
}

int main(size_t argc, char *args[]) {
    // Deal with wrong args number
    srand((unsigned) time(NULL));
    init_x();
    double fitness;
    size_t saved_dimensions;
    FILE *data_file = fopen(args[1], "rb");
    fread(&fitness, sizeof(fitness), 1, data_file);
    fread(&saved_dimensions, sizeof(saved_dimensions), 1, data_file);
    double *weights = malloc(sizeof(double) * saved_dimensions);
    fread(weights, sizeof(double), saved_dimensions, data_file);
    fread(&layers, sizeof(layers), 1, data_file);
    layout = malloc(sizeof(size_t) * layers);
    for (size_t i = 0; i < layers; i++) {
        fread(&layout[i], sizeof(layout[i]), 1, data_file);
    }
    fread(&gw, sizeof(gw), 1, data_file);
    fread(&gh, sizeof(gh), 1, data_file);
    fclose(data_file);
    printf("Playing game for with fitness %lf. Max score is %lf.\n", fitness, calc_max_score());
    cw = width / gw;
    ch = height / gh;
    grid = malloc(sizeof(char *) * gw);
    for (size_t x = 0; x < gw; x++) {
        grid[x] = malloc(sizeof(char) * gh);
    }
    reset_grid();
    size_t move_counter = 0;
    char current_state = moved;
    double current_score = 0;
    XEvent e;
    while (current_state != won && current_state != lost && move_counter < max_game_moves) {
        XNextEvent(dis, &e);
        if (e.type == Expose) {
            //XDrawString(dis, win, DefaultGC(dis, screen), 10, 50, msg, strlen(msg));
            draw(current_score);
        }
        if (e.type == KeyPress) {
            double *inputs = get_inputs();
            double *outputs = malloc(sizeof(double) * layout[layers - 1]);
            nn(inputs, weights, outputs, tanh);
            char direction = 0;
            for (char i = 1; i < layout[layers - 1]; i++) {
                if (outputs[i] > outputs[direction]) {
                    direction = i;
                }
            }
            free(outputs);
            current_state = move_snake(direction);
            if (current_state == ate || current_state == won) current_score++;
            move_counter++;
            draw(current_score);
        }

    }
    for (size_t x = 0; x < gw; x++) {
        free(grid[x]);
    }
    free(layout);
    free(grid);
    printf("Final Score: %lf\n\n", current_score);
    close_x();
}