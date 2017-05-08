#include "mac_addr_tbl.h"
#include "switch.h"

#include <rte_errno.h>
#include <rte_hash.h>

#define DEBUG 1

void mac_addr_tbl_init (void) {
    /* Create a new hash table. */
    sw.mac_addr_tbl = rte_hash_create(&hash_params);
}

int mac_addr_tbl_add_route (struct ether_addr *mac_addr, uint8_t port) {
    /* Hash selected MAC address and use returned index as a key. */
    int ret = rte_hash_add_key (sw.mac_addr_tbl,(void *) mac_addr);
    /* Store the port in key-value array. */
    sw.dst_port_list[ret] = port;

    #if DEBUG
    if (!(ret < 0))
        printf("Added new device (%02X:%02X:%02X:%02X:%02X:%02X) at port %u\n",
                mac_addr->addr_bytes[0],
                mac_addr->addr_bytes[1],
                mac_addr->addr_bytes[2],
                mac_addr->addr_bytes[3],
                mac_addr->addr_bytes[4],
                mac_addr->addr_bytes[5],
                sw.dst_port_list[ret]);
    #endif /* DEBUG */

    return ret;
}

int mac_addr_tbl_lookup_data (struct ether_addr *mac_addr, uint8_t *port) {
    /* Find the MAC address in hash table. */
    int ret = rte_hash_lookup(sw.mac_addr_tbl, (void *) mac_addr);
    /* Use found key to retrieve the correct destination port. */
    *port = sw.dst_port_list[ret];

    return ret;
}

/* Check if key exists in hash table. */
int mac_addr_tbl_lookup (struct ether_addr *mac_addr) {
    int ret = rte_hash_lookup(sw.mac_addr_tbl, mac_addr);

    return ret;
}

/* Free resources taken by created hash table */
void mac_addr_tbl_free (void) {
    rte_hash_free(sw.mac_addr_tbl);
}