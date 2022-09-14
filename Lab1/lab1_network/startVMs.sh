#!/bin/bash
#Script to start the VMs

VMs="r1 r2 r3 h1 h2 h3 h4 h5"
echo "Starting the VMs..."
for i in $VMs; do
	VBoxManage startvm "$i" --type headless
	sleep 2
done	
