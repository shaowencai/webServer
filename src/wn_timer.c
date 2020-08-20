#include "wn_timer.h"
#include "time.h"
#include "stdio.h"

static struct Timer* head_handle = NULL; //timer handle list head.


void timer_init(struct Timer* handle, void(*timeout_cb)(),void *session ,uint32_t timeout, uint32_t repeat)
{
    handle->timeout_cb = timeout_cb;
    handle->session = session;
    handle->timeout_interval = timeout;
    handle->timeout = time(0) + timeout;
    handle->repeat = repeat;
}

void timer_reset(struct Timer* handle)
{
    handle->timeout = time(0) + handle->timeout_interval;
}


void timer_start(struct Timer* handle)
{
    struct Timer* target = head_handle;
    while(target)
    {
        if(target == handle) return;
        target = target->next;
    }
    handle->next = head_handle;
    head_handle = handle;
}


void timer_stop(struct Timer* handle)
{
    struct Timer** curr;
    for(curr = &head_handle; *curr; )
    {
        struct Timer* entry = *curr;
        if (entry == handle)
        {
            *curr = entry->next;
        }
        else
        {
            curr = &entry->next;
        }
    }
}


void timer_loop()
{
    struct Timer* target;
    for(target=head_handle; target; target=target->next)
    {
        if( time(0) >= target->timeout)
        {
            if(target->repeat == 0)
            {
                timer_stop(target);
            }
            else
            {
                target->timeout =  time(0) + target->repeat;
            }
            target->timeout_cb(target->session);
        }
    }
}
