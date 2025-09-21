#!/bin/bash
# File: test_afxdp.sh
# Usage: sudo ./test_loopback.sh ./LoopbackAFXDP

if [ "$EUID" -ne 0 ]; then
  echo "Please run as root."
  exit 1
fi

APP="$1"
if [ -z "$APP" ]; then
  echo "Usage: $0 <path-to-LoopbackAFXDP>"
  exit 1
fi

INGRESS=veth0
EGRESS=veth1
IP1=192.168.100.1
IP2=192.168.100.2

echo "Creating veth pair..."
ip link add $INGRESS type veth peer name $EGRESS
ip addr add $IP1/24 dev $INGRESS
ip addr add $IP2/24 dev $EGRESS
ip link set $INGRESS up
ip link set $EGRESS up

# Disable offloads for AF_XDP compatibility
ethtool -K $INGRESS tx off rx off
ethtool -K $EGRESS tx off rx off

echo "Starting AF_XDP loopback app..."
$APP $INGRESS $EGRESS &
APP_PID=$!

# Give it a moment to start
sleep 2

echo "Testing packet forwarding with ping..."
ping -c 4 $IP2 -I $INGRESS

echo "Testing complete. Killing loopback app..."
kill $APP_PID
wait $APP_PID 2>/dev/null

echo "Cleaning up veth pair..."
ip link delete $INGRESS

echo "Done."
