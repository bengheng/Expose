#!/bin/bash

VBOX_NAME=$1
DISK_FILE=$2
DVD_FILE=$3

VBoxManage createvm --name "$VBOX_NAME" --register
VBoxManage modifyvm "$VBOX_NAME" --memory 256 --acpi on --boot1 dvd --nic1 nat
VBoxManage createvdi --filename "$DISK_FILE" --size 65536 --register
#VBoxManage storagectl "$VBOX_NAME" --name "IDE Controller" --add ide
VBoxManage modifyvm "$VBOX_NAME" --hda "$DISK_FILE"
VBoxManage registerimage dvd "$DVD_FILE"
VBoxManage modifyvm "$VBOX_NAME" --dvd "$DVD_FILE"
#VBoxManage storageattach "$VBOX_NAME" --storagectl "IDE Controller" --port 0 --device 1 --type dvddrive --medium "$DVD_FILE"
