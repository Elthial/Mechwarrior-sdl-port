#include "mechwarrior/port_diagnostics.h"
#include <stdio.h>
#include <stdlib.h>

_Noreturn void MwAbortWithReason(const char *category,const char *reason,
    const char *file,int line,const char *function)
{
    fprintf(stderr,"MechWarrior %s: %s\nFunction: %s\nSource: %s:%d\n",
        category,reason,function,file,line);
    fflush(stderr);
    abort(); /* Deliberate host stop; preserve SIGABRT diagnostics and tests. */
}
