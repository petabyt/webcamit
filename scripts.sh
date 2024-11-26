sudo modprobe v4l2loopback devices=1 video_nr=7 exclusive_caps=1 card_label="ExternalWebCam"
rmmod v4l2loopback 2> /dev/null
gphoto2 --stdout --capture-movie | gst-launch-1.0 fdsrc ! image/jpeg,framerate=25/1,format=YUY2,width=1024,height=680 ! decodebin3 name=dec ! queue ! videoconvert ! tee ! v4l2sink device=/dev/video5 sync=false
ffmpeg -f x11grab -r 30 -s 1366x768 -i :0.0+0,0 -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f v4l2 /dev/video6
