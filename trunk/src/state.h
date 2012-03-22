#ifndef _STATE_H
#define _STATE_H
#include "spin.h"

void buildNext(Element *, Module *);
void checkIfAllVariableHasNext();
void addDefaultToAllNext(Module *);

#endif
