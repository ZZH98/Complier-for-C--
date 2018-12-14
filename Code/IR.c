#include "semAnalysis.h"
#include "IR.h"
#include <stdarg.h>
#include <string.h>
#include <assert.h>

int debug_ir = 1;
extern int debug;
extern char strNames[STRUCT_NUM][MAX_VAR_LEN];

HashNode* findVar_noerror(char* strname) {
	if(debug & debug_ir) printf("findVar_noerror: idname = %s\n", strname);
	unsigned int res = hash_pjw(strname);
	HashNode* p = HNListVar[res];
	//assert(p != NULL);
	if (strcmp(p->name, strname) == 0){
		return p;
	}
	while (p->next != NULL) {
		p = p->next;
		if (strcmp(p->name, strname) == 0) {
			return p;
		}
	}
	return NULL;
}

HashNode* findStrVar_noerror(char* idname, int idx) {
	if(debug & debug_ir) printf("findStrVar_noerror: idname = %s, idx = %d\n", idname, idx);
	int res = hash_pjw(idname);
	HashNode** strNode = structHashTableList[idx]; //type:HashNode**
	HashNode* p = strNode[res];
	while (p != NULL) {
		if (strcmp(p->name, idname) == 0) {
			//printf("find p\n");
			return p;
			break;
		}
		else 
			p = p->next;
	}
	return NULL;
}

void insertNode(ICNode* p) {
	if(IChead == NULL) {
		IChead = p;
		ICtail = p;
		p->pre = NULL;
		p->next = NULL;
	}
	else {
		ICtail->next = p;
		p->pre = ICtail;
		p->next = NULL;
		ICtail = ICtail->next;
	}
	return;
}

void insertToHead(ICNode* p) {
	if (IChead == NULL) {
		IChead = p;
		ICtail = p;
		p->pre = NULL;
		p->next = NULL;
	}
	else {
		IChead->pre = p;
		p->pre = NULL;
		p->next = IChead;
		IChead = IChead->pre;
	}
}

void insertNode_mid(ICNode* pos, ICNode* p) { //insert between pos and pos->next
	if (debug && debug_ir) printf("insertNode_mid\n");
	if (pos == NULL) {
		insertToHead(p);
	}
	else if (pos->next == NULL) {
		pos->next = p;
		p->pre = pos;
		p->next = NULL;
		ICtail = p;
	}
	else {
		p->next = pos->next;
		p->pre = pos;
		pos->next->pre = p;
		pos->next = p;
	}
}

Operand newLabel() {
	Operand op;
	op.type = LBL;
	op.num = lblcnt;
	op.ctrl = 0;
	lblcnt++;
	return op;
}

Operand newTemp() {
	Operand op;
	op.type = TMP;
	op.num = tmpcnt;
	op.ctrl = 0;
	tmpcnt++;
	return op;
}

int fillInOffset(Node* r) { //r->child is StructSpecifier
	if (debug & debug_ir) printf("fillInOffset\n");
	Node* opttag = r->child->child->nextSib;
	Node* def = opttag->nextSib->nextSib->child;
	while (def != NULL) {
		//
		fillInSize(def);
		Node* dec = def->child->nextSib->child;

		while (1) {
			Node* vardec = dec->child;
			while(vardec->child != NULL)
				vardec = vardec->child;

			HashNode* p = findStrVar_noerror(vardec->cval, r->child->type->stridx);
			if (p != NULL) p->offset = opttag->type->size;
			opttag->type->size += vardec->type->size; //have been filled in "fillInSize"

			if (dec->nextSib == NULL)
				break;
			else
				dec = dec->nextSib->nextSib->child;
		}

		def = def->nextSib->child;
	}
	return opttag->type->size;
}

void fillInSize(Node* r) { /*Def or ExtDef*/
	//semAnalysis.c does not deal with any issue of size
	int t = judgeType(r->child->type);
	if (debug && debug_ir) {
		printf("fillInSize %s:\n", r->name);
		printType(r->child->type, 0);
	}
	int basicSize = -1;
	if (t == 0 || t == 1) {
		basicSize = 4;
	}

	//set basic size
	else if (t == 3) {
		Node* tag = r->child->child->child->nextSib;
		if (strcmp(tag->name, "OptTag") == 0){ //def of new struct
			basicSize = fillInOffset(r->child/*Specifier*/);
			if (tag->child != NULL) {
				HashNode* p = findVar_noerror(tag->child->cval);
				if (p != NULL) p->type->size = basicSize;
			}
			if (debug && debug_ir) printf("basicSize = struct's totalSize = %d\n", basicSize);
		}
		else { //use of a struct
			char* strname = tag->child->cval;

			HashNode* p = findVar_noerror(strname);
			if (p != NULL) basicSize = p->type->size; //if no sem error p cannot be null, just in case the pro may crash
			if (debug && debug_ir) printf("basicSize = p->type->size = %d\n", basicSize);
		}
		tag->type->size = basicSize;
	}
	if (strcmp(r->child->nextSib->name, "DecList") != 0)
		return;

	//if an array, size of ID is different from size of specifier
	Node * dec = r->child->nextSib->child;
	while (1) {
		Node* vardec = dec->child;
		int arrayItems = 1;
		while (1) {
			vardec = vardec->child;
			if (strcmp(vardec->name, "ID") == 0) {
				int totalSize = basicSize * arrayItems;
				vardec->type->size = totalSize;
				if(debug && debug_ir) printf("give size %d to ID %s\n", totalSize, vardec->cval);
				break;
			}
			else {
				arrayItems *= vardec->nextSib->nextSib->ival;
			}
		}
		if (dec->nextSib == NULL)
			break;
		else
			dec = dec->nextSib->nextSib->child;
	}

	//set array dimensions, useful later
	dec = r->child->nextSib->child;
	while (1) {
		Node* p = dec->child;
		while (strcmp(p->name, "ID") != 0) {
			p = p->child;
		} //now p is ID
		HashNode* res = findVar_noerror(p->cval);

		DimNode* crr = p->type->dims;
		Node* q = dec->child->child;
		while (strcmp(q->name, "ID") != 0) {
			//is an array, first fill in 1
			 
			if (crr == NULL) {
				p->type->dims = malloc(sizeof(DimNode));
				p->type->dims->dim = basicSize;
				p->type->dims->next = NULL;
				crr = p->type->dims;
			}

			int dim = q->nextSib->nextSib->ival;
			DimNode* dn = malloc(sizeof(DimNode));
			dn->dim = dim * crr->dim;
			dn->next = NULL;
			crr->next = dn;
			crr = dn;
			if (debug && debug_ir) printf("dim: %d\n", crr->dim);

			q = q->child;
		}
		if (res != NULL) res->type->dims = p->type->dims;

		if (dec->nextSib == NULL)
			break;
		else
			dec = dec->nextSib->nextSib->child;
	}
}

void fillInParamSize(Node* r) { //ParamDec -> Specifier VarDec
	int t = judgeType(r->child->type);
	if (debug && debug_ir) {
		printf("fillInParamSize %s:\n", r->name);
		printType(r->child->type, 0);
	}
	int basicSize = -1;
	if (t == 0 || t == 1) {
		basicSize = 4;
	}

	//set basic size
	else if (t == 3) {
		Node* tag = r->child->child->child->nextSib;
		if (strcmp(tag->name, "OptTag") == 0){ //def of new struct
			basicSize = fillInOffset(r->child/*Specifier*/);
			if (tag->child != NULL) {
				HashNode* p = findVar_noerror(tag->child->cval);
				if (p != NULL) p->type->size = basicSize;
			}
			if (debug && debug_ir) printf("basicSize = struct's totalSize = %d\n", basicSize);
		}
		else { //use of a struct
			char* strname = tag->child->cval;

			HashNode* p = findVar_noerror(strname);
			if (p != NULL) basicSize = p->type->size; //if no sem error p cannot be null, just in case the pro may crash
			if (debug && debug_ir) printf("basicSize = p->type->size = %d\n", basicSize);
		}
		tag->type->size = basicSize;
	}

	Node* vardec = r->child->nextSib;
	int arrayItems = 1;
	while (1) {
		vardec = vardec->child;
		if (strcmp(vardec->name, "ID") == 0) {
			int totalSize = basicSize * arrayItems;
			vardec->type->size = totalSize;
			if(debug && debug_ir) printf("give size %d to ID %s\n", totalSize, vardec->cval);
			break;
		}
		else {
			arrayItems *= vardec->nextSib->nextSib->ival;
		}
	}

	Node* p = r->child->nextSib;
	Node* q = p->child;
	while (strcmp(p->name, "ID") != 0) {
		p = p->child;
	} //now p is ID
	HashNode* res = findVar_noerror(p->cval);

	DimNode* crr = p->type->dims;
	while (strcmp(q->name, "ID") != 0) {
		//is an array, first fill in 1
		 
		if (crr == NULL) {
			p->type->dims = malloc(sizeof(DimNode));
			p->type->dims->dim = basicSize;
			p->type->dims->next = NULL;
			crr = p->type->dims;
		}

		int dim = q->nextSib->nextSib->ival;
		DimNode* dn = malloc(sizeof(DimNode));
		dn->dim = dim * crr->dim;
		dn->next = NULL;
		crr->next = dn;
		crr = dn;
		if (debug && debug_ir) printf("dim: %d\n", crr->dim);

		q = q->child;
	}
	if (res != NULL) res->type->dims = p->type->dims; //into table
}

void putLabel(Operand op) {
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 1;
	ic->type1.op = LABEL;
	ic->type1.arg = op;
	insertNode(ic);
	return;
}

void putGoto(Operand op) {
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 1;
	ic->type1.op = GOTO;
	ic->type1.arg = op;
	insertNode(ic);
	return;
}

void putDec(Operand op, int size) {
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 1;
	ic->type1.op = DEC;
	ic->type1.arg = op;
	ic->type1.decsize = size;
	insertNode(ic);
	return;
}

void putAssign(Operand op1, Operand op2) {
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 2;
	ic->type2.op = ASSIGN;
	ic->type2.arg1 = op1;
	ic->type2.arg2 = op2;
	insertNode(ic);
	return;
}

void putArgs(Node* r) {
	if (debug && debug_ir) printf("putArgs, start!\n");
	Node* arg_0 = r;
	Node* arg = r;
	while (1) {
		translate_Exp(arg_0);

		if (arg_0->nextSib == NULL)
			break;
		else
			arg_0 = arg_0->nextSib->nextSib->child;
	}

	ICNode* tail2 = ICtail;
	while (1) {
		int res = judgeType(arg->type);
		Operand push = arg->ICope;

		if (res == 2 || res == 3) {
			if (push.ctrl == 2 || push.ctrl == 1)
				push.ctrl = 0;
			else
				push.ctrl = 1;
		}
		ICNode* para = malloc(sizeof(ICNode));
		para->type = 1;
		para->type1.op = ARG;
		para->type1.arg = push;
		insertNode_mid(tail2, para);

		if (arg->nextSib == NULL)
			break;
		else
			arg = arg->nextSib->nextSib->child;
	}
}

int judgeNum(Operand *op) {
	if (op->type == NUM && op->num == 0)
		return 0;
	else if (op->type == NUM && op->num == 1)
		return 1;
	return -1;
}

Operand putPMSD(Operand op1, Operand op2, Operand op3, int caltype) {
	if (op2.type == NUM && op3.type == NUM) {
		op1.type = NUM;
		switch(caltype) {
		case 0: op1.num = op2.num + op3.num; break;
		case 1: op1.num = op2.num - op3.num; break;
		case 2: op1.num = op2.num * op3.num; break;
		case 3: op1.num = op2.num / op3.num; break;
		}
		return op1;
	}
	else {
		if (caltype == 0 && judgeNum(&op2) == 0) 
			return op3;
		else if (caltype == 0 && judgeNum(&op3) == 0)
			return op2;
		else if (caltype == 1 && judgeNum(&op3) == 0)
			return op2;
		else if (caltype == 2 && judgeNum(&op2) == 1)
			return op3;
		else if (caltype == 2 && judgeNum(&op3) == 1)
			return op2;
		else if (caltype == 3 && judgeNum(&op2) == 0)
			return op2;
		else if (caltype == 3 && judgeNum(&op3) == 1)
			return op2;
	}
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 3;
	ic->type3.op = caltype;
	ic->type3.arg1 = op1;
	ic->type3.arg2 = op2;
	ic->type3.arg3 = op3;
	insertNode(ic);
	return op1;
}

void translate_Def(Node* r/*Def*/) {
	fillInSize(r);
	// struct?
	if (strcmp(r->child->child->name, "StructSpecifier") == 0) {
		//size has been stored in ID
		Node* str = r->child->child; //StructSpecifier
		Node* strtag = str->child/*STRUCT*/->nextSib/*Tag or OptTag*/;
		Node* strid = strtag->child;

		Node* dec = r->child->nextSib->child; //Dec
		while (1) {
			Node* vardec = dec->child;
			while (strcmp(vardec->child->name, "ID") != 0)
				vardec = vardec->child;
			putDec(vardec->child->ICope, vardec->child->type->size);
			if (dec->nextSib == NULL)
				break;
			else
				dec = dec->nextSib->nextSib->child;
		}
	}
	//array?
	else {
		Node* dec = r->child->nextSib->child; //Dec
		while(1) {
			Node* vardec = dec->child;
			int flag = 0;
			while (strcmp(vardec->child->name, "ID") != 0) {
				flag = 1;
				vardec = vardec->child;
			}

			if (dec->child->nextSib != NULL) {
				//ASSIGNOP?
				Node* arg2 = dec->child->nextSib->nextSib;
				translate_Exp(arg2);
				putAssign(vardec->child->ICope, arg2->ICope);
			}
			else if (flag == 1) {
				//array?
				putDec(vardec->child->ICope, vardec->child->type->size);
			}
			if (dec->nextSib == NULL)
				break;
			dec = dec->nextSib->nextSib->child;
		}
	}
}

void translate_Stmt(Node* r/*Stmt*/) { // translate one stmt one time
	if (strcmp(r->child->name, "Exp") == 0) {
		translate_Exp(r->child);
	}
	else if (strcmp(r->child->name, "CompSt") == 0) {
		Node* def = r->child/*CompSt*/->child/*LC*/->nextSib/*DefList*/;
		Node* stmt = def->nextSib/*StmtList*/->child/*Stmt or eps*/;
		def = def->child;
		while (def != NULL) {
			translate_Def(def);
			def = def->nextSib->child;
		}
		while (stmt != NULL) {
			translate_Stmt(stmt);
			stmt = stmt->nextSib->child;
		}
	}
	else if (strcmp(r->child->name, "RETURN") == 0) {
		Node* exp = r->child->nextSib;
		translate_Exp(exp); //will fill in the ICope of Node Exp
		ICNode* ic = malloc(sizeof(ICNode));
		ic->type = 1;
		ic->type1.op = RETURN;
		ic->type1.arg = exp->ICope; //take it out
		insertNode(ic);
	}
	else if (strcmp(r->child->name, "WHILE") == 0) {
		// while loop
		Operand lb1 = newLabel();
		Operand lb2 = newLabel();
		Operand lb3 = newLabel();

		putLabel(lb1);

		Node* cond = r/*Stmt*/->child/*WHILE*/->nextSib/*LP*/->nextSib/*Exp*/;
		translate_Cond(cond, lb2, lb3);

		Node* stmt = cond->nextSib/*RP*/->nextSib/*Stmt*/;
		putLabel(lb2);
		translate_Stmt(stmt);

		putGoto(lb1);
		putLabel(lb3);
		
	}
	else {
		if (r->child/*IF*/->nextSib/*LP*/->nextSib/*Exp*/->nextSib/*RP*/->nextSib/*Stmt*/->nextSib == NULL) {
			// no else
			Operand lb1 = newLabel();
			Operand lb2 = newLabel();

			Node* cond = r/*Stmt*/->child/*IF*/->nextSib/*LP*/->nextSib/*Exp*/;
			translate_Cond(cond, lb1, lb2);
			putLabel(lb1);

			Node* stmt = cond->nextSib/*RP*/->nextSib/*Stmt*/;
			translate_Stmt(stmt);

			putLabel(lb2);
		}
		else {
			//with else
			Operand lb1 = newLabel();
			Operand lb2 = newLabel();
			Operand lb3 = newLabel();

			Node* cond = r/*Stmt*/->child/*IF*/->nextSib/*LP*/->nextSib/*Exp*/;
			translate_Cond(cond, lb1, lb2);
			putLabel(lb1);

			Node* stmt1 = cond->nextSib/*RP*/->nextSib/*Stmt*/;
			translate_Stmt(stmt1);
			putGoto(lb3);
			putLabel(lb2);

			Node* stmt2 = stmt1->nextSib/*ELSE*/->nextSib/*Stmt*/;
			translate_Stmt(stmt2);
			putLabel(lb3);
		}
	}
	return;
}

void translate_Exp2Cond(Node* r) {
	if (debug && debug_ir) printf("mid station\n");
	Operand lb1 = newLabel(); //true
	Operand lb2 = newLabel(); //false
	Operand res = newTemp();
	Operand ans = {NUM, 0, 0};
	putAssign(res, ans);
	translate_Cond(r, lb1, lb2);
	putLabel(lb1);
	ans.num = 1;
	putAssign(res, ans);
	putLabel(lb2);
	r->ICope = res;
}

void translate_Cond(Node* r, Operand lb1, Operand lb2) { //lb1: true lb2:false
	//Exp -> NOT Exp
	if (strcmp(r->child->name, "NOT") == 0) {
		translate_Cond(r->child->nextSib, lb2, lb1);
	}
	//Exp -> Exp && Exp
	else if (r->child->nextSib != NULL && strcmp(r->child->nextSib->name, "AND") == 0) {
		Operand lb3 = newLabel(); //mid lbl
		translate_Cond(r->child, lb3, lb2);
		putLabel(lb3);
		translate_Cond(r->child->nextSib->nextSib, lb1, lb2);
	}
	//Exp -> Exp || Exp
	else if (r->child->nextSib != NULL && strcmp(r->child->nextSib->name, "OR") == 0) {
		Operand lb3 = newLabel(); //mid lbl
		translate_Cond(r->child, lb1, lb3);
		putLabel(lb3);
		translate_Cond(r->child->nextSib->nextSib, lb1, lb2);
	}
	//Exp -> Exp RELOP Exp
	else if (r->child->nextSib != NULL && strcmp(r->child->nextSib->name, "RELOPG") == 0 || strcmp(r->child->nextSib->name, "RELOPL") == 0 || strcmp(r->child->nextSib->name, "RELOPGE") == 0 || strcmp(r->child->nextSib->name, "RELOPLE") == 0 || strcmp(r->child->nextSib->name, "RELOPE") == 0 || strcmp(r->child->nextSib->name, "RELOPNE") == 0) {
		Node* e1 = r->child;
		Node* e2 = r->child->nextSib->nextSib;		
		translate_Exp(e1);
		translate_Exp(e2);
		ICNode* ifcase = malloc(sizeof(ICNode));
		ifcase->type = 3;
		ifcase->type3.op = IF;
		ifcase->type3.arg1 = e1->ICope;
		ifcase->type3.arg2 = e2->ICope;
		ifcase->type3.arg3 = lb1;
		int op;
		if (strcmp(r->child->nextSib->name, "RELOPG") == 0)
			op = 0;
		else if (strcmp(r->child->nextSib->name, "RELOPL") == 0)
			op = 1;
		else if (strcmp(r->child->nextSib->name, "RELOPGE") == 0)
			op = 2;
		else if (strcmp(r->child->nextSib->name, "RELOPLE") == 0)
			op = 3;
		else if (strcmp(r->child->nextSib->name, "RELOPE") == 0)
			op = 4;
		else if (strcmp(r->child->nextSib->name, "RELOPNE") == 0)
			op = 5;
		ifcase->type3.relop = op;
		insertNode(ifcase);
		putGoto(lb2);
	}
	else {
		if (debug && debug_ir) printf("Cond: last case, got exp\n");
		translate_Exp(r);
		ICNode* ifcase = malloc(sizeof(ICNode));
		ifcase->type = 3;
		ifcase->type3.op = IF;
		ifcase->type3.arg1 = r->ICope;
		Operand zero = {NUM, 0, 0};
		ifcase->type3.arg2 = zero;
		ifcase->type3.arg3 = lb1;	
		ifcase->type3.relop = 5; //RELOPNE	
		insertNode(ifcase);
		putGoto(lb2);
	}
}

void translate_Exp(Node* r/*Exp*/) {
	//Exp -> ID | INT
	if (r->child->nextSib == NULL) {
		if (strcmp(r->child->name, "ID") == 0) {
			HashNode* res = findVar_noerror(r->child->cval);
			r->child->ICope = res->ICope;
			if (debug && debug_ir) printf("get %s's ICope, ctrl = %d\n", r->child->cval, r->child->ICope.ctrl);
		}
		r->ICope = r->child->ICope;
	}
	//Exp -> MINUS Exp | Exp -> NOT Exp
	else if (r->child->nextSib->nextSib == NULL) {
		if (strcmp(r->child->name, "MINUS") == 0) {
			// res = #0 - arg
			translate_Exp(r->child->nextSib);

			Operand temp = newTemp();
			/*if (r->child->nextSib->ICope.type == NUM) {
				temp.type = NUM;
				temp.num = -1 * r->child->nextSib->ICope.num;
			}
			else {*/
				Operand zero = {NUM, 0, 0};
				temp = putPMSD(temp, zero, r->child->nextSib->ICope, 1);
			//}

			r->ICope = temp; //Important!
		}
		else {
			translate_Exp2Cond(r);
		}
	}
	//ASSIGNOP
	else if (strcmp(r->child->nextSib->name, "ASSIGNOP") == 0) { //Exp -> Exp ASSIGNOP Exp
		Node* arg1 = r->child;
		Node* arg2 = r->child->nextSib->nextSib;
		translate_Exp(arg1);
		translate_Exp(arg2);
		if (arg1->ICope.type == arg2->ICope.type && arg1->ICope.num == arg2->ICope.num && arg1->ICope.ctrl == arg2->ICope.ctrl) {
			r->ICope = arg1->ICope;
			return;
		}

		putAssign(arg1->ICope, arg2->ICope);
		/*r->ICope = newTemp();
		putAssign(r->ICope, arg1->ICope); // x = y = z?*/
		r->ICope = arg1->ICope;
		
	}
	//Exp -> Exp DOT ID
	else if (strcmp(r->child->nextSib->name, "DOT") == 0){ 
		// find ID in the table
		Node* arg1 = r->child;
		Node* arg2 = r->child->nextSib->nextSib;
		translate_Exp(arg1);
		HashNode* res = findStrVar_noerror(arg2->cval, arg1->type->stridx);
		//assert(res != NULL);
		int offset = res->offset;

		Operand op1 = arg1->ICope;
		if (op1.ctrl == 2 || op1.ctrl == 1) //xx.yy.zz, xx.yy has been an address, so here add it directly
			op1.ctrl = 0;
		else
			op1.ctrl = 1;

		Operand op2 = {NUM, offset, 0};
		Operand temp = newTemp();
		temp = putPMSD(temp, op1, op2, 0); 
		temp.ctrl = 2; //temp is a address, no meaning. occured later it will be something like *t1
		r->ICope = temp;
	}
	//Exp -> ( Exp )
	else if (strcmp(r->child->name, "LP") == 0) {
		Node* arg = r->child->nextSib;
		translate_Exp(arg);
		r->ICope = arg->ICope;
	}
	//Exp -> Exp LB Exp RB
	else if (strcmp(r->child->nextSib->name, "LB") == 0) {
		// use dimensions that have been stored before
		Node* id = r->child;
		Node* bottom;
		//calculate dims of the exp, may less than def
		int expCNT = 1;
		while (1) {
			if (strcmp(id->child->name, "ID") == 0) {
				translate_Exp(id);
				bottom = id;
				id = id->child;
				break;
			}
			else if (id->child->nextSib != NULL && strcmp(id->child->nextSib->name, "DOT") == 0) {
				translate_Exp(id);
				bottom = id;
				id = id->child->nextSib->nextSib;
				break;
			}
			id = id->child;
			expCNT++;
		}

		HashNode* res = findVar_noerror(id->cval);

		if (res != NULL) {
			id->type->dims = res->type->dims;
		}
		Node* idx = r->child->nextSib->nextSib;
		translate_Exp(idx);

		//calculate dims of the def
		DimNode* tempD = id->type->dims;
		int defCNT = 0;
		while (tempD->next != NULL) {
			tempD = tempD->next;
			defCNT++;
		}

		int gap = defCNT - expCNT;
		DimNode* dims = id->type->dims;
		while (gap) {
			dims = dims->next;
			gap--;
		}
		Operand o_offset = newTemp();
		Operand offset_temp = newTemp();
		Operand off = idx->ICope;

		Operand f_mul = {NUM, dims->dim, 0};
		offset_temp = putPMSD(offset_temp, idx->ICope, f_mul, 2);
		dims = dims->next;

		Operand arr = bottom->ICope;
		if (arr.ctrl == 2 || arr.ctrl == 1) //xx.yy.zz, xx.yy has been an address, so here add it directly
			arr.ctrl = 0;
		else
			arr.ctrl = 1;
		o_offset = putPMSD(o_offset, arr, offset_temp, 0);
		if (judgeNum(&offset_temp) == 0) {
			o_offset = newTemp();
			putAssign(o_offset, arr);
		}

		Node* iter = r->child->child;
		while (1) {
			if (strcmp(iter->name, "ID") == 0) {
				break;
			}
			else if (iter->nextSib != NULL && strcmp(iter->nextSib->name, "DOT") == 0) {
				break;
			}
			Operand offset_l = newTemp();
			Node* idx_l = iter->nextSib->nextSib;
			translate_Exp(idx_l);
			Operand mul = {NUM, dims->dim, 0};
			offset_l = putPMSD(offset_l, idx_l->ICope, mul, 2);
			o_offset = putPMSD(o_offset, o_offset, offset_l, 0);
			iter = iter->child;
			dims = dims->next;
		}
		o_offset.ctrl = 2;
		r->ICope = o_offset;
	}
	//Exp -> ID LP RP | Exp -> ID LP Args RP
	else if (strcmp(r->child->name, "ID") == 0) {
		if (strcmp(r->child->cval, "read") == 0) {
			// READ t4
			Operand toread = newTemp();
			ICNode* read = malloc(sizeof(ICNode));
			read->type = 1;
			read->type1.op = READ;
			read->type1.arg = toread;
			insertNode(read);
			r->ICope = toread;
		}
		else if (strcmp(r->child->cval, "write") == 0) {
			Node* arg = r->child->nextSib->nextSib->child; //Exp
			translate_Exp(arg);
			Operand towrite = arg->ICope;
			int res = judgeType(arg->type);
			if (res == 2 || res == 3) {
				if (towrite.ctrl == 2)
					towrite.ctrl = 0;
				else
					towrite.ctrl = 1;
			}

			ICNode* write = malloc(sizeof(ICNode));
			write->type = 1;
			write->type1.op = WRITE;
			write->type1.arg = towrite;
			insertNode(write);
			Operand ret = {NUM, 0, 0};
			r->ICope = ret;
		}
		else {
			if (r->child->nextSib->nextSib->nextSib != NULL) {
				// have arguments
				Node* arg = r->child->nextSib->nextSib->child; //Exp
				putArgs(arg);
			}
			Operand ret = newTemp();

			Operand func;
			func.type = FUN;
			func.name = r->child->cval;
			func.ctrl = 0;

			ICNode* callfunc = malloc(sizeof(ICNode));
			callfunc->type = 2;
			callfunc->type2.op = FUNCTION;
			callfunc->type2.arg1 = ret;
			callfunc->type2.arg2 = func;
			insertNode(callfunc);

			r->ICope = ret;
		}
	}
	//Exp -> Exp PLUS Exp
	else if (strcmp(r->child->nextSib->name, "PLUS") == 0){
		translate_Exp(r->child);
		translate_Exp(r->child->nextSib->nextSib);
		Operand res = newTemp();
		res = putPMSD(res, r->child->ICope, r->child->nextSib->nextSib->ICope, 0);
		r->ICope = res;
		if (debug && debug_ir) printf("gen PLUS in line %d\n", r->lineno);
	}
	//Exp -> Exp MINUS Exp
	else if (strcmp(r->child->nextSib->name, "MINUS") == 0){
		translate_Exp(r->child);
		translate_Exp(r->child->nextSib->nextSib);
		Operand res = newTemp();
		res = putPMSD(res, r->child->ICope, r->child->nextSib->nextSib->ICope, 1);
		r->ICope = res;
		if (debug && debug_ir) printf("gen MINUS in line %d\n", r->lineno);
	}
	//Exp -> Exp STAR Exp
	else if (strcmp(r->child->nextSib->name, "STAR") == 0){
		translate_Exp(r->child);
		translate_Exp(r->child->nextSib->nextSib);
		Operand res = newTemp();
		res = putPMSD(res, r->child->ICope, r->child->nextSib->nextSib->ICope, 2);
		r->ICope = res;
		if (debug && debug_ir) printf("gen STAR in line %d\n", r->lineno);
	}
	//Exp -> Exp DIV Exp
	else if (strcmp(r->child->nextSib->name, "DIV") == 0){
		translate_Exp(r->child);
		translate_Exp(r->child->nextSib->nextSib);
		Operand res = newTemp();
		res = putPMSD(res, r->child->ICope, r->child->nextSib->nextSib->ICope, 3);
		r->ICope = res;
		if (debug && debug_ir) printf("gen DIV in line %d\n", r->lineno);
	}
	//Exp -> Exp AND/OR/RELOP Exp
	else {
		if (debug && debug_ir) printf("Exp: last case, goto Cond\n");
		translate_Exp2Cond(r);
	}
}

void translate_FunDec(Node* r) {
	ICNode* ic = malloc(sizeof(ICNode));
	ic->type = 1;
	ic->type1.op = FUNCTION;
	ic->type1.arg.type = FUN;
	ic->type1.arg.name = r->child->cval;
	insertNode(ic);

	if (r->child/*ID*/->nextSib/*LP*/->nextSib->nextSib == NULL)
		return;
	Node* arg = r->child/*ID*/->nextSib/*LP*/->nextSib/*VarList*/->child/*ParamDec*/;
	while (1) {
		Node* vardec = arg->child->nextSib;
		while (strcmp(vardec->child->name, "ID") != 0)
			vardec = vardec->child;
		ICNode* ic = malloc(sizeof(ICNode));
		ic->type = 1;
		ic->type1.op = PARAM;
		ic->type1.arg = vardec->child->ICope;
		insertNode(ic);
		Node* id = vardec->child;
		int res = judgeType(id->type);
		if (res == 2 || res == 3) {
			if (debug && debug_ir) printf("para %s: type is addr\n", id->cval);
			id->ICope.ctrl = 1; // convey addr of struct/array into a function, so the type of para is addr
			HashNode* list = findVar_noerror(id->cval);
			list->ICope.ctrl = 1;
		}

		fillInParamSize(arg);

		if (arg->nextSib == NULL)
			break;
		else
			arg = arg->nextSib->nextSib->child;
	}
}

void genIR(Node* r/*Program*/) {

	Node* extdef = r->child->child;
	while(extdef != NULL) {
		if (strcmp(extdef->child->nextSib->name, "FunDec") == 0) {
			Node* func = extdef->child->nextSib; //FunDec
			if (debug && debug_ir) printf("Enter function --------------\n");
			translate_FunDec(func);
			Node* def = func->nextSib/*Compst*/->child/*LC*/->nextSib/*DefList*/;
			Node* stmt = def->nextSib/*StmtList*/->child/*Stmt or eps*/;
			def = def->child/*Def*/;
			while (def != NULL) {
				translate_Def(def);
				def = def->nextSib->child;
			}

			while (stmt != NULL) {
				translate_Stmt(stmt);
				stmt = stmt->nextSib->child;
			}
		}
		else {
			fillInSize(extdef); //only ExtDef -> Specifier SEMI will exist
		}

		extdef = extdef->nextSib->child;
	}
}

void printOPE(FILE* outfile, Operand* ope) {
	if (ope->ctrl == 1) {
		fprintf(outfile, "&");
	}
	else if (ope->ctrl == 2)
		fprintf(outfile, "*");
	switch(ope->type) {
	case 0: fprintf(outfile, "label%d", ope->num); break; //LBL
	case 1: fprintf(outfile, "v%d", ope->num); break; //VAR
	case 2: fprintf(outfile, "t%d", ope->num); break; //TMP
	case 3: fprintf(outfile, "%s", ope->name); break; //FUN
	case 4: fprintf(outfile, "#%d", ope->num); break; //NUM(only int)
	default: break;
	}
}

void printIR(FILE* outfile) {
	ICNode* p = IChead;
	while (p != NULL) {
		if (p->type == 1) {
			switch (p->type1.op) {
			case 0: fprintf(outfile, "LABEL "); break;
			case 1: fprintf(outfile, "FUNCTION "); break;
			case 2: fprintf(outfile, "GOTO "); break;
			case 3: fprintf(outfile, "RETURN "); break;
			case 4: fprintf(outfile, "DEC "); break;
			case 5: fprintf(outfile, "ARG "); break;
			case 6: fprintf(outfile, "PARAM "); break;
			case 7: fprintf(outfile, "READ "); break;
			case 8: fprintf(outfile, "WRITE "); break; 
			default: break;
			}
			printOPE(outfile, &(p->type1.arg));
			if (p->type1.op == DEC)
				fprintf(outfile, " %d", p->type1.decsize);
			if (p->type1.op == LABEL || p->type1.op == FUNCTION)
				fprintf(outfile, " :\n");
			else
				fprintf(outfile, "\n");
		}
		else if (p->type == 2) {
			printOPE(outfile, &(p->type2.arg1));
			switch (p->type2.op) {
			case 0: fprintf(outfile, " := "); break;
			case 1: fprintf(outfile, " := CALL "); break;
			default: break;
			}
			printOPE(outfile, &(p->type2.arg2));
			fprintf(outfile, "\n");
		}
		else {
			if (p->type3.op == IF) {
				fprintf(outfile, "IF ");
				printOPE(outfile, &(p->type3.arg1));
				switch (p->type3.relop) {
				case 0: fprintf(outfile, " > "); break;
				case 1: fprintf(outfile, " < "); break;
				case 2: fprintf(outfile, " >= "); break;
				case 3: fprintf(outfile, " <= "); break;
				case 4: fprintf(outfile, " == "); break;
				case 5: fprintf(outfile, " != "); break;
				default: break;
				}
				printOPE(outfile, &(p->type3.arg2));
				fprintf(outfile, " GOTO ");
				printOPE(outfile, &(p->type3.arg3));
				fprintf(outfile, "\n");
				p = p->next;
				continue;
			}
			printOPE(outfile, &(p->type3.arg1));
			fprintf(outfile, " := ");
			printOPE(outfile, &(p->type3.arg2));
			switch(p->type3.op) {
			case 0: fprintf(outfile, " + "); break;
			case 1: fprintf(outfile, " - "); break;
			case 2: fprintf(outfile, " * "); break;
			case 3: fprintf(outfile, " / "); break;
			}
			printOPE(outfile, &(p->type3.arg3));
			fprintf(outfile, "\n");
		}
		p = p->next;
	}
}

void delIR() {
	ICNode* p = IChead;
	while(p != NULL) {
		ICNode* t = p;
		p = p->next;
		free(t);
	}
}