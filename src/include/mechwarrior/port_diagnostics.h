#ifndef MECHWARRIOR_PORT_DIAGNOSTICS_H
#define MECHWARRIOR_PORT_DIAGNOSTICS_H

/* Host diagnostics only, not recovered game procedures or new gameplay checks.
 * Call only at an existing fatal boundary. Keep its condition and ordering.
 * The reason is flushed before abort raises SIGABRT; the SDL entry's handler
 * can then add a stack trace. The launcher retains stderr in runtime.log. */
_Noreturn void MwAbortWithReason(const char *category,const char *reason,
    const char *file,int line,const char *function);

#define MW_PORT_FAILSAFE(reason) \
    MwAbortWithReason("port failsafe",reason,__FILE__,__LINE__,__func__)

/* An existing x86 fault represented by a host stop, not a newly added rule. */
#define MW_NATIVE_FAULT(reason) \
    MwAbortWithReason("native CPU fault emulation",reason,__FILE__,__LINE__,__func__)

#endif
