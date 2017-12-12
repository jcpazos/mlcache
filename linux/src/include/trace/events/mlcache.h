#undef TRACE_SYSTEM
#define TRACE_SYSTEM mlcache

#if !defined(_TRACE_9P_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_MLCACHE_H

#include <linux/tracepoint.h>

#define MLCACHE_HIT (0)
#define MLCACHE_MISS (1)

DECLARE_TRACE(mlcache_event,
				TP_PROTO(pgoff_t offset, pid_t pid, struct page *page, struct address_space *mapping, bool hit),
				TP_ARGS(offset, pid, page, mapping, hit));

#endif /* _TRACE_MLCACHE_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
