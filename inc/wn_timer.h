#ifndef _WN_TIMER_H_
#define _WN_TIMER_H_

#include "stdint.h"

typedef struct Timer {
    uint32_t timeout_interval;
    uint32_t timeout;
    uint32_t repeat;
    void *session;
    void (*timeout_cb)(void *session);
    struct Timer* next;
}Timer;

#ifdef __cplusplus
extern "C" {
#endif

void timer_init(struct Timer* handle, void(*timeout_cb)(),void *session ,uint32_t timeout, uint32_t repeat);
void timer_start(struct Timer* handle);
void timer_reset(struct Timer* handle);
void timer_stop(struct Timer* handle);
void timer_loop(void);

#ifdef __cplusplus
}
#endif

#endif
