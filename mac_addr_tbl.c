#include "mac_addr_tbl.h"
#include "switch.h"

#include <rte_errno.h>
#include <rte_hash.h>

int mac_addr_tbl_init (struct Switch *sw) {
    /* Creates a new hash tbl */
    sw->mac_addr_tbl = rte_hash_create(&hash_params);

    printf("Error: %s", rte_errno);
    return 0;
}

int mac_addr_tbl_add_route (struct Switch *sw, struct ether_addr *mac_addr, unsigned src_port) {
    int ret = rte_hash_add_key_data(sw->mac_addr_tbl, (void *) mac_addr, src_port);

    if (!ret)
        printf("Added new device (%02X:%02X:%02X:%02X:%02X:%02X) at port %u\n",
															 mac_addr->addr_bytes[0],
															 mac_addr->addr_bytes[1],
															 mac_addr->addr_bytes[2],
															 mac_addr->addr_bytes[3],
															 mac_addr->addr_bytes[4],
															 mac_addr->addr_bytes[5],
                                                             src_port);

    return ret;
}

int mac_addr_tbl_lookup (struct Switch *sw, struct ether_addr *mac_addr, unsigned *dst_port) {
    int ret = rte_hash_lookup_data(sw->mac_addr_tbl, mac_addr, dst_port);

    /* TODO: use rte_hash_lookup_bulk_data when possible */
    return ret;
}

void mac_addr_tbl_free (struct Switch *sw) {
    rte_hash_free(sw->mac_addr_tbl);
}