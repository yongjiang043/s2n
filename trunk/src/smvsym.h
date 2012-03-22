#ifndef _SMVSYM_H
#define _SMVSYM_H

int sameContext(Module *,Module *);
Module *lookupModule(char *);
Varinf *lookupVar(char *);
void   ready(Varinf *);

#endif
