#include "pkt_handling.h"
#include "mac_addr_tbl.h"
#include "switch.h"

#include <arpa/inet.h>

#define DEBUG 0

/* Broadcast traffic */
void broadcast(struct Switch *sw, struct rte_mbuf *pkt_buffer) {
    uint8_t port_id;

    for (port_id = 0; port_id < sw->nb_ports; port_id++) {
        rte_ring_enqueue_burst(sw->ports[port_id]->tx_ring, (void *) &pkt_buffer, 1);
    }
}

/* Unicast traffic */
void unicast(struct Switch *sw, struct rte_mbuf *pkt_buffer, struct ether_hdr *eth_hdr) {
    uint8_t dst_port;
    int ret;

    /* Get the destination port based on packet's destination address */
    ret = mac_addr_tbl_lookup_data(sw, &eth_hdr->d_addr, &dst_port);

    switch (ret) {
        case -EINVAL:
            printf("Invalid parameters\n");
            break;
        
        /* MAC address not found, drop packet */
        case -ENOENT:
            #if DEBUG
            printf("Packet dropped\n");
            #endif
            
            break;

        /* MAC address in MAC Address Table */
        default:
            /* Enqueue packet to the destination port's TX queue */
            rte_ring_enqueue_burst(sw->ports[dst_port]->tx_ring, (void *) &pkt_buffer, 1); // check if next packets also for this port and send them all at the same time - efficient?
            break;
    }
}

void pkt_description(struct ether_hdr *eth_hdr) {
    char src_buf[ETHER_ADDR_FMT_SIZE];
    char dst_buf[ETHER_ADDR_FMT_SIZE];
    char pkt_type[6];

    ether_format_addr(src_buf, ETHER_ADDR_FMT_SIZE, &eth_hdr->s_addr);
    ether_format_addr(dst_buf, ETHER_ADDR_FMT_SIZE, &eth_hdr->d_addr);

    switch(ntohs(eth_hdr->ether_type)) {
        case ETHER_TYPE_IPv4:
            sprintf(pkt_type, "IPv4");
            break;
        case ETHER_TYPE_IPv6:
            sprintf(pkt_type, "IPv6");
            break;
        case ETHER_TYPE_ARP:
            sprintf(pkt_type, "ARP");
            break;
        case ETHER_TYPE_VLAN:
            sprintf(pkt_type, "VLAN");
            break;
        default:
            break;
    }

    printf("%s > %s, %s \n", src_buf, dst_buf, pkt_type); // TODO: decide if use ntohs or htons
}