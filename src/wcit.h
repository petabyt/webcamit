#pragma once

#define wcit_dbg printf

// usb.c
int usb_monitor_start_thread(void (*callback)(void *arg), void *arg);
