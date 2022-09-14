#!/bin/bash
# Script to stop the VMs
echo "Stopping the VMs..."
VMs="h1 h2 h3 h4 h5 r1 r2 r3"
for vm in $VMs; do
	echo "Powering down $vm..."
	VBoxManage controlvm "$vm" poweroff
	sleep 1
done
