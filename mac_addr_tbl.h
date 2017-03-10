#ifndef MAC_ADDR_TBL_H
#define MAC_ADDR_TBL_H

#include "switch.h"

#include <rte_ether.h>
#include <rte_hash.h>

#define MAC_ADDR_TBL_SIZE 1024

static const struct rte_hash_parameters hash_params = {
    .name = "mac_address_table",
    .entries = MAC_ADDR_TBL_SIZE,
    .key_len = sizeof(struct ether_hdr),
    //.socket_id = rte_socket_id(), // FIXME: 'initializer element is not constant'
};

int mac_addr_tbl_init (struct Switch *sw);
int mac_addr_tbl_add_route (struct Switch *sw, struct ether_addr *mac_addr, unsigned src_port);
int mac_addr_tbl_lookup (struct Switch *sw, struct ether_addr *mac_addr, unsigned *dst_port);
void mac_addr_tbl_free (struct Switch *sw);

#endif /* MAC_ADDR_TBL_H */