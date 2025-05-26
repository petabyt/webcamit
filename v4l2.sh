#!/bin/bash
# This script needs to be run before connecting webcamit to v4l2
# Run as superuser
modprobe -r v4l2loopback
modprobe v4l2loopback devices=1 video_nr=10 exclusive_caps=1 card_label="webcamit"
