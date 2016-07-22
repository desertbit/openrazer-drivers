#!/bin/bash
# This script tests all available driver functionalities.

# Exit on error.
set -e

DRIVER_PATH="/sys/bus/hid/drivers/hid-razer"
DEVICE_PATH=""

COLOR='\033[0;31m'
NC='\033[0m' # No Color


if [[ ! -d "$DRIVER_PATH" ]]; then
    echo "driver is not loaded."
    exit 1
fi

# Find an attached device.
for d in "$DRIVER_PATH"/*
do
    if [[ $d == *:*:*\.* ]]; then
        DEVICE_PATH="$d"
        break
    fi
done

if [[ "$DEVICE_PATH" == "" ]]; then
    echo "no supported Razer device attached."
    exit 1
fi

echo "Using Razer device $DEVICE_PATH"


echo -n -e "${COLOR}get_firmware_version:${NC} "
cat "$DEVICE_PATH/get_firmware_version"

echo -n -e "${COLOR}get_serial:${NC} "
cat "$DEVICE_PATH/get_serial"

echo -n -e "${COLOR}device_type:${NC} "
cat "$DEVICE_PATH/device_type"

echo -n -e "${COLOR}brightness:${NC} "
cat "$DEVICE_PATH/brightness"

echo -e "${COLOR}set brightness to${NC} 50"
echo -n "50" > "$DEVICE_PATH/brightness"

echo -n -e "${COLOR}brightness:${NC} "
cat "$DEVICE_PATH/brightness"

sleep 2

echo -e "${COLOR}set brightness to${NC} 0"
echo -n "0" > "$DEVICE_PATH/brightness"

echo -n -e "${COLOR}brightness:${NC} "
cat "$DEVICE_PATH/brightness"

sleep 2

echo -e "${COLOR}set brightness to${NC} 255"
echo -n "255" > "$DEVICE_PATH/brightness"

echo -n -e "${COLOR}brightness:${NC} "
cat "$DEVICE_PATH/brightness"

sleep 2
