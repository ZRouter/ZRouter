
/*
 * assert.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "log.h"

void
DoAssert(const char *file, int line, const char *failedexpr)
{
  Log(LG_ERR, ("ASSERT \"%s\" failed: file \"%s\", line %d",
    failedexpr, file, line));
  DoExit(EX_ERRDEAD);
}

