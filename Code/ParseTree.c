#include "ParseTree.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

Node* allNodes[10000];
int idx = 0;
extern int debug;

Node* createNode(const char* nodeName, int ln, int cn, ...) {
	va_list ap;
	va_start(ap, cn);
	Node *n = malloc(sizeof(Node));
	n->name = nodeName;
	if (strcmp(nodeName, "INT") == 0) {
		int val = va_arg(ap, int);
		n->ival = val;
		n->ICope.type = NUM;
		n->ICope.num = val;
	}
	else if (strcmp(nodeName, "FLOAT") == 0) {
		double val = va_arg(ap, double);
		n->fval = val;
	}
	else if (strcmp(nodeName, "ID") == 0 || strcmp(nodeName, "TYPE") == 0) {
		char* val = va_arg(ap, char*);
		n->cval = malloc(32);
		strcpy(n->cval, val);
	}
	n->lineno = ln;
	n->colno = cn;
	n->nodetype = 0;
	n->type = NULL;
	n->child = NULL;
	n->nextSib = NULL;

	allNodes[idx] = n;
	idx++;

	return n;
}

Node* createTree(const char* rootName, int value, int ln, int cn, int arg_num, ...) {
	Node *r = createNode(rootName, ln, cn, value);
	r->nodetype = 1;
	r->type = NULL;
	va_list ap;
	va_start(ap, arg_num);
	for (int i = 0; i < arg_num; i++) {
		Node* c = va_arg(ap, Node*);
		if (r->child == NULL) {
			r->child = c;
			continue;
		}
		Node* p = r->child;
		while (p->nextSib != NULL)
			p = p->nextSib;
		p->nextSib = c;
	}

	return r;
}

void delTree() {
	for (int i = 0; i < idx; i++) {
		free(allNodes[idx]);
	}
	idx = 0;
}