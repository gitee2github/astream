#!/bin/bash
BASE_PATH=$(cd $(dirname $0);pwd)
diskName=$1

echo 0x`nvme get-log /dev/${diskName}n1 -i 0xc0 -n 0xffffffff -l 800|grep "01c0:"|awk '{print $13$12$11$10$9$8$7$6}'` >> ${BASE_PATH}/host_tmp
echo 0x`nvme get-log /dev/${diskName}n1 -i 0xc0 -n 0xffffffff -l 800|grep "01d0:"|awk '{print $9$8$7$6$5$4$3$2}'` >> ${BASE_PATH}/gc_tmp

# IO write counts,unit:4K #
hostWriteHexSectorTemp=`tail -1 ${BASE_PATH}/host_tmp`
# GC write counts,unit 4k #
gcWriteHexSectorTemp=`tail -1 ${BASE_PATH}/gc_tmp`
hostWriteDecSectorTemp=`printf "%llu" ${hostWriteHexSectorTemp}`
gcWriteDecSectorTemp=`printf "%llu" ${gcWriteHexSectorTemp}`
preHostValue=`tail -2 ${BASE_PATH}/host_tmp|head -1`
preGcValue=`tail -2 ${BASE_PATH}/gc_tmp|head -1`
preHostValue=`printf "%llu" ${preHostValue}`
preGcValue=`printf "%llu" ${preGcValue}`

# IO write counts for a period of time
hostWrittenSector=$(echo ${hostWriteDecSectorTemp}-${preHostValue} | bc -l)
# Gc write counts for a period of time
gcWrittenSector=$(echo ${gcWriteDecSectorTemp}-${preGcValue} | bc -l)
nandSector=$(echo ${hostWrittenSector}+${gcWrittenSector} | bc -l)

# unit from kB->MB
hostWrittenMB=$((${hostWrittenSector}/256))
nandWrittenMB=$((${nandSector}/256))

# compute the WA
WA=$(echo "scale=5;${nandSector}/${hostWrittenSector}" | bc)
echo $nandWrittenMB $hostWrittenMB $WA >> ${BASE_PATH}/result_WA.txt