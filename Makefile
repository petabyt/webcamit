CAMLIB := $(HOME)/AndroidStudioProjects/fudge/lib/camlib

CFLAGS := -I$(CAMLIB)/src

webcamit.out: camlib main.c
	$(CC) $(CFLAGS) main.c $(CAMLIB)/libcamlib.a -lusb-vcam -o webcamit.out

.PHONY: camlib
camlib:
	cd $(CAMLIB) && make TARGET=l libcamlib.a

clean:
	rm -rf *.out *.o
