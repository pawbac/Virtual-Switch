#include "mac_addr_tbl.h"
#include "pkt_handling.h"
#include "port.h"
#include "switch.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <sys/types.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <setjmp.h>
#include <stdarg.h>
#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <signal.h>
#include <stdbool.h>

#include <rte_common.h>
#include <rte_log.h>
#include <rte_malloc.h>
#include <rte_memory.h>
#include <rte_memcpy.h>
#include <rte_memzone.h>
#include <rte_eal.h>
#include <rte_per_lcore.h>
#include <rte_launch.h>
#include <rte_atomic.h>
#include <rte_cycles.h>
#include <rte_prefetch.h>
#include <rte_lcore.h>
#include <rte_per_lcore.h>
#include <rte_branch_prediction.h>
#include <rte_interrupts.h>
#include <rte_pci.h>
#include <rte_random.h>
#include <rte_debug.h>
#include <rte_ether.h>
#include <rte_ethdev.h>
#include <rte_mempool.h>
#include <rte_mbuf.h>

#define NB_MBUF   8192
#define MEMPOOL_CACHE_SIZE 256

#define DEBUG 0

/* Print flowing traffic to the string */
#define FLOW 1

static volatile bool force_quit;

/* Taken from the DPDK example
 * handles the Unix signals (e.g. CTRL-C) from user
*/
static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n\nSignal %d received, preparing to exit...\n\n",
                signum);
        force_quit = true;
    }
}

void switch_init(struct Switch *sw, int argc, char **argv) {
    int ret = 0;
    int port_id = 0;

    /* Init EAL */
    ret = rte_eal_init(argc, argv);
    if (ret < 0)
        rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");

    force_quit = false;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create the memory pool */
    sw->pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
    if (sw->pktmbuf_pool == NULL)
        rte_exit(EXIT_FAILURE, "Cannot initialise memory pool\n");

    /* Initialise MAC Address Table */
    mac_addr_tbl_init(sw);
    if (sw->mac_addr_tbl == NULL)
        rte_exit(EXIT_FAILURE, "Unable to create a hashmap\n");

    /* Count the total number of ports available */
    int nb_ports = rte_eth_dev_count();
    printf("Number of ports: %d\n", nb_ports);
    if (nb_ports == 0)
        rte_exit(EXIT_FAILURE, "No ports available, exiting\n");

    /* Initialise each port */
    for (port_id = 0; port_id < nb_ports; port_id++) {
        struct Port *port;
        port = port_init(port_id, sw->pktmbuf_pool);
        sw->ports[port_id] = port;
        sw->nb_ports = port_id + 1;
    }	
}

void switch_run (struct Switch *sw) {
    /* Launch tasks on isolated cores (requires at least 3 cores; 0 is MASTER) */
    rte_eal_remote_launch((int (*)(void*)) launch_rx_loop, sw, 1);
    rte_eal_remote_launch((int (*)(void*)) launch_tx_loop, sw, 2);
    rte_eal_remote_launch((int (*)(void*)) launch_fwd_loop, sw, 3);	

    #if DEBUG
    uint8_t lcore_id;
    for (lcore_id = 0 ;lcore_id < 4; lcore_id++) {
        printf("Lcore %d is %s\n", lcore_id, rte_lcore_is_enabled(lcore_id)? "enabled" : "disabled");
    }
    #endif /* DEBUG */

    /* Wait untill all slave lcores in WAIT state */
    rte_eal_mp_wait_lcore();
}

void switch_stop(struct Switch *sw) {
    uint8_t port_id;

    /* Stop and disable ports */
    for (port_id = 0; port_id < sw->nb_ports; port_id++) {
        printf("Port_id: %d\n", port_id);
        port_stop(sw->ports[port_id]);
    }

    /* Free the hash table */
    mac_addr_tbl_free(sw);
    printf("\nFinished\n");
}

/* Parsing arguments from CLI
 * TODO: e.g. -rx 0x4 -tx 0x12 
 */
int switch_parse_args (void) {
    return 0;
}

int launch_rx_loop(struct Switch *sw) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    uint8_t port_id;
    uint8_t enqueued, recv;

    while (!force_quit) {
        for (port_id = 0; port_id < sw->nb_ports; port_id++) {
            /* Receive packets from NIC */
            recv = rte_eth_rx_burst(port_id, 0, pkts_burst, MAX_PKT_BURST);

            if (recv) {
                /* Enque packets to the port's RX ring */
                enqueued = rte_ring_enqueue_burst(sw->ports[port_id]->rx_ring, (void *) pkts_burst, recv);

                sw->ports[port_id]->total_packets_rx += enqueued;

                #if DEBUG
                printf("%d packets received & %d packets enqueued at port %d\n", enqueued, recv, port_id);
                #endif /* DEBUG */
            }
            
        }
    }

    return 0;
}

int launch_tx_loop(struct Switch *sw) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    uint8_t port_id;
    uint8_t dequeued, sent;

    while (!force_quit) {
        for (port_id = 0; port_id < sw->nb_ports; port_id++) {
            /* Dequeue packets from port's TX ring */
            dequeued = rte_ring_dequeue_burst(sw->ports[port_id]->tx_ring, (void *) pkts_burst, MAX_PKT_BURST);

            if (dequeued) {
                sent = rte_eth_tx_burst(port_id, 0, pkts_burst, dequeued);

                sw->ports[port_id]->total_packets_tx += sent;

                #if DEBUG
                printf("%d packets dequeued & %d packets sent at port %d\n", dequeued, sent, port_id);
                #endif /* DEBUG */
            }
        }
    }

    return 0;
}

int launch_fwd_loop(struct Switch *sw) {
    struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
    uint8_t port_id = 0;
    uint8_t dequeued;

    while (!force_quit) {
        for (port_id = 0; port_id < sw->nb_ports; port_id++) {
            /* Dequeue packets from port's RX ring */
            dequeued = rte_ring_dequeue_burst(sw->ports[port_id]->rx_ring, (void *) pkts_burst, MAX_PKT_BURST);

            if (dequeued) {
                uint8_t pkt;
                
                for (pkt = 0; pkt < dequeued; pkt++) {
                    struct ether_hdr *eth_hdr;
                    int ret;

                    /* Export packet's header */
                    eth_hdr = rte_pktmbuf_mtod(pkts_burst[pkt], struct ether_hdr *);

                    #ifdef FLOW
                    pkt_description(eth_hdr);
                    #endif /* DEBUG */

                    /* Check if packet's source address exists in MAC Address Table */
                    ret = mac_addr_tbl_lookup(sw, &eth_hdr->s_addr);

                    if (unlikely (ret < 0)) {
                        switch (ret) {
                            case -EINVAL:
                                printf("Invalid parameters\n");
                                break;
                            
                            /* MAC address not found, add it */
                            case -ENOENT:
                                mac_addr_tbl_add_route(sw, &eth_hdr->s_addr, port_id); // TODO: Time - for how long
                        }
                    }

                    /* Check if broadcast */
                    if (is_broadcast_ether_addr(&eth_hdr->d_addr))
                        broadcast(sw, pkts_burst[pkt]);
                    else
                        unicast(sw, pkts_burst[pkt], eth_hdr);
                }
            }
        }
    }

    return 0;
}