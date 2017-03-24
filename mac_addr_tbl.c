#include "mac_addr_tbl.h"
#include "switch.h"

#include <rte_errno.h>
#include <rte_hash.h>

int mac_addr_tbl_init (struct Switch *sw) {
    /* Create a new hash table. */
    sw->mac_addr_tbl = rte_hash_create(&hash_params);

    if (sw->mac_addr_tbl == NULL) rte_exit(EXIT_FAILURE, "Unable to create the hashmap");
    return 0;
}

int mac_addr_tbl_add_route (struct Switch *sw, struct ether_addr *mac_addr, uint8_t port) {
    /* Add a key-value pair to an existing hash table. */
    int ret = rte_hash_add_key_data(sw->mac_addr_tbl, mac_addr, &port);

    if (!ret)
        printf("Added new device (%02X:%02X:%02X:%02X:%02X:%02X) at port %u\n",
                mac_addr->addr_bytes[0],
                mac_addr->addr_bytes[1],
                mac_addr->addr_bytes[2],
                mac_addr->addr_bytes[3],
                mac_addr->addr_bytes[4],
                mac_addr->addr_bytes[5],
                port);

    return ret;
}

int mac_addr_tbl_lookup_data (struct Switch *sw, struct ether_addr *mac_addr, uint8_t *port) {
    /* Find a key-value pair in the hash table. */
    /* returns index where key is stored */
    int ret = rte_hash_lookup_data(sw->mac_addr_tbl, mac_addr, (void **) port);

    /* TODO: use rte_hash_lookup_bulk_data when possible */
    return ret;
}

int mac_addr_tbl_lookup (struct Switch *sw, struct ether_addr *mac_addr) {
    /* Check if key in hash table. */
    int ret = rte_hash_lookup(sw->mac_addr_tbl, mac_addr);

    return ret;
}

void mac_addr_tbl_free (struct Switch *sw) {
    rte_hash_free(sw->mac_addr_tbl);
}