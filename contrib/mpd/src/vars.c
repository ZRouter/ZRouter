
/*
 * vars.c
 *
 * Written by Archie Cobbs <archie@freebsd.org>
 * Copyright (c) 1995-1999 Whistle Communications, Inc. All rights reserved.
 * See ``COPYRIGHT.whistle''
 */

#include "ppp.h"
#include "console.h"

/*
 * DEFINITIONS
 */

  #define ENABLE	1
  #define DISABLE	2
  #define ACCEPT	3
  #define DENY		4

  #define NO_SCOLD	0x100

/*
 * INTERNAL FUNCTIONS
 */

  static void	Do(int which, int ac, char *av[], Options o, ConfInfo list);
  static int	FindOpt(ConfInfo list, const char *name);

/*
 * OptStat()
 */

void
OptStat(Context ctx, Options opt, ConfInfo list)
{
    int		k, peered = 0;

    for (k = 0; list[k].name; k++) {
	if (list[k].peered) {
	    peered=1;
	    break;
	}
    }

    if (peered)
	Printf("\t\t\tSelf\t\tPeer\r\n");
    for (k = 0; list[k].name; k++) {
	ConfInfo	const c = &list[k];

	Printf("\t%-10s\t%s",
    	    c->name, Enabled(opt, c->option) ? "enable" : "disable");
	if (c->peered)
    	    Printf("\t\t%s", Acceptable(opt, c->option) ? "accept" : "deny");
	Printf("\r\n");
    }
}

/*
 * NoCommand()
 */

void
NoCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(DISABLE, ac, av, opt, list);
  Do(DENY | NO_SCOLD, ac, av, opt, list);
}

/*
 * YesCommand()
 */

void
YesCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(ENABLE, ac, av, opt, list);
  Do(ACCEPT | NO_SCOLD, ac, av, opt, list);
}

/*
 * EnableCommand()
 */

void
EnableCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(ENABLE, ac, av, opt, list);
}

/*
 * DisableCommand()
 */

void
DisableCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(DISABLE, ac, av, opt, list);
}

/*
 * AcceptCommand()
 */

void
AcceptCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(ACCEPT, ac, av, opt, list);
}

/*
 * DenyCommand()
 */

void
DenyCommand(int ac, char *av[], Options opt, ConfInfo list)
{
  Do(DENY, ac, av, opt, list);
}

/*
 * Do()
 */

static void
Do(int which, int ac, char *av[], Options opt, ConfInfo list)
{
  const int	scold = !(which & NO_SCOLD);

  which &= ~NO_SCOLD;
  for ( ; ac > 0; av++, ac--)
  {
    const int	index = FindOpt(list, *av);

    switch (index)
    {
      case -1:
	Log(LG_ERR, ("option \"%s\" unknown", *av));
	break;
      case -2:
	Log(LG_ERR, ("option \"%s\" ambiguous", *av));
	break;
      default:
	{
	  ConfInfo	const c = &list[index];

	  switch (which)
	  {
	    case ENABLE:
	      Enable(opt, c->option);
	      break;
	    case DISABLE:
	      Disable(opt, c->option);
	      break;
	    case ACCEPT:
	      if (!c->peered)
	      {
		if (scold)
		  Log(LG_ERR, ("'%s %s' is not applicable",
		    "accept", c->name));
	      }
	      else
		Accept(opt, c->option);
	      break;
	    case DENY:
	      if (!c->peered)
	      {
		if (scold)
		  Log(LG_ERR, ("'%s %s' is not applicable", "deny", c->name));
	      }
	      else
		Deny(opt, c->option);
	      break;
	    default:
	      assert(0);
	  }
	}
	break;
    }
  }
}

/*
 * FindOpt()
 *
 * Returns index of option, -1 if not found, -2 if ambiguous
 */

static int
FindOpt(ConfInfo list, const char *name)
{
    int		k, found = -1;

    for (k = 0; list[k].name; k++) {
	if (strncasecmp(list[k].name, name, strlen(name)) == 0) {
    	    if (found == -1)
		found = k;
    	    else {
		found = -2;		/* ambiguous */
    		break;
	    }
	}
    }
    return(found);
}


