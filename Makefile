CAMLIB := $(HOME)/AndroidStudioProjects/fudge/lib/camlib
ifeq ($(wildcard $(CAMLIB)),)
CAMLIB := $(HOME)/Documents/fudge/lib/camlib
endif

CAMLIB_A := $(CAMLIB)/libcamlib.a

CFLAGS := -I$(CAMLIB)/src -g

ifdef VCAM
LDFLAGS += -lusb-vcam
else
LDFLAGS += -lusb-1.0
endif

OUT := src/main.o src/usb.o

webcamit.out: camlib $(OUT)
	$(CC) $(OUT) $(CAMLIB_A) $(LDFLAGS) -ludev -o webcamit.out

.PHONY: camlib
camlib:
	cd $(CAMLIB) && make TARGET=l libcamlib.a

clean:
	rm -rf *.out *.o src/*.o src/*.d
