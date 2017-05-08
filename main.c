#include "switch.h"

int main(int argc, char **argv) {
    struct Switch sw;

    switch_init(&sw, argc, argv);
    switch_run(&sw);
    switch_stop(&sw);

    return 0;
}