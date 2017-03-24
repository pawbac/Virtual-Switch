#include "port.h"

#include <rte_mempool.h>
#include <rte_malloc.h>
#include <rte_ring.h>

#define DEBUG 0

/* Maximum number of TX/RX queues for virtualised e1000 is 1 */
#define NB_RX_QUEUES_PER_DEVICE 1
#define NB_TX_QUEUES_PER_DEVICE 1

/*
 * Configurable number of RX/TX ring descriptors
 */
#define RTE_TEST_RX_DESC_DEFAULT 128
#define RTE_TEST_TX_DESC_DEFAULT 512
uint16_t nb_rxd = RTE_TEST_RX_DESC_DEFAULT;
uint16_t nb_txd = RTE_TEST_TX_DESC_DEFAULT;

/* The size of the rings */
#define RX_RING_SIZE 1024
#define TX_RING_SIZE 1024

struct Port* port_init(uint8_t port_id, struct rte_mempool *mb_pool) {
	int ret = 0;
    char ring_name[10];

    struct Port *port = malloc(sizeof(struct Port));

    printf("Initializing port %u...", port_id);

    /* Assign the port number */
    port->port_id = port_id;

    /* Get a MAC address of the device */
    rte_eth_macaddr_get(port_id, &port->ether_addr);

    /* Configure an Ethernet device */
    ret = rte_eth_dev_configure(port_id, NB_RX_QUEUES_PER_DEVICE, NB_TX_QUEUES_PER_DEVICE, &port_conf);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Cannot configure device: err=%d, port=%u\n", ret, port_id);

    /* Initialise RX queues */
    ret = rte_eth_rx_queue_setup(port_id, 0, nb_rxd, rte_eth_dev_socket_id(port_id), NULL, mb_pool);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_rx_queue_setup:err=%d, port=%u\n", ret, (unsigned) port_id);

    /* Initialise TX queues */
    ret = rte_eth_tx_queue_setup(port_id, 0, nb_txd, rte_eth_dev_socket_id(port_id), NULL);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_tx_queue_setup:err=%d, port=%u\n", ret, (unsigned) port_id);

    /* Initialise TX buffers */
    port->tx_buffer = rte_zmalloc_socket("tx_buffer", RTE_ETH_TX_BUFFER_SIZE(MAX_PKT_BURST), 0, rte_eth_dev_socket_id(port_id));
    if (port->tx_buffer == NULL)
        rte_exit(EXIT_FAILURE, "Cannot allocate buffer for tx on port %u\n", (unsigned) port_id);

    rte_eth_tx_buffer_init(port->tx_buffer, MAX_PKT_BURST);

    ret = rte_eth_tx_buffer_set_err_callback(port->tx_buffer, rte_eth_tx_buffer_count_callback, &port->total_packets_dropped);
    if (ret < 0)
            rte_exit(EXIT_FAILURE, "Cannot set error callback for tx buffer on port %u\n", (unsigned) port_id);

    /* Initialise RX ring */
    snprintf(ring_name, 10, "rx_ring_%u", port_id);
    port->rx_ring = rte_ring_create(ring_name, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), RING_F_SP_ENQ || RING_F_SC_DEQ);
    if (port->rx_ring == NULL)
            rte_exit(EXIT_FAILURE, "Cannot initialise RX ring on port %u\n", (unsigned) port_id);

    #if DEBUG
    printf("\nStatus of RX ring (%s):\n", ring_name);    
    rte_ring_dump(stdout, port->rx_ring);
    #endif

    /* Initialise TX ring */
    snprintf(ring_name, 10, "tx_ring_%u", port_id);
    port->tx_ring = rte_ring_create(ring_name, RX_RING_SIZE, rte_eth_dev_socket_id(port_id), RING_F_SP_ENQ || RING_F_SC_DEQ);
    if (port->rx_ring == NULL)
            rte_exit(EXIT_FAILURE, "Cannot initialise TX ring on port %u\n", (unsigned) port_id);

    #if DEBUG
    printf("\nStatus of TX ring (%s):\n", ring_name);    
    rte_ring_dump(stdout, port->tx_ring);
    #endif

    /* Start device */
    ret = rte_eth_dev_start(port_id);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "rte_eth_dev_start:err=%d, port=%u\n", ret, (unsigned) port_id);

    printf("done!\n");

	/* Enable promiscuous mode on a port */
    rte_eth_promiscuous_enable(port_id);

    /* TODO: Delete as probably necessary (or just to see the MAC address in case of sending packet to the WEB) */
    printf("Port %u, MAC address: %02X:%02X:%02X:%02X:%02X:%02X\n\n",
            (unsigned) port_id,
            port->ether_addr.addr_bytes[0],
            port->ether_addr.addr_bytes[1],
            port->ether_addr.addr_bytes[2],
            port->ether_addr.addr_bytes[3],
            port->ether_addr.addr_bytes[4],
            port->ether_addr.addr_bytes[5]);

    return port;
}

void port_stop(struct Port *port) {
    printf("Closing port %d...", port->port_id);

    /* Free RX/TX rings */
    rte_ring_free(port->rx_ring);
    rte_ring_free(port->tx_ring);

    #if DEBUG
    /* To be sure that rings has been freed */
    rte_ring_dump(stdout, port->rx_ring);
    rte_ring_dump(stdout, port->tx_ring);
    #endif

    /* Close a port */
    rte_eth_dev_stop(port->port_id);
    rte_eth_dev_close(port->port_id);
    printf("Done\n");
}