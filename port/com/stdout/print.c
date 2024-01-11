#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../interfaces.h"
#include "../../os/osal.h"
#include "../../../rlog.h"

bool rlog_stdout_init(void* me);
bool rlog_stdout_poll(void* me);
bool rlog_stdout_send(void* me, const void* buf, int len);

rlog_ifc_t rlog_stdout = {
    .init       = &rlog_stdout_init,
    .poll       = &rlog_stdout_poll,
    .send       = &rlog_stdout_send,
    .deinit     = NULL,
    .ctx        = NULL,
};

bool rlog_stdout_init(void* me)
{
    //stdout does not need to be initialized here
    return true;
}

bool rlog_stdout_poll(void* me)
{
    //stdout is always available
    return true;
}

bool rlog_stdout_send(void* me, const void* buf, int len)
{
    if( printf("%s", (const char*) buf) < 0 )
        return false;
        
    return true;
}