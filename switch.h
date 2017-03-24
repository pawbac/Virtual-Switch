#ifndef SWITCH_H
#define SWITCH_H

#include "port.h"

#include <stdint.h>

#define DEBUG 1

/* TODO: count number of ports or make a list */
#define MAX_NB_PORTS 3

struct Switch {
    uint8_t nb_ports;
    struct rte_mempool *pktmbuf_pool;
    struct Port *ports[MAX_NB_PORTS];
    struct rte_hash *mac_addr_tbl;
};

int switch_parse_args (void);
int switch_init(struct Switch *s, int argc, char **argv);
int switch_run(struct Switch *s);
void switch_stop(struct Switch *s);
int launch_rx_loop(struct Switch *s);
int launch_tx_loop(struct Switch *s);
int launch_fwd_loop(struct Switch *s);

//extern struct Switch sw;

#endif /* SWITCH_H */