#ifndef _MODULE_H
#define _MODULE_H

#include "spin.h"

extern int   mid;
extern char *exclusive;
extern char *point;
extern Module   *s_context;
extern ModList  *Mlist;

Module *lookupChanModule(Lextok *);

#endif
