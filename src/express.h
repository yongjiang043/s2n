#ifndef _EXPRESS_H
#define _EXPRESS_H

#include "spin.h"
#include "smv.h"

void    checkTypeCompatible(Next *);
char    *complicatingName(Varinf *, Varinf *, int);
Express *translateConstToExpress(int);
Express *translateUnaryExpress(int, Express *);
Express *translateBinaryExpress(Express *, Express *, int);
Express *expressForExclusive(int, int);
Express *elementOfChanModule(Module *, int, int);
Express *conditionOfChanIo(Lextok *, Varinf *, int);
Express *expressOfPointInChanModule(Lextok *, int, int);
Express *translateExpress(Lextok *);
Express *expressOfProvider(Lextok *);
Express *putExpress(Express *, FILE *);

#endif
