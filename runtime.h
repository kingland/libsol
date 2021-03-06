
#ifndef RUNTIME_H
#define	RUNTIME_H

#include "sol.h"
#include "solfunc.h"
#include "sollist.h"
#include "solop.h"
#include "soltoken.h"
#include "soltypes.h"
#include "solerror.h"
#include "solevent.h"
#include "solar.h"

void sol_runtime_init();
void sol_runtime_destroy();

SolObject sol_runtime_execute(unsigned char* data);

#endif	/* RUNTIME_H */

