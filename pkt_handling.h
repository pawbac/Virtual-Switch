#ifndef PKT_HANDLING_H
#define PKT_HANDLING_H

#include "switch.h"

#include <rte_ether.h>

void broadcast(struct Switch *sw, struct rte_mbuf *pkts_burst);
void unicast(struct Switch *sw, struct rte_mbuf *pkts_burst, struct ether_hdr *eth_hdr);

void pkt_description(struct ether_hdr *eth_hdr);

#endif /* PKT_HANDLING_H */