#!/bin/sh
set -eu

links="/tmp/ttyRKRight /tmp/ttyRKRightPeer /tmp/ttyRKLeft /tmp/ttyRKLeftPeer /tmp/ttyUSB /tmp/ttyUSBPeer"
pids=""

cleanup() {
    for pid in $pids; do kill "$pid" 2>/dev/null || true; done
    for link in $links; do rm -f "$link"; done
}
trap cleanup EXIT INT TERM

command -v socat >/dev/null 2>&1 || {
    echo "socat is required: sudo apt-get install socat" >&2
    exit 1
}

cleanup
socat PTY,raw,echo=0,link=/tmp/ttyRKRight PTY,raw,echo=0,link=/tmp/ttyRKRightPeer & pids="$pids $!"
socat PTY,raw,echo=0,link=/tmp/ttyRKLeft PTY,raw,echo=0,link=/tmp/ttyRKLeftPeer & pids="$pids $!"
socat PTY,raw,echo=0,link=/tmp/ttyUSB PTY,raw,echo=0,link=/tmp/ttyUSBPeer & pids="$pids $!"

sleep 1
echo "RK right: /tmp/ttyRKRight <-> /tmp/ttyRKRightPeer"
echo "RK left:  /tmp/ttyRKLeft  <-> /tmp/ttyRKLeftPeer"
echo "USB ACM:  /tmp/ttyUSB     <-> /tmp/ttyUSBPeer"
wait
