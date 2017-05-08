#ifndef MAC_ADDR_TBL_H
#define MAC_ADDR_TBL_H

#include "switch.h"

#include <rte_ether.h>
#include <rte_hash.h>

#define MAC_ADDR_TBL_SIZE 1024

static const struct rte_hash_parameters hash_params = {
    .name = "mac_address_table",
    .entries = MAC_ADDR_TBL_SIZE,
    .key_len = sizeof(struct ether_addr),
    //.socket_id = rte_socket_id(), // FIXME: 'initializer element is not constant'
};

void mac_addr_tbl_init (void);
int mac_addr_tbl_add_route (struct ether_addr *mac_addr, uint8_t src_port);
int mac_addr_tbl_lookup_data (struct ether_addr *mac_addr, uint8_t *dst_port);
int mac_addr_tbl_lookup (struct ether_addr *mac_addr);
void mac_addr_tbl_free (void);

#endif /* MAC_ADDR_TBL_H */