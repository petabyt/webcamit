// Test EOS Liveview
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
#define JPEG_LIB_VERSION 62
#include <jpeglib.h>
#include <turbojpeg.h>
#include <gphoto2/gphoto2.h>
#include <gphoto2/gphoto2-camera.h>
//void ptp_verbose_log(char *fmt, ...) {}
//void ptp_error_log(char *fmt, ...) {}
//__attribute__ ((noreturn)) void ptp_panic(char *fmt, ...) {abort();}
#include "wcit.h"

struct Connection {
	// If zero, this struct is invalid and free
	int is_active;
	// unique path/ID/address of connection
	char path[128];
	// Has first liveview frame been recieved
	int first_frame;
	int v4l2_fd;
	int v4l2_fd_valid;
	uint8_t *framebuffer;
	unsigned int framebuffer_size;

#ifdef LIBPICT
	struct PtpRuntime *r;
	struct PtpLiveviewParams param;
#endif
	Camera *camera;
};

struct State {
	//struct Connection connections[10];
	struct Connection connection;
	GPContext *context;
}state = {0};

void close_down_connection(struct Connection *c) {
	gp_camera_exit(c->camera, state.context);
	c->is_active = 0;
	// TODO: gracefully close down v4l2 pipe
}

void on_quit(int status) {
	printf("Closing down all connections\n");
	if (state.connection.is_active) {
		close_down_connection(&state.connection);
		// TODO: Wait for liveview thread to close down
	}

	printf("Goodbye\n");
	exit(0);
}

int open_v4l2_pipe(unsigned int width, unsigned int height, int *fd_ptr) {
	int fd = open("/dev/video10", O_RDWR);
	if (fd < 0) {
		printf("Failed to open v4l2 pipe\n");
	}

	struct v4l2_capability vid_caps;
	struct v4l2_format vid_format;

	int rc = ioctl(fd, VIDIOC_QUERYCAP, &vid_caps);
	if (rc == -1) return rc;

	memset(&vid_format, 0, sizeof(vid_format));

	vid_format.type                 = V4L2_BUF_TYPE_VIDEO_OUTPUT;
	vid_format.fmt.pix.width        = width;
	vid_format.fmt.pix.height       = height;
	vid_format.fmt.pix.field        = V4L2_FIELD_NONE;
	vid_format.fmt.pix.colorspace   = V4L2_COLORSPACE_JPEG;
	vid_format.fmt.pix.pixelformat  = V4L2_PIX_FMT_YUV420;
	vid_format.fmt.pix.bytesperline = width * 2;
	vid_format.fmt.pix.sizeimage    = width * height * 2;

	rc = ioctl(fd, VIDIOC_S_FMT, &vid_format);
	*fd_ptr = fd;
	return rc;
}

static void errordumper(GPLogLevel level, const char *domain, const char *str, void *data) {
	fprintf(stdout, "%s\n", str);
}

// TODO: Take some kind of hint as to what USB port to connect to
// TODO: Lock a mutex while connection is happening
int add_connection(void) {
	printf("Connecting to camera\n");
	struct Connection *c = &state.connection;
	memset(c, 0, sizeof(struct Connection));

	int	retval;

	gp_log_add_func(GP_LOG_ERROR, errordumper, NULL);

	gp_camera_new(&c->camera);

	retval = gp_camera_init(c->camera, state.context);
	if (retval != GP_OK) {
		printf("Failed to connect: %d\n", retval);
		return -1;
	}

	GPPortInfo info;
	retval = gp_camera_get_port_info(c->camera, &info);
	if (retval != GP_OK) {
		printf("Failed to get port info\n");
		return -1;
	}

	char *path;
	gp_port_info_get_path(info, &path);
	strlcpy(c->path, path, sizeof(c->path));

	c->is_active = 1;
	return -1;
}

METHODDEF(void)my_error_exit(j_common_ptr cinfo) {
	struct jpeg_error_mgr *err = cinfo->err;
	char buffer[1024];
	err->format_message(cinfo, buffer);
	printf("%s\n", buffer);
	abort();
}


int send_mjpeg(struct Connection *c, const void *ptr, unsigned int size) {
	tjhandle tj = NULL;
	int w, h, subsamp, colorspace;
	int rc;

	tj = tjInitDecompress();
	if (!tj) {
		fprintf(stderr, "tjInitDecompress failed\n");
		return -1;
	}

	rc = tjDecompressHeader3(tj, ptr, size, &w, &h, &subsamp, &colorspace);
	if (rc) {
		fprintf(stderr, "Header read failed: %s\n", tjGetErrorStr2(tj));
		tjDestroy(tj);
		return -1;
	}

	if (!c->v4l2_fd_valid) {
		printf("Opening pipe as %dx%d\n", w, h);
		rc = open_v4l2_pipe(w, h, &c->v4l2_fd);
		if (rc) {
			printf("Error opening v4l2 pipe\n");
			tjDestroy(tj);
			return -1;
		}
		c->v4l2_fd_valid = 1;
	}

	int y_size = w * h;
	int uv_size = (w/2) * (h/2);
	size_t frame_size = y_size + 2*uv_size;

	int uv_width = (w + 1) / 2;   // round up for 4:2:0
	int uv_height = (h + 1) / 2;

	if (!c->framebuffer || frame_size > c->framebuffer_size) {
		free(c->framebuffer);
		c->framebuffer_size = frame_size;
		c->framebuffer = malloc(frame_size);
		if (!c->framebuffer) {
			fprintf(stderr, "Failed to allocate framebuffer\n");
			tjDestroy(tj);
			return -1;
		}
	}

	unsigned char *planes[3];
	planes[0] = c->framebuffer;             // Y
	planes[1] = c->framebuffer + y_size;    // U
	planes[2] = c->framebuffer + y_size + uv_size; // V

	int strides[3] = {
		w,
		uv_width,
		uv_width,
	};

	if (colorspace == TJCS_YCbCr && subsamp == TJSAMP_420) {
		// tjDecompressToYUVPlanes
		// rc = tjDecompressToYUVPlanes(tj, ptr, size, planes, w, strides, h, 0);
	} else {
		size_t pitch = w * tjPixelSize[TJPF_RGB];
		size_t rgbBufSize = pitch * h;
		uint8_t *buffer = malloc(rgbBufSize);
		rc = tjDecompress2(tj, ptr, size, buffer, w, (int)pitch, h, TJPF_RGB, 0);
		if (rc) {
			printf("tjDecompress2\n");
			tjDestroy(tj);
			return -1;
		}

		tjhandle *encoder = tjInitCompress();

		rc = tjEncodeYUVPlanes(encoder, buffer, w, (int)pitch, h, TJPF_RGB, planes, strides, TJSAMP_420, 0);
		if (rc) {
			fprintf(stderr, "tjDecompressToYUVPlanes failed: %s\n", tjGetErrorStr2(tj));
			tjDestroy(tj);
			return -1;
		}

		free(buffer);
	}

	ssize_t written = write(c->v4l2_fd, c->framebuffer, frame_size);
	if (written != (ssize_t)frame_size) {
		fprintf(stderr, "Short write %zd/%zu\n", written, frame_size);
	}

	tjDestroy(tj);
	return 0;
}

int cycle_connection(struct Connection *c) {
	CameraFile *gp_buf;
	gp_file_new(&gp_buf);
	int retval = gp_camera_capture_preview(c->camera, gp_buf, state.context);
	if (retval != GP_OK) {
		printf("Failed to capture preview\n");
		gp_file_unref(gp_buf);
		close_down_connection(c);
		return 0;
	}

	const char *frame;
	unsigned long int frame_size;

	printf("Getting jpeg\n");
	retval = gp_file_get_data_and_size(gp_buf, &frame, &frame_size);
	if (retval != GP_OK) {
		printf("Failed to get preview file\n");
		gp_file_unref(gp_buf);
		return 0;
	}

	c->first_frame = 1;

	printf("Sending jpeg to video stream\n");
	int rc = send_mjpeg(c, frame, frame_size);
	if (rc) {
		printf("Failed to send frame to video stream\n");
		close_down_connection(c);
	}

	gp_file_unref(gp_buf);

	return 0;
}

int register_camera(void) {

	return 0;	
}

void *main_app_thread(void *arg) {
	(void)arg;

	add_connection();
	
	while (1) {
		if (state.connection.is_active) {
			cycle_connection(&state.connection);
		} else {
			printf("Waiting for a connection to be active...\n");
		}
		usleep(1000 * 1000);
	}
	pthread_exit(NULL);
	return NULL;
}

static void usb_discover(void *arg) {
	printf("USB device discovered\n");
	// TODO: Send signal to thread
}

int main(int argc, char **argv) {
	state.context = gp_context_new();
	if (state.context == NULL) {
		printf("Failed to open gphoto context\n");
	}

	signal(SIGINT, on_quit);

	pthread_t ptp_thread;

	pthread_create(&ptp_thread, NULL, main_app_thread, NULL);

	usb_monitor_start_thread(usb_discover, &ptp_thread);

	// TODO: Do UI things
	for (;;) pause();

	return 0;
}
