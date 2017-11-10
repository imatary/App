#!/bin/bash

python2.7 "sniffer.py" -p /dev/ttyUSB0 | "wireshark" -ki -
