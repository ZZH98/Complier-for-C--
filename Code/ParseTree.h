#ifndef _PARSETREE_H_
#define _PARSETREE_H_

#include "typeSys.h"

struct Operand {
	enum {LBL, VAR, TMP, FUN, NUM} type;
	union {
		int num; //LBL(label), TMP(t), VAR(v), NUM(only int in this lab)
		char* name; //FUN
	};
	int ctrl; //about ADR(1) and PTR(2)
};
typedef struct Operand Operand;

struct Node{
	char* name;
	int lineno;
	int colno;
	int nodetype;
	TypeNode* type;
	union {
		int ival;
		double fval;
		char* cval;
	};
	int stridx; // ID in which struct?
	enum {left, right} pos; //left: both sides of '='. right: only right of '='.
	Operand ICope;
	struct Node* child;
	struct Node* nextSib;
};
typedef struct Node Node;

Node* createNode(const char* nodeName, int ln, int cn, ...);

Node* createTree(const char* rootName, int value, int ln, int cn, int arg_num, ...);

void delTree();

#endif