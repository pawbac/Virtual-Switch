#ifndef SWITCH_H
#define SWITCH_H

#include "port.h"

#include <stdint.h>

#define MAX_NB_PORTS 12

struct Switch {
    uint8_t nb_ports;
    struct rte_mempool *pktmbuf_pool;
    struct Port *ports[MAX_NB_PORTS];

    struct rte_hash *mac_addr_tbl;
    uint8_t dst_port_list[1024];
};

int switch_parse_args (void);
void switch_init(int argc, char **argv);
void switch_run(void);
void switch_stop(void);
int launch_rx_loop(void);
int launch_tx_loop(void);
int launch_fwd_loop(void);

extern struct Switch sw;

#endif /* SWITCH_H */