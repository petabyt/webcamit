#!/bin/bash
modprobe -r v4l2loopback
modprobe v4l2loopback devices=1 video_nr=10 exclusive_caps=1 card_label="webcamit"
