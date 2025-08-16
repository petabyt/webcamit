#include <libpict.h>

// TODO: Take some kind of hint as to what USB port to connect to
// TODO: Lock a mutex while connection is happening
int add_connection(void) {
	int rc = 0;
	struct PtpRuntime *r = ptp_new(PTP_USB);

	if (r != NULL) {
		printf("Only one connection supported right now.\n");
	}

	rc = ptp_device_init(r);
	if (rc) {
		printf("No device found\n");
		ptp_close(r);
		return rc;
	}

	struct Connection *c = &state.connection;
	memset(c, 0, sizeof(struct Connection));
	c->id = 123456; // generate ID
	c->is_active = 1;
	c->r = r;

	rc = ptp_open_session(r);
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
		printf("Refusing to connect to non-EOS\n");
		goto error;
	}

	rc = ptp_liveview_init(r);
	if (rc) {
		printf("Failed to init liveview %d\n", rc);
		goto error;
	}

	ptp_liveview_params(r, &c->param);

	if (ptp_liveview_type(r) != PTP_LV_EOS) {
		printf("Only Canon supported\n");
		goto error;
	}

	return 0;
	error:;
	ptp_device_close(r);
	ptp_close(r);
	c->is_active = 0;
	return rc;
}

int close_connection(struct Connection *c) {
	struct PtpRuntime *r = c->r;
	ptp_mutex_lock(r);
	ptp_close_session(r);
	ptp_device_close(r);
	printf("quit kill switch %d\n", r->operation_kill_switch);
	c->is_active = 0;
	ptp_mutex_unlock(r);
}
