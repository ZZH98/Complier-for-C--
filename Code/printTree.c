#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ParseTree.h"

void printTree(Node* n, int layer) {
	/*if (n->child == NULL && n->nodetype == 1)
		return;*/
	for (int i = 0; i < layer; i++)
		printf("  ");
	printf("%s", n->name);

	if (strcmp(n->name, "INT") == 0) {
		printf(": %d ", n->ival);
	}
	else if (strcmp(n->name, "FLOAT") == 0) {
		printf(": %f ", n->fval);
	}
	else if (strcmp(n->name, "ID") == 0 || strcmp(n->name, "TYPE") == 0) {
		printf(": %s ", n->cval);
	}
	printf(" (%d)", n->lineno);
	printf("\n");
	Node* c = n->child;
	while (c != NULL) {
		printTree(c, layer + 1);
		c = c->nextSib;
	}
	return;
}
