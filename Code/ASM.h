#ifndef _ASM_H_
#define _ASM_H_

#include "ParseTree.h"
#include <stdio.h>
#include <stdlib.h>

struct RegDesc {
	int filled;
	int varidx;
};
typedef struct RegDesc RegDesc;

struct VarDesc {
	int reg;
	int offset;
};
typedef struct VarDesc VarDesc;

void genASM(FILE* outfile);

#endif