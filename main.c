#include "switch.h"

struct Switch sw;

int main(int argc, char **argv) {
    switch_init(argc, argv);
    switch_run();
    switch_stop();

    return 0;
}