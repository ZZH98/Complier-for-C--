#include "semAnalysis.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

int debug_sem = 0;
extern int debug;

// deal with type system

TypeNode* VarDecType(Node* root, TypeNode* basetype) { //target: give the bottom "ID" type
	if(debug && debug_sem) printf("VarDecType: give type to line %d\n", root->lineno);
	//deal with high-dimesion array when making definitions
	root->type = basetype;
	Node* p = root->child; //VarDec or ID
	TypeNode* fathertype = basetype;

	while (strcmp(p->name, "ID") != 0) { //VarDec -> VarDec LB INT RB
		p->type = createArray(p->nextSib->nextSib->ival, fathertype);
		fathertype = p->type;

		p = p->child;
	}
	p->type = fathertype; //now p->name is "ID"

	if (debug && debug_sem) {
		printf("VarDecType: The type of ID %s is:\n", p->cval);
		printf("**********\n");
		printType(p->type, 0);
		printf("**********\n");
	}
	return p->type;
}

unsigned int hash_pjw(char* name) {
	unsigned int val = 0, i;
	for (; *name; ++name) {
		val = (val << 2) + *name;
		if (i = val & ~HASHLIST_LEN) 
			val = (val ^ (i >> 12)) & HASHLIST_LEN;
	}
	return val;
}

void initialType(Node* root) { //DecList
	if(debug && debug_sem) printf("initialType: intial type in line %d, name is %s\n", root->lineno, root->name);
	root->child->type = root->type; //Dec
	VarDecType(root->child->child/*VarDec*/, root->type); 
	if (root->child->nextSib != NULL) { //DecList -> Dec COMMA DecList
		Node* nextDL = root->child->nextSib->nextSib;
		nextDL->type = root->type;
		initialType(nextDL);
	}
	return;
}

void initialTypeExt(Node* root) {
	if(debug && debug_sem) printf("initialTypeExt: intial type in line %d, name is %s\n", root->lineno, root->name);
	if (strcmp(root->name, "ExtDecList") == 0) {
		VarDecType(root->child, root->type); //VarDec
		if (root->child->nextSib != NULL) { //ExtDecList ->Var Dec COMMA ExtDecList
			Node* nextEDL = root->child->nextSib->nextSib;
			nextEDL->type = root->type;
			initialType(nextEDL);
		}
	}
	return;
}

void fillInStrType(Node* root, int idx) {

	root->stridx = idx;

	Node* c = root->child;
	while (c != NULL) {
		fillInStrType(c, idx);
		c = c->nextSib;
	}
}

char strNames[STRUCT_NUM][MAX_VAR_LEN];

void fillInType(Node* r) {

	if (strcmp(r->name, "StructSpecifier") == 0 && strcmp(r->child->nextSib->name, "OptTag") == 0) {
		Node* opttag = r->child->nextSib; //OptTag
		opttag->stridx = sindex;
		opttag->type = createRecord(sindex, opttag->name);
		r->stridx = sindex; //StructSpecifier
		r->type = opttag->type;
		Node* p = r->child->nextSib->child; //ID
		if (p != NULL) {
			p->stridx = sindex; //the type of a struct is stuck to ID
			p->type = opttag->type;
			strcpy(strNames[sindex], p->cval);
			if (debug && debug_sem) printf("add %s to strNames[%d]\n", p->cval, sindex);
		}
		fillInStrType(r, sindex);
		sindex++;
	}

	if (strcmp(r->name, "StructSpecifier") == 0 && strcmp(r->child->nextSib->name, "Tag") == 0) {
		Node* tag = r->child->nextSib; //Tag
		Node* id = tag->child; //ID: name0
		int flag = 0;
		for (int i = 0; i < sindex; i++) {
			if (strcmp(id->cval, strNames[i]) == 0) {
				tag->type = createRecord(i, tag->name);
				fillInStrType(r, i);
				flag = 1;
				break;
			}
		}
		if (flag == 0) {
			tag->type = createRecord(-1, tag->name);
			fillInStrType(r, -1);
		}
		r->type = tag->type;
		id->type = tag->type; //important!!! give id the type of a struct
		if (debug && debug_sem) {
			printf("Tag: add type to ID %s, the type is:\n", id->cval);
			printType(id->type, 0);
		}
	}

	Node* c = r->child;
	while (c != NULL) {
		fillInType(c);
		c = c->nextSib;
	}

	if (strcmp(r->name, "Specifier") == 0) {
		if (debug && debug_sem) {
			printf("give %s's type to Specifier, the type is:\n", r->child->name);
			printType(r->child->type, 0);
		}
		r->type = r->child->type;
		if (strcmp(r->nextSib->name, "FunDec") == 0) {
			r->nextSib->type = r->type; //set the return type of function
			r->nextSib->child->type = r->type; /*ID*/
		}
	}

	//find the def of variable, initial its type
	if (strcmp(r->name, "Def") == 0) {
		r->child/*Specifier*/->nextSib/*DecList*/->type = r->child->type;
		initialType(r->child->nextSib/*DecList*/);
	}
	if (strcmp(r->name, "ExtDef") == 0) {
		if (strcmp(r->child->nextSib->name, "ExtDecList") == 0) { //ExtDef -> Specifier ExtDecList SEMI
			r->child->nextSib->type = r->child->type;
			initialTypeExt(r->child->nextSib/*ExtDecList*/);
		}
	}
}

void examineStruct(Node* root, int idx) {

	if (strcmp(root->name, "OptTag") == 0 && root->child != NULL) {
		Node* id = root->child;
		examineVarDef(id, 16);
	}

	if (strcmp(root->name, "VarDec") == 0 && strcmp(root->child->name, "ID") == 0 && root->child->stridx == idx) {
		//add to the symbol table of the struct
		Node* r = root->child; //ID
		unsigned int res = hash_pjw(r->cval);
		HashNode** strNode = structHashTableList[idx]; //type:HashNode**
		if (strNode[res] == NULL) {
			HashNode *hn = malloc(sizeof(HashNode));
			hn->name = r->cval;
			hn->type = r->type;
			hn->next = NULL;
			strNode[res] = hn;
		}
		else {
			HashNode* p = strNode[res];
			if (strcmp(p->name, r->cval) == 0)
				printf("Error Type 15 at line %d: Multi defined scope name \'%s\' in a struct.\n", r->lineno, r->cval); //find the same name
			while (p->next != NULL) {
				p = p->next;
				if (strcmp(p->name, r->cval) == 0) {
					printf("Error Type 15 at line %d: Multi defined scope name \'%s\' in a struct.\n", r->lineno, r->cval);
					break;
				}
				else if (p->next == NULL) {
					HashNode *hn = malloc(sizeof(HashNode));
					hn->name = r->cval;
					hn->type = r->type;
					hn->next = NULL;
					p->next = hn;
				}
			}
		}
	}

	if (strcmp(root->name, "Dec") == 0 && root->child->nextSib != NULL) {
		//stmt like "int var = 1" in a struct
		printf("Error Type 15 at line %d: Initialization of struct scope \'%s\' at the time of definition.\n", root->lineno, root->child->child->cval);
	}

	Node* c = root->child;
	while (c != NULL) {
		examineStruct(c, idx);
		c = c->nextSib;
	}
	return;
}

int examineFuncArgs(Node* root/*Exp*/, TypeNode* deftype) {
	if (debug && debug_sem) printf("examine Func Args\n");
	Node* myarg = root->child->nextSib->nextSib;
	if (myarg->nextSib == NULL){ //Exp -> ID LP RP
		if (deftype->args == NULL)
			return 1;
		else {
			printf("Error Type 9 at line %d: Formal paramters and actual paramters are not in consistence when calling function '%s'.\n", root->lineno, root->child->cval);
			return 0;
		}
	}
	TypeNode* defarg = deftype->args;
	myarg = myarg->child;
	while (1) { //Exp -> ID LP Args RP
		TypeNode* mytype = myarg->type;
		if (mytype == NULL) {
			if (debug && debug_sem) printf("error in function args\n");
			return 0;
		}
		if (debug && debug_sem) {
			printf("++++++++++\n");
			printType(mytype, 0);
			printf("++++++++++\n");
			printType(defarg, 0);
			printf("++++++++++\n");
		}
		if (!compType(mytype, defarg)) {
			printf("Error Type 9 at line %d: Formal paramters and actual paramters are not in consistence when calling function '%s'.\n", root->lineno, root->child->cval);
			return 0;
		}
		if (myarg->nextSib == NULL && defarg->argNext == NULL) {
			break;
		}
		else if (myarg->nextSib != NULL && defarg->argNext != NULL) {
			defarg = defarg->argNext;
			myarg = myarg->nextSib->nextSib->child;
		}
		else {
			printf("Error Type 9 at line %d: Formal paramters and actual paramters are not in consistence when calling function '%s'.\n", root->lineno, root->child->cval);
			return 0;
		}
		
	}
	return 1;
}

TypeNode* retype_temp;

void examineVarDef(Node* root, int errortype) {
	int res = addVar(root->cval, root->type);
	if (res == 1) {
		if (errortype == 3)  //variable
			printf("Error Type 3 at line %d: Multi defined variable name \'%s\'.\n", root->lineno, root->cval);
		else if (errortype == 16)  //struct
			printf("Error Type 16 at line %d: Multi defined struct name \'%s\'.\n", root->lineno, root->cval);
	}
	root->ICope.type = VAR;
	root->ICope.num = varcnt;
	varcnt++;
}

void examineFuncDef(Node* root) {
	int res = addFunc(root->cval, root->type);
	if (res == 1)
		printf("Error Type 4 at line %d: Multi defined function name \'%s\'.\n", root->lineno, root->cval);
}

HashNode* examineVarUse(Node* root, int errortype) {
	HashNode* res = findVar(root->cval);
	if (res == NULL) {
		if (errortype == 1) //variable
			printf("Error Type 1 at line %d: The variable \'%s\' is used without definition.\n", root->lineno, root->cval);
		else if (errortype == 17) //struct
			printf("Error Type 17 at line %d: The struct name \'%s\' is used without definition.\n", root->lineno, root->cval);
	}
	else {
		root->ICope = res->ICope;
		if (debug && debug_sem) printf("get ICope v%d out from list, origin name is %s\n", root->ICope.num, root->cval);
	}
	return res;
}

HashNode* examineFuncUse(Node* root) {
	HashNode* res = findFunc(root->cval);
	HashNode* res2 = findVar(root->cval);
	if (res == NULL && res2 == NULL)
		printf("Error Type 2 at line %d: The function \'%s\' is used without definition.\n", root->lineno, root->cval);
	if (res == NULL && res2 != NULL)
		printf("Error Type 11 at line %d: use \'(...)\' or \'()\' after a normal variable.\n", root->lineno);
	return res;
}

void fillInFuncType(Node* root/*FunDec*/) {
	Node* id = root->child; //ID
	if (debug && debug_sem) printf("fillInFunType: funcname = %s\n", id->cval);

	if (id->nextSib->nextSib->nextSib == NULL) { //FunDec -> ID LP RP
		id->type->args = NULL;
	}
	else { //FunDec -> ID LP VarList RP
		Node* p = id->nextSib->nextSib->child; //ParamDec
		//int tail = 0;
		TypeNode* tail = NULL;

		while (1) { //VarList -> ParamDec COMMA VarList | ParamDec
			Node* spe = p->child; //Specifier
			TypeNode* argtype = VarDecType(spe->nextSib, spe->type); //VarDec

			TypeNode* arg_in = copyType(argtype);

			if (id->type->args == NULL) {
				id->type->args = arg_in;
				tail = id->type->args;
			}
			else {
				tail->argNext = arg_in;
				tail = tail->argNext;
			}

			if (debug && debug_sem) printf("fill\n");
			if (p->nextSib == NULL)
				break;

			p = p->nextSib->nextSib->child;
		}
	}
	return;
}

void examineSem(Node* r) {
	//do before its children are visited

	//find a newly defined struct
	if (strcmp(r->name, "StructSpecifier") == 0 && strcmp(r->child->nextSib->name, "OptTag") == 0) {

		//find a newly defined struct
		int idx = r->child->nextSib->stridx;
		if(debug && debug_sem) printf("examineStruct: idx = %d\n", idx);
		structHashTableList[idx] = malloc(sizeof(HashNode*) * HASHLIST_LEN);
		memset(structHashTableList[idx], '\0', sizeof(HashNode*) * HASHLIST_LEN);
		examineStruct(r, idx); //StructSpecifier
	}

	//find the use of a struct
	if (strcmp(r->name, "Tag") == 0) {
		Node* id = r->child;
		HashNode* res = examineVarUse(id, 17);
		// size can be taken from HashNode
	}

	//find a newly defined variable
	if (strcmp(r->name, "VarDec") == 0 && r->child->nextSib == NULL) {
		Node* id = r->child;
		if (debug && debug_sem) {
			printf("call examineVarDef at line %d, name = %s, type is\n", r->lineno, id->cval);
			printf("-------------\n");
			printType(id->type, 0);
			printf("-------------\n");
		}

		examineVarDef(id, 3);
	}

	//find a newly defined function
	else if (strcmp(r->name, "FunDec") == 0) {
		Node* id = r->child;

		fillInFuncType(r);

		//finish the args, add the type to func list
		examineFuncDef(id);

		retype_temp = r->child->type;

		if (debug && debug_sem) printf("add type to func ext vari, and the type is:\n");
	}

	Node* c = r->child;
	while (c != NULL) {
		examineSem(c);
		c = c->nextSib;
	}

	//find the use of a variable
	//Exp -> ID
	if (strcmp(r->name, "Exp") == 0 && strcmp(r->child->name, "ID") == 0 && r->child->nextSib == NULL) {
		r->pos = left;
		Node* id = r->child;
		if (debug && debug_sem) printf("call examineVarUse, name = %s\n", id->cval);
		HashNode* res = examineVarUse(id, 1);
		if (res != NULL) {
			id->type = res->type;
			if (id->type->type == record)
				id->stridx = res->type->stridx; //use a defined struct variable later
		}
		else {
			id->type = NULL;
			return;
		}
		if (debug && debug_sem) {
			printf("get variable %s from symbol table, and the type is:\n", id->cval);
			printf("----------\n");
			printType(id->type, 0);
			printf("----------\n");
		}
		r->type = id->type;
		if (id->type->type == record)
			r->stridx = id->stridx; //use a defined struct variable later
	}

	//find the use of a function
	//Exp -> ID LP RP | ID LP Args RP
	else if(strcmp(r->name, "Exp") == 0 && strcmp(r->child->name, "ID") == 0) {
		r->pos = right;
		Node* id = r->child;
		HashNode* hn = examineFuncUse(id);
		if (hn != NULL) {
			id->type = hn->type;
			int res = examineFuncArgs(r, id->type);
		}
		else
			id->type = NULL;
		r->type = id->type;  //return type of function
	}

	else if(strcmp(r->name, "Exp") == 0) {
		fillInExpType(r);
	}

	else if (strcmp(r->name, "Dec") == 0 && r->child->nextSib != NULL) { // Dec -> VarDec ASSIGNOP Exp
		//
		Node* arg1 = r->child;
		Node* arg2 = r->child->nextSib->nextSib;
		if (arg2->type == NULL)
			return;
		if (!compType(arg1->type, arg2->type)) {
			printf("Error Type 5 at line %d: Cannot make assignments between different types.\n", r->lineno);
			return;
		}
	}

	else if (strcmp(r->name, "Stmt") == 0 && strcmp(r->child->name, "RETURN") == 0) {
		Node* retexp = r->child->nextSib; //Exp
		if (debug && debug_sem) {
			printf("examine ret ---\n");
			printType(retype_temp, 0);
			printf("+++\n");
			printType(retexp->type, 0);
			printf("---\n");
		}
		if (!compType(retype_temp, retexp->type)) {
			printf("Error Type 8 at line %d: Returning type that does not match function's defined returning type.\n", r->lineno);
		}
	}

	return;
}

void fillInExpType(Node* r) {
	//if find one type error, give up all the ancestors(by setting type to NULL

	if (r->child->nextSib == NULL) {
		//Exp -> INT | FLOAT
		//Exp -> ID is in "examinID"
		if (strcmp(r->child->name, "INT") == 0 || strcmp(r->child->name, "FLOAT") == 0) {
			r->pos = right;
			r->type = r->child->type;
		}
	}

	//Exp -> MINUS Exp | NOT Exp
	else if (r->child->nextSib->nextSib == NULL) {
		r->pos = right;
		if (r->child->nextSib->type == NULL) { //have found error before
			r->type = NULL;
			return;
		}
		Node* arg = r->child->nextSib;
		int res = judgeType(arg->type);
		if (strcmp(r->child->name, "MINUS") == 0) {
			if (res != 0 && res != 1) {
				printf("Error Type 7 at line %d: Expression type does not match operator type. \n", r->lineno);
				r->type = NULL;
				return;
			}
		}
		else if (strcmp(r->child->name, "NOT") == 0){
			if (res != 0) {
				printf("Error Type 7 at line %d: Expression type does not match operator type. \n", r->lineno);
				r->type = NULL;
				return;
			}
		}
		r->type = r->child->nextSib->type;
	}

	//ASSIGNOP, &&, || +, -, *, /, RELOP, (Exp), Exp.ID
	else if (strcmp(r->child->nextSib->name, "ASSIGNOP") == 0) { //Exp -> Exp ASSIGNOP Exp
		r->pos = right;
		Node* arg1 = r->child;
		Node* arg2 = r->child->nextSib->nextSib;
		if (arg1->type == NULL || arg2->type == NULL) {
			r->type = NULL;
			return;
		}
		if (!compType(arg1->type, arg2->type)) {
			printf("Error Type 5 at line %d: Cannot make assignments between different types.\n", r->lineno);
			r->type = NULL;
			return;
		}
		if (arg1->pos == right) {
			printf("Error Type 6 at line %d: Expression in the left of '=' has only right value.\n", r->lineno);
			r->type = NULL;
			return;
		}
		r->type = arg1->type;
	}
	else if (strcmp(r->child->nextSib->name, "DOT") == 0){ //Exp -> Exp DOT ID
		r->pos = left;
		Node* arg1 = r->child;
		Node* arg2 = r->child->nextSib->nextSib;
		int t = judgeType(arg1->type);
		if (t != 3) {
			printf("Error Type 13 at line %d: use operator '.' after a non-struct variable.\n", r->lineno);
			r->type = NULL;
			return;
		}

		//Is the scope arg2 in struct arg1?
		int strI = arg1->type->stridx;
		int res = hash_pjw(arg2->cval);
		HashNode** strNode = structHashTableList[strI]; //type:HashNode**
		int flag = 0;
		HashNode* p = strNode[res];
		while (p != NULL) {
			if (strcmp(p->name, arg2->cval) == 0) {
				flag = 1;
				r->type = p->type;
				arg2->type = p->type; //useful in lab3
				if (debug && debug_sem) { 
					printf("Type in struct:**********\n");
					printType(r->type, 0); 
					printf("T**********\n");
				}
				break;
			}
			else 
				p = p->next;
		}
		if (flag == 0) {
			printf("Error Type 14 at line %d: visited undefined scope '%s' of the struct.\n", r->lineno, arg2->cval);
			r->type = NULL;
			return;
		}
	}
	else if (strcmp(r->child->name, "LP") == 0) {
		r->pos = right;
		if (r->child->nextSib->type == NULL) {
			r->type = NULL;
			return;
		}
		r->type = r->child->nextSib->type;
		if (r->child->nextSib->type->type == record)
			r->stridx = r->child->type->stridx;
	}
	else if (strcmp(r->child->nextSib->name, "AND") == 0 || strcmp(r->child->nextSib->name, "OR") == 0) {
		//only int do logical cal
		r->pos = right;
		Node* arg1 = r->child;
		Node* arg2 = arg1->nextSib->nextSib;
		if (arg1->type == NULL || arg2->type == NULL) {
			r->type = NULL;
			return;
		}
		int res1 = judgeType(arg1->type);
		int res2 = judgeType(arg2->type);
		if (res1 == res2 && res1 == 0) {
			r->type = arg1->type; //always int
		}
		else {
			printf("Error Type 7 at line %d: Expression type does not match operator type. \n", r->lineno);
			r->type = NULL;
			return;
		}
	}
	//Exp -> Exp LB Exp RB
	else if (strcmp(r->child->nextSib->name, "LB") == 0) {
		// get correct type
		r->pos = left;
		Node* arg1 = r->child;
		Node* arg2 = arg1->nextSib->nextSib;
		if (arg1->type == NULL || arg2->type == NULL) {
			r->type = NULL;
			return;
		}
		if (judgeType(arg2->type) != 0) { //Integer
			printf("Error Type 12 at line %d: The array index must be an integer\n", r->lineno);
			r->type = NULL;
			return;
		}
		if (judgeType(arg1->type) != 2) { //Array
			printf("Error Type 10 at line %d: use [] after a non-array variable\n", r->lineno);
			r->type = NULL;
			return;
		}
		r->type = arg1->type->right;
	}
	//>, <, >=, <=, ==, !=, +, -, *, /
	else {
		//array and record cannot do
		r->pos = right;
		Node* arg1 = r->child;
		Node* arg2 = arg1->nextSib->nextSib;
		if (arg1->type == NULL || arg2->type == NULL) {
			r->type = NULL;
			return;
		}
		int res1 = judgeType(arg1->type);
		int res2 = judgeType(arg2->type);
		if (res1 == res2 && (res1 == 0 || res1 == 1)) { //the same integer or float type
			r->type = arg1->type;
		}
		else {
			printf("Error Type 7 at line %d: Expression type does not match operator type. \n", r->lineno);
			r->type = NULL;
			return;
		}
	}
}

int addVar(char* name, TypeNode* type) {
	unsigned int res = hash_pjw(name);
	if (HNListVar[res] == NULL) {
		HashNode *hn = malloc(sizeof(HashNode));
		hn->name = name;
		hn->type = type;
		hn->next = NULL;
		
		//find a definition, add to Operand in list
		hn->ICope.type = VAR;
		hn->ICope.num = varcnt;
		hn->ICope.ctrl = 0;
		if(debug && debug_sem) printf("add ICope v%d to list, origin name is %s\n", hn->ICope.num, name);

		HNListVar[res] = hn;
		if(debug && debug_sem) printf("%s is add to val list\n", name);
		return 0; //no same name
	}
	
	HashNode* p = HNListVar[res];
	if (strcmp(p->name, name) == 0)
		return 1; //find the same name
	while (p->next != NULL) {
		p = p->next;
		if (strcmp(p->name, name) == 0)
			return 1; //find the same name
	}

	// add new var
	HashNode *hn = malloc(sizeof(HashNode));
	hn->name = name;
	hn->type = type;
	hn->next = NULL;

	//find a definition, add to Operand in list
	hn->ICope.type = VAR;
	hn->ICope.num = varcnt;
	hn->ICope.ctrl = 0;
	varcnt++;
	printf("add ICope v%d to list\n", hn->ICope.num);

	p->next = hn;
	if(debug && debug_sem) printf("%s is add to val list\n", name);
	return 0;
}

int addFunc(char* name, TypeNode* type) {
	unsigned int res = hash_pjw(name);
	if (HNListFunc[res] == NULL) {
		HashNode *hn = malloc(sizeof(HashNode));
		hn->name = name;
		hn->type = type;
		hn->next = NULL;
		HNListFunc[res] = hn;
		if(debug && debug_sem) printf("%s is add to func list\n", name);
		return 0; //no same name
	}
	
	HashNode* p = HNListFunc[res];
	if (strcmp(p->name, name) == 0)
		return 1; //find the same name
	while (p->next != NULL) {
		p = p->next;
		if (strcmp(p->name, name) == 0)
			return 1; //find the same name
	}

	// add new var
	HashNode *hn = malloc(sizeof(HashNode));
	hn->name = name;
	hn->type = type;
	hn->next = NULL;
	p->next = hn;
	if(debug && debug_sem) printf("%s is add to func list\n", name);
	return 0;
}

HashNode* findVar(char* name) {
	unsigned int res = hash_pjw(name);
	if(debug && debug_sem) printf("find variable name %s in var list\n", name);
	if (HNListVar[res] == NULL)
		return NULL; //no same name

	HashNode* p = HNListVar[res];
	if (strcmp(p->name, name) == 0)
		return p; //find the same name
	while (p->next != NULL) {
		p = p->next;
		if (strcmp(p->name, name) == 0)
			return p; //find the same name
	}

	return NULL;
}

HashNode* findFunc(char* name) {
	unsigned int res = hash_pjw(name);
	if(debug && debug_sem) printf("find function name %s in func list\n", name);
	if (HNListFunc[res] == NULL)
		return NULL; //no same name
	if(debug && debug_sem) printf("here1\n");

	HashNode* p = HNListFunc[res];
	if (strcmp(p->name, name) == 0)
		return p; //find the same name
	
	while (p->next != NULL) {
		if(debug && debug_sem) printf("here2\n");
		p = p->next;
		if (strcmp(p->name, name) == 0)
			return p; //find the same name
	}

	if(debug && debug_sem) printf("here7\n");
	return NULL;
}

void addRW() {
	addFunc("read", createInteger(0, "INT"));
	TypeNode* argw = createInteger(0, "INT");
	TypeNode* tpw = createInteger(0, "INT");
	tpw->args = argw;
	addFunc("write", tpw);
}

void delStrList() {
	for (int i = 0; i < sindex; i++) {
		free(structHashTableList[i]);
	}
}