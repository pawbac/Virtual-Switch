#ifndef PORT_H
#define PORT_H

#include <stddef.h>

#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_ring.h>

#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

/* Struct used to configure available port */
static const struct rte_eth_conf port_conf = {
    .rxmode = {
        .max_rx_pkt_len = ETHER_MAX_LEN, /* Maximum length of the incoming packet */
    },
};

struct Port {
    uint8_t port_id;

    /* MAC address */
    struct ether_addr ether_addr;

    struct rte_ring *rx_ring;
    struct rte_ring *tx_ring;

    /* Statistics */
    uint64_t total_packets_dropped;
    uint64_t total_packets_rx;
    uint64_t total_packets_tx;
};

struct Port* port_init(uint8_t port_id, struct rte_mempool *mb_pool);
void port_stop(struct Port *port);

#endif /* PORT_H */