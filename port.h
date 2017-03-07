#ifndef PORT_H
#define PORT_H

#include <stddef.h>

#include <rte_ethdev.h>
#include <rte_malloc.h>
#include <rte_mempool.h>


#define MAX_PKT_BURST 32
#define BURST_TX_DRAIN_US 100 /* TX drain every ~100us */

static const struct rte_eth_conf port_conf = {
	.rxmode = {
		.split_hdr_size = 0,
		.header_split   = 0, /**< Header Split disabled */
		.hw_ip_checksum = 0, /**< IP checksum offload disabled */
		.hw_vlan_filter = 0, /**< VLAN filtering disabled */
		.jumbo_frame    = 0, /**< Jumbo Frame Support disabled */
		.hw_strip_crc   = 0, /**< CRC stripped by hardware */
	},
	.txmode = {
		.mq_mode = ETH_MQ_TX_NONE,
	},
};

struct Port {
    uint8_t port_id;
    struct rte_eth_dev_tx_buffer *tx_buffer;
    /* MAC address */
    struct ether_addr ether_addr;

    /* Statistics */
    uint64_t total_packets_dropped;
    uint64_t total_packets_rx;
    uint64_t total_packets_tx;
};

struct Port* port_init(uint8_t port_id, struct rte_mempool *mb_pool);

#endif /* PORT_H */