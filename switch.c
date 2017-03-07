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

/* Took from L2FWD DPDK example
 * handles the Unix signals from user
*/
static void signal_handler(int signum)
{
	if (signum == SIGINT || signum == SIGTERM) {
		printf("\n\nSignal %d received, preparing to exit...\n",
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
	s->pktmbuf_pool = rte_pktmbuf_pool_create("mbuf_pool", NB_MBUF, MEMPOOL_CACHE_SIZE, 0,
	RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
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
	}	

	return ret;
}

int switch_run (void) {
	/* Launch tasks on isolated cores (requires at least 3 cores; 0 is MASTER) */
	rte_eal_remote_launch(launch_rx_loop, NULL, 1);
	rte_eal_remote_launch(launch_tx_loop, NULL, 2);

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
		printf("Closing port %d...", port_id);
		rte_eth_dev_stop(port_id);
		rte_eth_dev_close(port_id);
		printf(" Done\n");
	}
	printf("Bye...\n");
}

int switch_parse_args (void) {
	return 0;
}

int launch_rx_loop(void *dummy) {
	while (!force_quit) {
		// TODO: RX LOOP
	}
	printf("QUIT RX\n");
	return 0;
}

int launch_tx_loop(void *dummy) {
	while (!force_quit) {
		// TODO: TX LOOP
	}
	printf("QUIT TX\n");
	return 0;
}