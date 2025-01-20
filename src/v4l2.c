// This is a v4l2loopback config script that has to be run as root.
// TODO: This can just be a shell script
#include <stdio.h>
#include <stdlib.h>

int main() {

	// sudo modprobe v4l2loopback devices=1 video_nr=7 exclusive_caps=1 card_label="webcamit"

	return 0;
	
}
