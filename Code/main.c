#include <stdio.h>
#include <stdlib.h>
#include "ParseTree.h"
#include "semAnalysis.h"
#include "typeSys.h"
#include "IR.h"

extern FILE* yyin;
//int yydebug = 1;
extern void yyrestart (FILE *input_file);
extern int yyparse(void);
extern int yylex(void);

extern void printTree(Node* n, int layer);
extern int printAble;
extern Node* root;

extern void delTree(void);

int main (int argc, char* argv[]) {
	if (argc <= 1) return 1;
	FILE* f = fopen(argv[1], "r");
	if (!f) {
		perror(argv[1]);
		return 1;
	}

	yyrestart(f);
	yyparse();

	//while (yylex() != 0) ;
	//if (printAble)
		//printTree(root, 0);
	fillInType(root);
	addRW();
	examineSem(root);
	genIR(root);
	FILE* outfile = fopen(argv[2], "w");
	if (outfile)
		printIR(outfile);
	else
		printIR(stdout);

	delTree();
	delStrList();
	delTypeList();
	delIR();
	return 0;
}
