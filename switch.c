#include "switch.h"
#include "port.h"

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

static volatile bool force_quit;

/* Taken from L2FWD DPDK example
 * handles the Unix signals from user
*/
static void signal_handler(int signum) {
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n\n",
				signum);
		force_quit = true;
	}
}

int switch_init(struct Switch *s, int argc, char **argv) {
	int ret = 0;
	int port_id = 0;

    /* Init EAL */
	ret = rte_eal_init(argc, argv);
	if (ret < 0)
		rte_exit(EXIT_FAILURE, "Invalid EAL arguments\n");

    force_quit = false;
	signal(SIGINT, signal_handler);
	signal(SIGTERM, signal_handler);

	/* Create the mbuf pool */
	s->pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SIZE, 0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	if (s->pktmbuf_pool == NULL)
		rte_exit(EXIT_FAILURE, "Cannot init mbuf pool\n");

    /* Count the total number of ports available */
	int nb_ports = rte_eth_dev_count();
	printf("Number of ports: %d\n", nb_ports);
	if (nb_ports == 0)
		rte_exit(EXIT_FAILURE, "No Ethernet ports - bye\n");

	/* Initialise each port */
	for (port_id = 0; port_id < nb_ports; port_id++) {
		struct Port *port;
		port = port_init(port_id, s->pktmbuf_pool);
		s->ports[port_id] = port;
		s->nb_ports = port_id + 1;
	}	

	return ret;
}

int switch_run (struct Switch *sw) {
	/* Launch tasks on isolated cores (requires at least 3 cores; 0 is MASTER) */
	rte_eal_remote_launch(launch_rx_loop, sw, 1);
	rte_eal_remote_launch(launch_tx_loop, sw, 2);

	#if DEBUG
	int lcore_id;
	for (lcore_id = 0 ;lcore_id < 4; lcore_id++) {
		printf("Lcore %d is %s\n", lcore_id, rte_lcore_is_enabled(lcore_id)? "enabled" : "disabled");
	}
	#endif /* DEBUG */

	/* Wait untill all slave lcores in WAIT state */
	rte_eal_mp_wait_lcore();

	return 0;
}

void switch_stop(struct Switch *s) {
	int port_id;

	/* Stop and disable ports */
	for (port_id = 0; port_id < s->nb_ports; port_id++) {
		port_stop(s->ports[port_id]);
	}
	printf("\nBye...\n");
}

int switch_parse_args (void) {
	return 0;
}

int launch_rx_loop(struct Switch *sw) {
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];
	unsigned port_id = 0;
	unsigned recv = 0;
	unsigned enqueued = 0;

	while (!force_quit) {
		for (port_id = 0; port_id <= (sw->nb_ports - 1); port_id++) {
			recv = rte_eth_rx_burst((uint8_t) port_id, 0, pkts_burst, MAX_PKT_BURST);

			if (recv) {
				/* Enque packets to the RX ring */
				enqueued = rte_ring_enqueue_burst(sw->ports[port_id]->rx_ring, (void *) pkts_burst, recv);

				sw->ports[port_id]->total_packets_rx += recv;

				#if DEBUG
				printf("%u packets received\n", recv);
		
				uint8_t pkt;
				struct ether_hdr *eth;

				for (pkt = 0; pkt < recv; pkt++) {
					eth = rte_pktmbuf_mtod(pkts_burst[pkt], struct ether_hdr *);
					printf("\tPacket %d from %02X:%02X:%02X:%02X:%02X:%02X\n", pkt,
															 eth->s_addr.addr_bytes[0],
															 eth->s_addr.addr_bytes[1],
															 eth->s_addr.addr_bytes[2],
															 eth->s_addr.addr_bytes[3],
															 eth->s_addr.addr_bytes[4],
															 eth->s_addr.addr_bytes[5]);
				}
				printf("%u packets eneueued\n", enqueued);
				#endif
			}
			
		}
	}

	#if DEBUG
	for (port_id = 0; port_id < sw->nb_ports; port_id++) {
		printf("Packets received on port %u: %u\n", port_id, (unsigned) sw->ports[port_id]->total_packets_rx);
		rte_ring_dump(stdout, sw->ports[port_id]->rx_ring);
	}
	#endif

	return 0;
}

int launch_tx_loop(struct Switch *sw) {
	unsigned port_id = 0, sent;
	unsigned dequeued;
	struct rte_mbuf *pkts_burst[MAX_PKT_BURST];

	while (!force_quit) {
		for (port_id = 0; port_id <= (sw->nb_ports - 1); port_id++) {
			/* Dequeue packets from port's TX ring */
			dequeued = rte_ring_dequeue_burst(sw->ports[port_id]->rx_ring, (void *) pkts_burst, MAX_PKT_BURST); // FIXME: change to tx_ring
			
			if (dequeued) {
				sent = rte_eth_tx_burst(port_id, 0, pkts_burst, dequeued);

				sw->ports[port_id]->total_packets_tx += sent;

				#if DEBUG
				printf("%u packets sent\n", sent);
		
				uint8_t pkt;
				struct ether_hdr *eth;

				for (pkt = 0; pkt < sent; pkt++) {
					eth = rte_pktmbuf_mtod(pkts_burst[pkt], struct ether_hdr *);
					printf("\tPacket %d from %02X:%02X:%02X:%02X:%02X:%02X\n", pkt,
															 eth->s_addr.addr_bytes[0],
															 eth->s_addr.addr_bytes[1],
															 eth->s_addr.addr_bytes[2],
															 eth->s_addr.addr_bytes[3],
															 eth->s_addr.addr_bytes[4],
															 eth->s_addr.addr_bytes[5]);
				}
				printf("%u packets dequeued\n", dequeued);
				#endif
			}
		}
	}

	#if DEBUG
	for (port_id = 0; port_id < sw->nb_ports; port_id++) {
		printf("Packets transmitted on port %u: %u\n", port_id, (unsigned) sw->ports[port_id]->total_packets_tx);
	}
	#endif

	return 0;
}