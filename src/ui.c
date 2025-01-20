#include <stdio.h>
#include <ui.h>

int onClosing(uiWindow *w, void *data)
{
	uiQuit();
	return 1;
}

int main(void)
{
	uiInitOptions o = {0};
	const char *err;
	uiWindow *w;
	uiLabel *l;

	err = uiInit(&o);
	if (err != NULL) {
		fprintf(stderr, "Error initializing libui: %s\n", err);
		uiFreeInitError(err);
		return 1;
	}

	// Create a new window
	w = uiNewWindow("webcamit", 500, 1000, 0);
	uiWindowOnClosing(w, onClosing, NULL);

	uiBox *box = uiNewVerticalBox();
	uiBoxSetPadded(box, 1);

	l = uiNewLabel("webcamit");
	uiBoxAppend(box, uiControl(l), 1);

	uiWindowSetChild(w, uiControl(box));
	uiControlShow(uiControl(w));
	uiMain();
	uiUninit();
	return 0;
}
