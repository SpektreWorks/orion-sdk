#include "OrionComm.h"


BOOL OrionCommOpen(int *pArgc, char ***pArgv)
{
    // If there are at least two arguments, and the first looks like a serial port or IP
    if (*pArgc >= 2)
    {
        // IP address...?
        if (OrionCommIpStringValid((*pArgv)[1]))
        {
            // Try connecting to a gimbal at this IP
            return OrionCommOpenNetworkIp((*pArgv)[1]);
        }
    }

    // If we haven't connected any other way, try using network broadcast
    return OrionCommOpenNetwork();

}// OrionCommOpen
