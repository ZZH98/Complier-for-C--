#ifndef _IR_H_
#define _IR_H_

#include "ParseTree.h"
#include <stdio.h>
#include <stdlib.h>

int lblcnt;
int tmpcnt;

struct ICNode {
	int type; // type1/type2/type3
	union {
		struct {
			enum {LABEL, FUNCTION, GOTO, RETURN, DEC, ARG, PARAM, READ, WRITE} op;
			Operand arg;
			int decsize; //only for DEC
		} type1;
		struct {
			enum {ASSIGN, CALL} op; //ASSIGN include things like x := &y
			Operand arg1;
			Operand arg2;
		} type2;
		struct {
			enum {PLUS, MINUS, STAR, DIV, IF} op;
			int relop;
			Operand arg1;
			Operand arg2;
			Operand arg3;
		} type3;
	};
	struct ICNode* pre;
	struct ICNode* next;
};
typedef struct ICNode ICNode;

ICNode* IChead;
ICNode* ICtail;

void fillInSize(Node* r);

void translate_Exp(Node* r);
void translate_Cond(Node* r, Operand lb1, Operand lb2);
void translate_Stmt(Node* r);
void translate_Def(Node* r);
void translate_Exp2Cond(Node* r);

void genIR(Node* r);

void printIR(FILE* outfile);

void delIR();

#endif
