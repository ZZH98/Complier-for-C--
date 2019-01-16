#include "semAnalysis.h"
#include "IR.h"
#include "ASM.h"
#include <string.h>
#include <assert.h>
#include <time.h>

extern ICNode* IChead;
extern ICNode* ICtail;
extern int varcnt;
extern int tmpcnt;

extern int debug;
int debug_asm = 0;

struct BlockPointer {
	ICNode* pointer;
	struct BlockPointer* next;
};
typedef struct BlockPointer BlockPointer;
BlockPointer* BPhead;

void parseBlock() {
	BPhead = malloc(sizeof(BlockPointer));
	BPhead->pointer = IChead;
	BPhead->next = NULL;
	BlockPointer* BPtail = BPhead;
	/*for (ICNode* p = IChead->next; p != NULL; p = p->next) {
		// IF xxx GOTO xxx
		//GOTO xxx
		//RETURN xxx
		if ((p->type == 3 && p->type3.op == IF) || (p->type == 1 && (p->type1.op == GOTO || p->type1.op == RETURN))) {
			BlockPointer* temp = malloc(sizeof(BlockPointer));
			temp->pointer = p->next;
			temp->next = NULL;
			BPtail->next = temp;
			BPtail = temp;
		}
		//LABEL xxx
		//FUNCTION xxx
		if (p->type == 1 && (p->type1.op == LABEL || p->type1.op == FUNCTION)) {
			BlockPointer* temp = malloc(sizeof(BlockPointer));
			temp->pointer = p;
			temp->next = NULL;
			BPtail->next = temp;
			BPtail = temp;
		}
	}*/
	for (ICNode* p = IChead->next; p != NULL; p = p->next) {
		//FUNCTION xxx
		if (p->type == 1 && p->type1.op == FUNCTION) {
			BlockPointer* temp = malloc(sizeof(BlockPointer));
			temp->pointer = p;
			temp->next = NULL;
			BPtail->next = temp;
			BPtail = temp;
		}
	}
	return;
}

void putPre(FILE* outfile) {
	fprintf(outfile, ".data\n");
	fprintf(outfile, "_prompt: .asciiz \"Enter an integer:\"\n");
	fprintf(outfile, "_ret: .asciiz \"\\n\"\n");
	fprintf(outfile, ".globl main\n");
	fprintf(outfile, ".text\n");
	fprintf(outfile, "read:\n");
	fprintf(outfile, "li $v0, 4\n");
	fprintf(outfile, "la $a0, _prompt\n");
	fprintf(outfile, "syscall\n");
	fprintf(outfile, "li $v0, 5\n");
	fprintf(outfile, "syscall\n");
	fprintf(outfile, "jr $ra\n\n");
	fprintf(outfile, "write:\n");
	fprintf(outfile, "li $v0, 1\n");
	fprintf(outfile, "syscall\n");
	fprintf(outfile, "li $v0, 4\n");
	fprintf(outfile, "la $a0, _ret\n");
	fprintf(outfile, "syscall\n");
	fprintf(outfile, "move $v0, $0\n");
	fprintf(outfile, "jr $ra\n\n");
}

void putLW(int reg, int offset, FILE* outfile) {
	fprintf(outfile, "lw $%d, %d($sp)\n", reg, offset);
}

void putSW(int reg, int offset, FILE* outfile) {
	fprintf(outfile, "sw $%d, %d($sp)\n", reg, offset);
}

RegDesc* allReg;
VarDesc* allVar;
int spillcnt = 5;

int gegRegLeft(Operand x, FILE* outfile) {
	//
}

int getReg1(Operand x, FILE* outfile) {
	// &
	int idx = (x.type == TMP) ? x.num + varcnt : x.num;
	int reg = allVar[idx].reg;
	if (reg != -1) {
		putSW(reg, allVar[idx].offset, outfile);
		allReg[reg].filled = 0;
	}
	fprintf(outfile, "addi $s8, $sp, %d\n", allVar[idx].offset);
	return 30;
}

int getReg2(Operand x, FILE* outfile) {
	// *
	int idx = (x.type == TMP) ? x.num + varcnt : x.num;
	int reg = allVar[idx].reg;
	if (reg != -1) {
		putSW(reg, allVar[idx].offset, outfile);
		allReg[reg].filled = 0;
	}
	fprintf(outfile, "lw $t9, %d($sp)\n", allVar[idx].offset);
	fprintf(outfile, "lw $t9, 0($t9)\n");
	return 25;
}

int getReg(Operand x, FILE* outfile, int pos /*1: left 2:right*/) {
	if (debug && debug_asm) {
		printOPE(stdout, &x);
		printf(", getReg: \n");
	}
	if (x.ctrl == 1 && pos == 2)
		return getReg1(x, outfile);
	else if (x.ctrl == 2 && pos == 2) 
		return getReg2(x, outfile);

	int beg, end, idx;
	if (x.type == VAR) {
		beg = 16; end = 23; // $s0 ~ $s7
		idx = x.num;
	}
	else if (x.type == TMP)  {
		beg = 8; end = 15; // $t0 ~ $t7
		idx = x.num + varcnt;
	}
	else if (x.type == NUM) {
		fprintf(outfile, "li $t8, %d\n", x.num);
		return 24; //$t8
	}
	else {
		if (debug && debug_asm) printf("error in getReg\n");
		return -1;
	}
	if (allVar[idx].reg != -1) return allVar[idx].reg;

	for (int i = beg; i <= end; i++) {
		if (allReg[i].filled == 0) { // put x in reg $i
			allVar[idx].reg = i;
			allReg[i].varidx = idx;
			allReg[i].filled = 1;
			if (pos == 2 || (pos == 1 && x.ctrl == 2))
				putLW(i, allVar[idx].offset, outfile);
			if (debug && debug_asm) printf("find an empty reg %d, and x's offset is %d\n", i, allVar[idx].offset);

			return i;
		}
	}

	// spilling
	if (debug && debug_asm) printf("begin spilling\n");
	srand((int)time(0)); 
	int remove = rand() % 8 + beg;

	int oldidx = allReg[remove].varidx;
	putSW(remove, allVar[oldidx].offset, outfile);
	allVar[oldidx].reg = -1;
	if (pos == 2 || (pos == 1 && x.ctrl == 2))
		putLW(remove, allVar[idx].offset, outfile);
	allVar[idx].reg = remove;
	allReg[remove].varidx = idx;
	return remove;
}

void IR2ASM(ICNode* p, FILE* outfile) {
	if (p->type == 1) {
		switch (p->type1.op) {
		case LABEL: 
			printOPE(outfile, &p->type1.arg); 
			fprintf(outfile, ":\n"); 
			break;
		// FUNCTION: in main
		case GOTO: 
			fprintf(outfile, "j "); 
			printOPE(outfile, &p->type1.arg); 
			fprintf(outfile, "\n"); 
			break;
		// RETURN: in main
		// DEC: in main
		// ARG: in main, together with FUNCTION
		// PARAM: in main
		case READ: 
			fprintf(outfile, "addi $sp, $sp, -4\n");
			fprintf(outfile, "sw $ra, 0($sp)\n");
			fprintf(outfile, "jal read\n");
			fprintf(outfile, "lw $ra, 0($sp)\n");
			fprintf(outfile, "addi $sp, $sp, 4\n");
			Operand temp_op = p->type1.arg;
			int temp_idx = (temp_op.type == TMP) ? temp_op.num + varcnt : temp_op.num;
			fprintf(outfile, "sw $v0, %d($sp)\n", allVar[temp_idx].offset);
			break;
		case WRITE: {
			fprintf(outfile, "addi $sp, $sp, -4\n");
			fprintf(outfile, "sw $a0, 0($sp)\n");

			Operand temp_op = p->type1.arg;
			int temp_idx = (temp_op.type == TMP) ? temp_op.num + varcnt : temp_op.num;
			if (allVar[temp_idx].reg != -1)
				fprintf(outfile, "move $a0, $%d\n", allVar[temp_idx].reg);
			else
				fprintf(outfile, "lw $a0, %d($sp)\n", 4 + allVar[temp_idx].offset);
			if (temp_op.ctrl == 2)
				fprintf(outfile, "lw $a0, 0($a0)\n");

			fprintf(outfile, "addi $sp, $sp, -4\n");
			fprintf(outfile, "sw $ra, 0($sp)\n");

			fprintf(outfile, "jal write\n");

			fprintf(outfile, "lw $ra, 0($sp)\n");
			fprintf(outfile, "addi $sp, $sp, 4\n");

			fprintf(outfile, "lw $a0, 0($sp)\n");
			fprintf(outfile, "addi $sp, $sp, 4\n");
			break;
		}
		default: break;
		}
	}
	else if (p->type == 2) {
		switch (p->type2.op) {
		case ASSIGN: {
			int reg1 = getReg(p->type2.arg1, outfile, 1);
			int reg2 = getReg(p->type2.arg2, outfile, 2);
			if (p->type2.arg1.ctrl == 0)
				fprintf(outfile, "move $%d, $%d\n", reg1, reg2);
			else
				fprintf(outfile, "sw $%d, 0($%d)\n", reg2, reg1);
			break;
		}
		// CALL: in main. together with ARG
		default: break;
		}
	}
	else if (p->type == 3) {
		switch (p->type3.op) {
		case PLUS: {
			int reg1 = getReg(p->type3.arg1, outfile, 1);
			int reg2 = getReg(p->type3.arg2, outfile, 2);
			int reg3 = getReg(p->type3.arg3, outfile, 2);
			if (p->type3.arg1.ctrl == 0)
				fprintf(outfile, "add $%d, $%d, $%d\n", reg1, reg2, reg3);
			else {
				fprintf(outfile, "add $v1, $%d, $%d\n", reg2, reg3);
				fprintf(outfile, "sw $v1, 0($%d)\n", reg1);
			}
			break;
		}
		case MINUS: {
			int reg1 = getReg(p->type3.arg1, outfile, 1);
			int reg2 = getReg(p->type3.arg2, outfile, 2);
			int reg3 = getReg(p->type3.arg3, outfile, 2);
			if (p->type3.arg1.ctrl == 0)
				fprintf(outfile, "sub $%d, $%d, $%d\n", reg1, reg2, reg3);
			else {
				fprintf(outfile, "sub $v1, $%d, $%d\n", reg2, reg3);
				fprintf(outfile, "sw $v1, 0($%d)\n", reg1);
			}
			break;
		}
		case STAR: {
			int reg1 = getReg(p->type3.arg1, outfile, 1);
			int reg2 = getReg(p->type3.arg2, outfile, 2);
			int reg3 = getReg(p->type3.arg3, outfile, 2);
			if (p->type3.arg1.ctrl == 0)
				fprintf(outfile, "mul $%d, $%d, $%d\n", reg1, reg2, reg3);
			else {
				fprintf(outfile, "mul $v1, $%d, $%d\n", reg2, reg3);
				fprintf(outfile, "sw $v1, 0($%d)\n", reg1);
			}
			break;
		}
		case DIV: {
			int reg1 = getReg(p->type3.arg1, outfile, 1);
			int reg2 = getReg(p->type3.arg2, outfile, 2);
			int reg3 = getReg(p->type3.arg3, outfile, 2);
			if (p->type3.arg1.ctrl == 0)
				fprintf(outfile, "mul $%d, $%d, $%d\n", reg1, reg2, reg3);
			else {
				fprintf(outfile, "mul $v1, $%d, $%d\n", reg2, reg3);
				fprintf(outfile, "sw $v1, 0($%d)\n", reg1);
			}
			break;
			break;
		}
		case IF: {
			int reg1 = getReg(p->type3.arg1, outfile, 2);
			int reg2 = getReg(p->type3.arg2, outfile, 2);
			switch (p->type3.relop) {
			case 0: fprintf(outfile, "bgt $%d, $%d, ", reg1, reg2); break; // >
			case 1: fprintf(outfile, "blt $%d, $%d, ", reg1, reg2); break; // <
			case 2: fprintf(outfile, "bge $%d, $%d, ", reg1, reg2); break; // >=
			case 3: fprintf(outfile, "ble $%d, $%d, ", reg1, reg2); break; // <=
			case 4: fprintf(outfile, "beq $%d, $%d, ", reg1, reg2); break; // ==
			case 5: fprintf(outfile, "bne $%d, $%d, ", reg1, reg2); break; // !=
			}
			printOPE(outfile, &p->type3.arg3); 
			fprintf(outfile, "\n");
			break;
		}
		default: break;
		}
	}
 }

void addOffset(Operand op) {
	if (op.type == TMP || op.type == VAR) {
		int idx = (op.type == TMP) ? op.num + varcnt : op.num;
		if (allVar[idx].offset == -1) {
			allVar[idx].offset = spillcnt;
			spillcnt += 4;
		}
	}
}

int dealwithARG(ICNode* q, FILE* outfile) {
	int cnt = 0;
	int total_offset = 80;
	for (ICNode* temp = q; temp->type == 1 && temp->type1.op == ARG; temp = temp->next) {
		int reg;
		if (temp->type1.arg.ctrl == 0) {
			reg = getReg(temp->type1.arg, outfile, 2);
		}
		else if (temp->type1.arg.ctrl == 2 /* * */) {
			int idx = (temp->type1.arg.type == TMP) ? temp->type1.arg.num + varcnt : temp->type1.arg.num;
			reg = allVar[idx].reg;
			if (reg != -1) {
				putSW(reg, allVar[idx].offset + total_offset, outfile);
				allReg[reg].filled = 0;
			}
			fprintf(outfile, "lw $t9, %d($sp)\n", allVar[idx].offset + total_offset);
			fprintf(outfile, "lw $t9, 0($t9)\n");
			reg = 25;
		}
		if (cnt <= 3) {
			fprintf(outfile, "move $a%d, $%d\n", cnt, reg);
		}
		else {
			fprintf(outfile, "sub $sp, $sp, 4\n");
			total_offset += 4;
			fprintf(outfile, "lw $%d, 0(sp)\n", reg);
		}
		cnt++;
	}
	return cnt;
}

int dealwithPARAM(ICNode* q, FILE* outfile) {
	int cnt = 0;
	ICNode* temp = q;
	while (1) {
		if (temp->type != 1 || temp->type1.op != PARAM)
			break;
		// count the num of params
		cnt++;
		temp = temp->next;
	}
	temp = q;
	int off = 4;
	while (cnt > 4) {
		fprintf(outfile, "lw $s8, %d($sp)\n", off + spillcnt);  // param is on the stack
		Operand temp_arg = temp->type1.arg;
		int var_idx = temp_arg.num;
		fprintf(outfile, "sw $s8, %d($sp)\n", allVar[temp_arg.num].offset);
		off += 4;
		cnt--;
		temp = temp->next;
	}
	while (cnt > 0) {
		if (temp->type != 1 || temp->type1.op != PARAM)
			break;
		Operand temp_arg = temp->type1.arg;
		int var_idx = temp_arg.num;
		fprintf(outfile, "sw $a%d, %d($sp)\n", cnt - 1, allVar[temp_arg.num].offset); // param is in reg $a0 ~ $a3
		cnt--;
		temp = temp->next;
	}
	return cnt;
}

void genASM(FILE* outfile) {
	allReg = malloc(sizeof(RegDesc) * 32);
	allVar = malloc(sizeof(VarDesc) * (varcnt + tmpcnt + 10)); //var: index tmp: index + varcnt

	memset(allReg, 0x00, sizeof(RegDesc) * 32);
	memset(allVar, 0xFF, sizeof(VarDesc) * (varcnt + tmpcnt + 10));

	putPre(outfile);
	parseBlock();
	if (debug && debug_asm) {
		BlockPointer* p = BPhead;
		while(p != NULL) {
			ICNode* ptr = p->pointer;
			if (ptr != NULL)
				printf("%d %d\n", ptr->type, ptr->type1.op);
			else
				printf("NULL\n");
			p = p->next;
		}
	}

	for (BlockPointer* p = BPhead; p != NULL; p = p->next) {
		spillcnt = 0; //5 * 4
		ICNode* ps = p->pointer;
		ICNode* pe = NULL;
		if (p->next != NULL) pe = p->next->pointer;
		for (ICNode* q = ps; q != pe; q = q->next) {
			if (q->type == 1) {
				if (q->type1.op == DEC) {
					int idx = q->type1.arg.num;     // must be var
					allVar[idx].offset = spillcnt;
					spillcnt += q->type1.decsize;
				}
				else {
					addOffset(q->type1.arg);
				}	
			}
			else if (q->type == 2) {
				addOffset(q->type2.arg1);
				addOffset(q->type2.arg2);
			}
			else if (q->type == 3) {
				addOffset(q->type3.arg1);
				addOffset(q->type3.arg2);
				addOffset(q->type3.arg3);
			}
		}
		if (debug && debug_asm) printf("spillcnt = %d\n", spillcnt);
		for (ICNode* q = ps; q != pe; q = q->next) {
			if (q->type == 1 && q->type1.op == ARG) {
				// begin calling another function, save useful regs
				fprintf(outfile, "addi $sp, $sp, -64\n");
				for (int i = 8; i < 24; i++) {
					fprintf(outfile, "sw $%d, %d($sp)\n", i, 4 * (i - 8));
				}
				fprintf(outfile, "addi $sp, $sp, -16\n");
				fprintf(outfile, "sw $a0, 0($sp)\n");
				fprintf(outfile, "sw $a1, 4($sp)\n");
				fprintf(outfile, "sw $a2, 8($sp)\n");
				fprintf(outfile, "sw $a3, 12($sp)\n");

				int paramnum = dealwithARG(q, outfile);
				while (q->type == 1 && q->type1.op == ARG) q = q->next;
				
				assert(q->type == 2 && q->type2.op == CALL);

				fprintf(outfile, "addi $sp, $sp, -4\n");
				fprintf(outfile, "sw $ra, 0($sp)\n");

				fprintf(outfile, "jal %s\n", q->type2.arg2.name);

				fprintf(outfile, "lw $ra, 0($sp)\n");
				fprintf(outfile, "addi $sp, $sp, 4\n");

				if (paramnum > 4)
					fprintf(outfile, "addi $sp, $sp, %d\n", 4 * (paramnum - 4));

				fprintf(outfile, "lw $a0, 0($sp)\n");
				fprintf(outfile, "lw $a1, 4($sp)\n");
				fprintf(outfile, "lw $a2, 8($sp)\n");
				fprintf(outfile, "lw $a3, 12($sp)\n");
				fprintf(outfile, "addi $sp, $sp, 16\n");
				for (int i = 8; i < 24; i++) {
					fprintf(outfile, "lw $%d, %d($sp)\n", i, 4 * (i - 8));
				}
				fprintf(outfile, "addi $sp, $sp, 64\n");

				Operand temp_op = q->type2.arg1;
				int temp_idx = (temp_op.type == TMP) ? temp_op.num + varcnt : temp_op.num;
				fprintf(outfile, "sw $v0, %d($sp)\n", allVar[temp_idx].offset);
			}
			else if (q->type == 2 && q->type2.op == CALL) {
				// no arg, only CALL, so paramnum = 0
				fprintf(outfile, "addi $sp, $sp, -64\n");
				for (int i = 8; i < 24; i++) {
					fprintf(outfile, "sw $%d, %d($sp)\n", i, 4 * (i - 8));
				}
				fprintf(outfile, "addi $sp, $sp, -16\n");
				fprintf(outfile, "sw $a0, 0($sp)\n");
				fprintf(outfile, "sw $a1, 4($sp)\n");
				fprintf(outfile, "sw $a2, 8($sp)\n");
				fprintf(outfile, "sw $a3, 12($sp)\n");
				
				fprintf(outfile, "addi $sp, $sp, -4\n");
				fprintf(outfile, "sw $ra, 0($sp)\n");

				fprintf(outfile, "jal %s\n", q->type2.arg2.name);

				fprintf(outfile, "lw $ra, 0($sp)\n");
				fprintf(outfile, "addi $sp, $sp, 4\n");

				fprintf(outfile, "lw $a0, 0($sp)\n");
				fprintf(outfile, "lw $a1, 4($sp)\n");
				fprintf(outfile, "lw $a2, 8($sp)\n");
				fprintf(outfile, "lw $a3, 12($sp)\n");
				fprintf(outfile, "addi $sp, $sp, 16\n");
				for (int i = 8; i < 24; i++) {
					fprintf(outfile, "lw $%d, %d($sp)\n", i, 4 * (i - 8));
				}
				fprintf(outfile, "addi $sp, $sp, 64\n");

				Operand temp_op = q->type2.arg1;
				int temp_idx = (temp_op.type == TMP) ? temp_op.num + varcnt : temp_op.num;
				fprintf(outfile, "sw $v0, %d($sp)\n", allVar[temp_idx].offset);
			}
			else if (q->type == 1 && q->type1.op == FUNCTION) {
				printOPE(outfile, &q->type1.arg); 
				fprintf(outfile, ":\n");

				q = q->next;

				fprintf(outfile, "sub $sp, $sp, %d\n", spillcnt);
				dealwithPARAM(q, outfile);
				while (q->type == 1 && q->type1.op == PARAM) q = q->next;
				q = q->pre;
			}
			else if (q->type == 1 && q->type1.op == RETURN) {
				fprintf(outfile, "move $v0, $%d\n", getReg(q->type1.arg, outfile, 2)); 
				fprintf(outfile, "addi $sp, $sp, %d\n", spillcnt);
				fprintf(outfile, "jr $ra\n"); 
			}
			else {
				IR2ASM(q, outfile);				
			}
		}
		// move all filled regs into mem
	}

	free(allReg);
	free(allVar);
	BlockPointer* p = BPhead;
	while(p != NULL) {
		BlockPointer* t = p;
		p = p->next;
		free(t);
	}
}