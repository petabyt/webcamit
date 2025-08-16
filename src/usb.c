// USB event scanner
#include <stdio.h>
#include <unistd.h>
#include <poll.h>
#include <sys/signalfd.h>
#include <signal.h>
#include <libudev.h>
#include <pthread.h>
#include <stdlib.h>
#include <libudev.h>
#include <string.h>

struct UsbMonitor {
	pthread_t thread;
	void *arg;
	void (*callback)(void *arg);
};

int usb_monitor_start(struct UsbMonitor *m) {
	int rc;
	struct udev *udev = udev_new();
	if (udev == NULL) abort();

	struct udev_monitor *monitor = udev_monitor_new_from_netlink(udev, "udev");
	struct pollfd pfd[2];
	udev_monitor_enable_receiving(monitor);

	sigset_t mask;

	// Set signals we want to catch
	sigemptyset(&mask);
	sigaddset(&mask, SIGTERM);
	//sigaddset(&mask, SIGINT);

	// Change the signal mask and check
	if (sigprocmask(SIG_BLOCK, &mask, NULL) < 0) abort();
	int signal_fd = signalfd(-1, &mask, 0);
	if (signal_fd < 0) abort();

	pfd[0].events = POLLIN;
	pfd[0].fd = signal_fd;
	pfd[1].events = POLLIN;
	pfd[1].fd = udev_monitor_get_fd(monitor);

	if (pfd[1].fd < 0) {
		printf("Error while getting hotplug monitor\n");
		udev_monitor_unref(monitor);
		return -1;
	}

	while (1) {
		rc = poll(pfd, 2, -1);
		if (rc < 0) return rc;

		if (pfd[0].revents & POLLIN) {
			printf("Signal\n");
			struct signalfd_siginfo signal_info;
			rc = read(pfd[0].fd, &signal_info, sizeof(signal_info));
			if (rc < 0) return rc;
			if (signal_info.ssi_signo == SIGINT || signal_info.ssi_signo == SIGTERM) {
				break;
			}
		}
		if (pfd[1].revents & POLLIN) {
			struct udev_device *device = udev_monitor_receive_device(monitor);
			if (device == NULL) continue;
			if (!strcmp(udev_device_get_action(device), "bind") && !strcmp(udev_device_get_subsystem(device), "usb")) {
				m->callback(m->arg);
			}
			udev_device_unref(device);
		}
	}

	udev_monitor_unref(monitor);

	return 0;
}

static void *usb_monitor_thread(void *arg) {
	int rc = usb_monitor_start((struct UsbMonitor *)arg);
	return NULL;
}

int usb_monitor_start_thread(void (*callback)(void *arg), void *arg) {
	struct UsbMonitor *mon = calloc(sizeof(struct UsbMonitor), 1);
	mon->callback = callback;
	mon->arg = arg;
	pthread_create(&mon->thread, NULL, usb_monitor_thread, mon);
	// Should return pid
	return 0;
}
