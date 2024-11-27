// Test EOS Liveview
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <signal.h>
#include <camlib.h>

static struct PtpRuntime *global_r;
static FILE *current_pipe;

//void ptp_verbose_log(char *fmt, ...) {
//	(void)fmt;
//}

FILE *create_ffmpeg_window_pipe() {
	char buffer[1024];
	sprintf(buffer,
		"%s -f mjpeg -i - -vcodec rawvideo -pix_fmt yuv420p -threads 0 -f sdl2 \"%s\"",
		"ffmpeg",
		"Test Window"
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

FILE *create_ffmpeg_video_pipe() {
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
	printf("Closing down\n");
	struct PtpRuntime *r = global_r;
	ptp_mutex_lock(r);
	ptp_close_session(r);
	ptp_device_close(r);
	ptp_mutex_unlock(r);

	// Have to wait on other threads to exit to close r
	// ptp_close(r);

	printf("Closing output pipe\n");
	pclose(current_pipe);
	printf("Goodbye\n");
	exit(0);
}

int liveview_thread(struct PtpRuntime *r) {
	if (ptp_device_init(r)) {
		puts("No device found");
		ptp_close(r);
		return 0;
	}

	FILE *p = create_ffmpeg_video_pipe();
	if (p == NULL) return -1;
	current_pipe = p;

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
			if (rc < 0) {
				printf("Error getting liveview frames %x\n", ptp_get_return_code(r));
				goto error;
			} else if (rc == 0) {
				usleep(1000);
				continue;
			}

			int sent = fwrite(ptp_get_payload(r) + param.payload_offset_to_data, rc, 1, p);
		} else {
			printf("Unknown liveview type");
			goto error;
		}

		usleep(10);
	}

	ptp_close_session(r);
	error:;
//	ptp_device_close(r);
//	ptp_close(r);

	fclose(p);
	return 0;	
}

void *main_app_thread(void *arg) {
	struct PtpRuntime *r = (struct PtpRuntime *)arg;
	liveview_thread(r);
	pthread_exit(NULL);
	return NULL;
}

int main(int argc, char **argv) {
	signal(SIGINT, on_quit);

	pthread_t thread;

	struct PtpRuntime *r = ptp_new(PTP_USB);
	global_r = r;

	if (pthread_create(&thread, NULL, main_app_thread, r)) {
		return -1;
	}

	pthread_join(thread, NULL);

	return 0;
}
