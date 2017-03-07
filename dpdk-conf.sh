#!/bin/bash

HUGEPGSZ=`cat /proc/meminfo  | grep Hugepagesize | cut -d : -f 2 | tr -d ' '`

Pages=512

cd ~/lib/dpdk/

export RTE_SDK=$(pwd)
modprobe uio
insmod $RTE_SDK/x86_64-native-linuxapp-gcc/kmod/igb_uio.ko
$RTE_SDK/tools/dpdk-devbind.py -b igb_uio enp0s9 enp0s10


echo "echo $Pages > /sys/kernel/mm/hugepages/hugepages-${HUGEPGSZ}/nr_hugepages" > .echo_tmp
sh .echo_tmp
rm -f .echo_tmp

echo "Creating /mnt/huge and mounting as hugetlbfs"
mkdir -p /mnt/huge

grep -s '/mnt/huge' /proc/mounts > /dev/null
if [ $? -ne 0 ] ; then
        mount -t hugetlbfs nodev /mnt/huge
fi 

cd -
