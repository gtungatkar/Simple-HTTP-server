#ifndef __LOGGER__
#define __LOGGER__
#ifndef TRACE
#define TRACE 1
#endif
#include <unistd.h>
#define LOG(_hdl, _msg)                                 \
{                                                       \
        if(TRACE > 0)                                   \
        fprintf(_hdl,"Pid=%d:%s %s",getpid(),__FUNCTION__, _msg);       \
}
#endif
