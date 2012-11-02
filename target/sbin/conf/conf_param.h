#ifndef _PARAM_H_
#define _PARAM_H_

#include <errno.h>
#include <conf_strbuf.h>

#ifndef INDENT
#define INDENT "    "
#endif

#define DEBUG
#ifdef DEBUG
#define DPRINTF(fmt, ...) printf("%s:%d: " fmt, __func__, __LINE__, __VA_ARGS__)
#else
#define DPRINTF(...)
#endif

#endif /* _PARAM_H_ */
