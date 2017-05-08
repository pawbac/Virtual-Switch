#include "pkt_handling.h"

#include "mac_addr_tbl.h"
#include "switch.h"

#include <arpa/inet.h>

/* Broadcast traffic */
int broadcast(struct Switch *sw, struct rte_mbuf *pkt_buffer) {
    uint8_t port_id;

    for (port_id = 0; port_id < sw->nb_ports; port_id++) {
        rte_ring_enqueue_burst(sw->ports[port_id]->tx_ring, (void *) &pkt_buffer, 1);
        printf("Enqueued to port %d\n", port_id);
    }
}

/* Unicast traffic */
int unicast(struct Switch *sw, struct rte_mbuf *pkt_buffer, struct ether_hdr *eth_hdr) {
    int ret;
    uint8_t dst_port;
    /* Get the destination port based on packet's destination address */
    ret = mac_addr_tbl_lookup_data(sw, &eth_hdr->d_addr, &dst_port);

    switch (ret) {
        case -EINVAL:
            printf("Invalid parameters\n");
            break;
        
        /* MAC address not found, drop packet */
        case -ENOENT:
            printf("Drop\n");
            // TODO: DROP
            break;

        /* MAC address in MAC Address Table */
        default:
            printf("Send to the %d port\n", dst_port);
            /* Enqueue packet to the destination port's TX queue */
            rte_ring_enqueue_burst(sw->ports[1]->tx_ring, (void *) &pkt_buffer, 1); // check if next packets also for this port and send them all at the same time - efficient?
            break;
    }
}

void pkt_description(struct ether_hdr *eth_hdr) {
    char src_buf[ETHER_ADDR_FMT_SIZE];
    char dst_buf[ETHER_ADDR_FMT_SIZE];

    ether_format_addr(src_buf, ETHER_ADDR_FMT_SIZE, &eth_hdr->s_addr);
    ether_format_addr(dst_buf, ETHER_ADDR_FMT_SIZE, &eth_hdr->d_addr);

    printf("Packet type %#06x from %s to %s\n", ntohs(eth_hdr->ether_type), src_buf, dst_buf); // TODO: decide if use ntohs or htons
}