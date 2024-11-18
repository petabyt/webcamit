CAMLIB := $(HOME)/AndroidStudioProjects/fudge/lib/camlib
ifeq ($(wildcard $(CAMLIB)),)
CAMLIB := $(HOME)/Documents/camlib
endif

CAMLIB_A := $(CAMLIB)/libcamlib.a

CFLAGS := -I$(CAMLIB)/src

ifdef VCAM
LDFLAGS += -lusb-vcam
else
LDFLAGS += -lusb-1.0
endif

webcamit.out: camlib main.c
	$(CC) $(CFLAGS) main.c $(CAMLIB_A) $(LDFLAGS) -o webcamit.out

.PHONY: camlib
camlib:
	cd $(CAMLIB) && make TARGET=l libcamlib.a

clean:
	rm -rf *.out *.o
