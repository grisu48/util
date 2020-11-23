#!/bin/bash
# Shell script to stop all KVM instances, create a BTRFS snapshot (read-only) and restart the stopped KVM instances
# https://gist.github.com/grisu48/535c27cd25c096248ce234ad81abf1b9

## Configuration ##############################################################

IMGDIR="/mnt/RAID/libvirt/images"
SNAPDIR="/mnt/RAID/libvirt/snapshots"
# timeout in seconds
TIMEOUT=300

## Script #####################################################################

running_domains() {
        virsh list | grep running | awk '{print $2}'
}




# Get all virtual machines that are currently running
VMACH="$(running_domains)"

# Shutdown machines
for i in $VMACH; do
        virsh shutdown "$i"
done

# Wait for machines to shut down
END_TIME=$(date -d "$TIMEOUT seconds" +%s)
while [ $(date +%s) -lt $END_TIME ]; do
        test -z "$(running_domains)" && break
        sleep 1         # Prevent busy waiting
done
# Check for still running machines (i.e. timeout)
if [ -n "$(running_domains)" ]; then
        echo "Timeout while shutting down"
        exit 1
fi


# Create snapshot
SNAPSHOT="${SNAPDIR}/`date --iso`"
if [ -d "$SNAPSHOT" ]; then
        echo "Overwriting existing snapshot $SNAPSHOT"
        btrfs subvolume delete "$SNAPSHOT"
fi
btrfs subvolume snapshot -r "$IMGDIR" "$SNAPSHOT"
status=$?

# Bring machines back online
for i in $VMACH; do
        virsh start "$i"
        if [ $? -ne 0 ]; then
                echo "Failed to start $i"
                status="1"
        fi
done

exit $status