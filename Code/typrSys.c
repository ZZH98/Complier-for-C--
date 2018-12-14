#include "typeSys.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

TypeNode* allTypeNodes[10000];
int tn_idx = 0;
TypeNode* allArgNodes[10000];
int arg_idx = 0;

extern int debug;
int ty_debug = 0;

TypeNode* createInteger(int val, char* name) {
	TypeNode *p = malloc(sizeof(TypeNode));
	p->type = type_int;
	if (strcmp(name, "TYPE") != 0)
		p->ival = val;
	p->size = 4;
	p->left = NULL;
	p->right = NULL;
	if (debug == 1 && ty_debug == 1)
		printf("type integer to node %s\n", name);
	
	allTypeNodes[tn_idx] = p;
	tn_idx++;
	return p;
}

TypeNode* createFloat(float val, char* name) {
	TypeNode *p = malloc(sizeof(TypeNode));
	p->type = type_float;
	if (strcmp(name, "TYPE") != 0)
		p->fval = val;
	p->size = 4;
	p->left = NULL;
	p->right = NULL;
	if (debug == 1 && ty_debug == 1)
		printf("type float to node %s\n", name);
	
	allTypeNodes[tn_idx] = p;
	tn_idx++;
	return p;
}

TypeNode* createRecord(int stridx, char* name) {
	TypeNode *p = malloc(sizeof(TypeNode));
	p->type = record;
	p->stridx = stridx;
	p->size = 0; // filled in later
	p->left = NULL;
	p->right = NULL;
	if (debug == 1 && ty_debug == 1)
		printf("type struct to node %s\n", name);
	
	allTypeNodes[tn_idx] = p;
	tn_idx++;
	return p;
}

TypeNode* createArray(int index, TypeNode* arr) {
	TypeNode* tn = malloc(sizeof(TypeNode));
	tn->type = array;
	allTypeNodes[tn_idx] = tn;
	tn_idx++;

	TypeNode* ltn = malloc(sizeof(TypeNode));
	ltn->type = type_int;
	ltn->ival = index;
	allTypeNodes[tn_idx] = ltn;
	tn_idx++;

	// tn->size = index * arr->size; // cal the total size of an array
	// printf("idex = %d, arr->size = %d, tn->size = %d\n", index, arr->size, tn->size);

	tn->left = ltn;
	tn->right = arr;
	return tn;
}

int judgeType(TypeNode* r) {
	if (r == NULL)
		return -1;
	int res = -1;
	switch (r->type) {
		case 0: res = 0; break;
		case 1: res = 1; break;
		case 2: res = 2; break;
		case 3: res = 3; break;
		default: break;
	}
	return res;
}

int compType(TypeNode* r1, TypeNode* r2) {
	if (r1 == NULL && r2 == NULL) {
		return 1;
	}
	if ((r1 == NULL && r2 != NULL) || (r1 != NULL && r2 == NULL))
		return 0;
	if (debug == 1 && ty_debug == 1) printf("compare type\n");
	if (r1->type != r2->type)
		return 0;
	else if (r1->type == r2->type && r1->type == record && r1->stridx != r2->stridx) {
		return 0;
	}
	else {
		if (compType(r1->left, r2->left) && compType(r1->right, r2->right))
			return 1;
		else 
			return 0;
	}
}

TypeNode* copyType(TypeNode* r) {
	TypeNode* ct = malloc(sizeof(TypeNode));
	memcpy(ct, r, sizeof(TypeNode));
	allTypeNodes[tn_idx] = ct;
	tn_idx++;
	return ct;
}

void printType(TypeNode* r, int layer) {
	for (int i = 0; i < layer; i++)
		printf("  ");
	if (r == NULL)
		return;
	switch (r->type) {
		case 0: printf("int:\n"); break;
		case 1: printf("float: \n"); break;
		case 2: printf("array\n"); break;
		case 3: printf("record %d\n", r->stridx); break;
		default: break;
	}
	if (r->left != NULL)
		printType(r->left, layer + 1);
	if (r->right != NULL)
		printType(r->right, layer + 1);
	return;
}

void delTypeList() {
	for (int i = 0; i < tn_idx; i++) {
		/*DimNode* p = allTypeNodes[tn_idx]->dims;
		while (p != NULL) {
			printf("%d\n", p->dim);
			p = p->next;
		}*/
		free(allTypeNodes[tn_idx]);
	}
	tn_idx = 0;
}