// Test EOS Liveview
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <camlib.h>
#include "wcit.h"

#if 0

init ffmpeg pipe
init usb watch:
  new thread:
    wait for usb hotplug...
	trigger usb_connect thread

usb_connect thread:
  check usb device
  while true:
    get free video device
	start video pipe
	start camera service
	...
  if none, exit

join usb watch thread

#endif

// webcamit.out: ../../libusb/os/threads_posix.h:46: usbi_mutex_lock: Assertion `pthread_mutex_lock(mutex) == 0' failed.
// Because fork() copies this data and isn't synchronized across threads?
static struct PtpRuntime *global_r = NULL;
static FILE *current_pipe = NULL;

//void ptp_verbose_log(char *fmt, ...) {
//	(void)fmt;
//}

FILE *create_ffmpeg_window_pipe(void) {
	char buffer[1024];
	sprintf(buffer,
		"%s -f mjpeg -i - -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f sdl2 \"%s\"",
		"ffmpeg",
		"Test Window"
	);

    sigset_t sigset, oldset, nset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGINT);
    pthread_sigmask(SIG_BLOCK, &sigset, &oldset);
	pthread_sigmask(SIG_SETMASK, &oldset, &nset);
	FILE *p = popen(buffer, "w");
	pthread_sigmask(SIG_SETMASK, &nset, NULL);
	if (p == NULL) {
		perror("Unable to open ffmpeg pipe");
		fclose(p);
		return NULL;
	} else {
		printf("Opened ffmpeg pipe\n");
	}

	return p;
}

FILE *create_ffmpeg_video_pipe(void) {
	char buffer[1024];
	sprintf(buffer,
		"%s -i - -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f v4l2 %s",
		"ffmpeg",
		"/dev/video10"
	);
	FILE *p = popen(buffer, "w");
	if (p == NULL) {
		perror("Unable to open ffmpeg pipe");
		fclose(p);
		return NULL;
	} else {
		printf("Opened ffmpeg pipe\n");
	}

	return p;
}

void on_quit(int status) {
	if (global_r) {
		printf("Closing down\n");
		struct PtpRuntime *r = global_r;
		ptp_mutex_lock(r);
		ptp_close_session(r);
		ptp_device_close(r);
		printf("quit kill switch %d\n", r->operation_kill_switch);
		fflush(stdout);
		ptp_mutex_unlock(r);

		// The liveview thread will free runtime
	}

	if (current_pipe) {
		printf("Closing output pipe\n");
		pclose(current_pipe);
	}

	printf("Goodbye\n");
	exit(0);
}

int register_camera(void) {
	int first_frame = 0;
	struct PtpRuntime *r = ptp_new(PTP_USB);
	global_r = r;

	if (global_r != NULL) {
		wcit_dbg("Only one connection supported right now.\n");
	}

	if (ptp_device_init(r)) {
		wcit_dbg("No device found\n");
		ptp_close(r);
		return 0;
	}

	printf("Creating ffmpeg pipe...\n");
	FILE *video_pipe = create_ffmpeg_video_pipe();
	if (video_pipe == NULL) return -1;
	current_pipe = video_pipe;

	int rc = ptp_open_session(r);
	if (rc) goto error;

	struct PtpDeviceInfo di;
	rc = ptp_get_device_info(r, &di);
	if (rc) goto error;
	printf("Connected to %s %s\n", di.manufacturer, di.model);

	if (ptp_device_type(r) == PTP_DEV_EOS) {
		rc = ptp_eos_set_remote_mode(r, 1);
		if (rc) goto error;
		rc = ptp_eos_set_event_mode(r, 1);
		if (rc) goto error;
	} else {
		wcit_dbg("Refusing to connect to non-EOS\n");
		goto error;
	}

	rc = ptp_liveview_init(r);
	if (rc) {
		printf("Failed to init liveview %d\n", rc);
		goto error;
	}

	struct PtpLiveviewParams param;
	ptp_liveview_params(r, &param);
	int type = ptp_liveview_type(r);
	while (1) {
		if (type == PTP_LV_EOS) {
			rc = ptp_eos_get_liveview(r);
			if (rc == PTP_CHECK_CODE || rc == 0) {
				usleep(10);
				if (first_frame == 0) {
					FILE *f = fopen("assets/nosignal.jpg", "rb");
					if (f == NULL) continue;

					fseek(f, 0, SEEK_END);
					long file_size = ftell(f);
					fseek(f, 0, SEEK_SET);

					char *buffer = malloc(file_size);
					fread(buffer, 1, file_size, f);
					fclose(f);

					fwrite(buffer, file_size, 1, video_pipe);

					free(buffer);
				}
				
				continue;
			} else if (rc < 0) {
				printf("Error getting liveview frames: %d\n", rc);
				goto error;
			}

			size_t sent = fwrite(ptp_get_payload(r) + param.payload_offset_to_data, rc, 1, current_pipe);
			if (sent != rc) {
				// ...
			}
			first_frame = 1;
		} else {
			printf("Unknown liveview type");
			goto error;
		}

		usleep(10);
	}

	ptp_close_session(r);
	error:;
	ptp_device_close(r);
	ptp_close(r);

	fclose(video_pipe);
	return 0;	
}

void *main_app_thread(void *arg) {
	(void)arg;
	register_camera();
	pthread_exit(NULL);
	return NULL;
}

static void usb_discover(void *arg) {
	printf("USB device discovered\n");
	fflush(stdout);

	pthread_t *ptp_thread = (pthread_t *)arg;

	pthread_create(ptp_thread, NULL, main_app_thread, NULL);
}

int main(int argc, char **argv) {
	signal(SIGINT, on_quit);

	pthread_t ptp_thread;

	usb_discover(&ptp_thread);

	usb_monitor_start_thread(usb_discover, &ptp_thread);

	// Do UI things
	for (;;) pause();

	return 0;
}
