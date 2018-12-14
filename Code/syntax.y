%locations

%{
	#include "lex.yy.c"
	//#define YYDEBUG 1
	//#define YYSTYPE Node*

	int printAble = 1;
	Node* root;

	void yyerror(char* str) {
		printf("Error type B at Line %d : Syntax error.\n", yylineno);
		printAble = 0;
		exit(-1);
	}

%}

%token INT
%token FLOAT
%token ID
%token PLUS MINUS STAR DIV
%token AND OR NOT
%token ASSIGNOP RELOPG RELOPL RELOPGE RELOPLE RELOPE RELOPNE
%token SEMI COMMA DOT
%token TYPE
%token LP RP LB RB LC RC
%token STRUCT
%token RETURN
%token IF ELSE WHILE

%right ASSIGNOP
%left OR
%left AND
%left RELOPG RELOPL RELOPGE RELOPLE RELOPE RELOPNE
%left PLUS MINUS 
%left STAR DIV 
%right NOT
%left LP RP LB RB DOT

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE

%%

Program : ExtDefList {
		$$ = createTree("Program", 0, @$.first_line, @$.first_column, 1, $1);
		root = $$;
}
	;

ExtDefList : {
		$$ = createTree("ExtDefList", 0, @$.first_line, @$.first_column, 0);
}
	| ExtDef ExtDefList {
		$$ = createTree("ExtDefList", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	;
ExtDef : Specifier ExtDecList SEMI {
		$$ = createTree("ExtDef", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Specifier ExtDecList error SEMI {
		printf("	                Detail: Missing \';\'\n");
}
	| Specifier SEMI {
		$$ = createTree("ExtDef", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	| Specifier error SEMI {
		printf("	                Detail: Missing \';\'\n");
}
	| Specifier FunDec CompSt {
		$$ = createTree("ExtDef", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	;
ExtDecList : VarDec {
		$$ = createTree("ExtDecList", 0, @$.first_line, @$.first_column, 1, $1);
}
	| VarDec COMMA ExtDecList {
		$$ = createTree("ExtDecList", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	;

Specifier : TYPE {
		$$ = createTree("Specifier", 0, @$.first_line, @$.first_column, 1, $1);
		if (strcmp($1->cval, "int") == 0)
			$1->type = createInteger($1->ival, $1->name);
		else if (strcmp($1->cval, "float") == 0)
			$1->type = createFloat($1->fval, $1->name);
}
	| StructSpecifier {
		$$ = createTree("Specifier", 0, @$.first_line, @$.first_column, 1, $1);
}
	;
StructSpecifier : STRUCT OptTag LC DefList RC {
		$$ = createTree("StructSpecifier", 0, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
}
	| STRUCT OptTag LC DefList error RC {
		printf("	                Detail: Missing \'}\'\n");
}
	| STRUCT Tag {
		$$ = createTree("StructSpecifier", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	;
OptTag : {
		$$ = createTree("OptTag", 0, @$.first_line, @$.first_column, 0);
}
	| ID {
		$$ = createTree("OptTag", 0, @$.first_line, @$.first_column, 1, $1);
}
	;
Tag : ID {
		$$ = createTree("Tag", 0, @$.first_line, @$.first_column, 1, $1);
}
	;

VarDec : ID {
		$$ = createTree("VarDec", 0, @$.first_line, @$.first_column, 1, $1);
}
	| VarDec LB INT RB {
		$$ = createTree("VarDec", 0, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
}
	| VarDec LB INT error RB {
		printf("	                Detail: Missing \']\'\n");
}
	;
FunDec : ID LP VarList RP {
		$$ = createTree("FunDec", 0, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
}
	| ID LP VarList error RP {
		printf("	                Detail: Missing \')\'\n");
}
	| ID LP RP {
		$$ = createTree("FunDec", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| ID LP error RP {
		printf("	                Detail: Missing \')\'\n");
}
	;
VarList : ParamDec COMMA VarList {
		$$ = createTree("VarList", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| ParamDec {
		$$ = createTree("VarList", 0, @$.first_line, @$.first_column, 1, $1);
}
	;
ParamDec : Specifier VarDec {
		$$ = createTree("ParamDec", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	;

CompSt : LC DefList StmtList RC {
		$$ = createTree("CompSt", 0, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
}
	| LC DefList StmtList error RC {
		printf("	                Detail: Missing \'}\'\n");
}
	;
StmtList : {
		$$ = createTree("StmtList", 0, @$.first_line, @$.first_column, 0);
}
	| Stmt StmtList {
		$$ = createTree("StmtList", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	;
Stmt : Exp SEMI {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	| Exp error SEMI {
		printf("	                Detail: Missing \';\'\n");
}
	| CompSt {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 1, $1);
}
	| RETURN Exp SEMI {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| RETURN Exp error SEMI {
		printf("	                Detail: Missing \';\'\n");
}
	| IF LP Exp RP Stmt %prec LOWER_THAN_ELSE {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
}
	| IF LP Exp error RP Stmt %prec LOWER_THAN_ELSE {
		printf("	                Detail: Missing \')\'\n");
}
	| IF LP Exp RP Stmt ELSE Stmt {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 7, $1, $2, $3, $4, $5, $6, $7);
}
	| IF LP Exp error RP Stmt ELSE Stmt {
		printf("	                Detail: Missing \')\'\n");
}
	| WHILE LP Exp RP Stmt {
		$$ = createTree("Stmt", 0, @$.first_line, @$.first_column, 5, $1, $2, $3, $4, $5);
}
	| WHILE LP Exp error RP Stmt {
		printf("	                Detail: Missing \')\'\n");
}	
	;

DefList : {
		$$ = createTree("DefList", 0, @$.first_line, @$.first_column, 0);
}
	| Def DefList {
		$$ = createTree("DefList", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	;
Def : Specifier DecList SEMI {
		$$ = createTree("Def", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Specifier DecList error SEMI {
		printf("	                Detail: Missing \';\'\n");
}
	;
DecList : Dec {
		$$ = createTree("DecList", 0, @$.first_line, @$.first_column, 1, $1);
}
	| Dec COMMA DecList {
		$$ = createTree("DecList", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	;
Dec : VarDec {
		$$ = createTree("Dec", 0, @$.first_line, @$.first_column, 1, $1);
}
	| VarDec ASSIGNOP Exp {
		$$ = createTree("Dec", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	;

Exp : Exp ASSIGNOP Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp AND Exp{
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp OR Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPG Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPL Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPGE Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPLE Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPE Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp RELOPNE Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp PLUS Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp MINUS Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp STAR Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp DIV Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| LP Exp RP {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| MINUS Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	| NOT Exp {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 2, $1, $2);
}
	| ID LP Args RP {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
}
	| ID LP Args error RP {
		printf("	                Detail: Missing \')\'\n");
}
	| ID LP RP {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| ID LP error RP {
		printf("	                Detail: Missing \')\'\n");
}
	| Exp LB Exp RB {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 4, $1, $2, $3, $4);
}
	| Exp LB Exp error RB {
		printf("	                Detail: Missing \']\'\n");
}
	| Exp DOT ID {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| ID {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 1, $1);
}
	| INT {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 1, $1);
		$1->type = createInteger($1->ival, $1->name);
}
	| FLOAT {
		$$ = createTree("Exp", 0, @$.first_line, @$.first_column, 1, $1);
		$1->type = createFloat($1->fval, $1->name);
}
	;

Args : Exp COMMA Args {
		$$ = createTree("Args", 0, @$.first_line, @$.first_column, 3, $1, $2, $3);
}
	| Exp {
		$$ = createTree("Args", 0, @$.first_line, @$.first_column, 1, $1);
}
	;

%%
