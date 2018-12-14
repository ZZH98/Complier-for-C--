
%option yylineno

%{
	#include "ParseTree.h"
	#include "semAnalysis.h"
	#include "typeSys.h"

	#define YYSTYPE Node*

	#include "syntax.tab.h"

	int yycolumn = 1;
	#define YY_USER_ACTION \
	yylloc.first_line = yylloc.last_line = yylineno; \
	yylloc.first_column = yycolumn; \
	yylloc.last_column = yycolumn + yyleng - 1; \
	yycolumn += yyleng; 

	int debug = 0;
	int debug_l = 0;
	extern int printAble;
%}

digit [0-9]
octDigit [0-7]
hexDigit [0-9a-fA-F]
digitNoZero [1-9]
IDletter [_a-zA-Z0-9]
IDletterNoDigit [_a-zA-Z]

%%

"\n" {
	if(debug && debug_l) printf("change line\n");
	yycolumn = 1;
}

0{octDigit}+ {
	if(debug && debug_l) printf("Octal integer value %d at line %d\n", (int)strtol(yytext, NULL, 0), yylineno); 
	yylval = createNode("INT", yylloc.first_line, yylloc.first_column, (int)strtol(yytext, NULL, 0));
	return INT;
}

0[Xx]{hexDigit}+ {
	if(debug && debug_l) printf("Hex integer value %d at line %d\n", (int)strtol(yytext, NULL, 0), yylineno); 
	yylval = createNode("INT", yylloc.first_line, yylloc.first_column, (int)strtol(yytext, NULL, 0));
	return INT;
}

"0"|{digitNoZero}{digit}* { 
	if(debug && debug_l) printf("Integer value %d at line %d\n", atoi(yytext), yylineno); 
	yylval = createNode("INT", yylloc.first_line, yylloc.first_column, atoi(yytext));
	return INT;
}
({digit}+\.{digit}+|{digit}+\.|\.{digit}+)([eE][+-]?{digit}+)? {
	if(debug && debug_l) printf("Float value %f at line %d\n", atof(yytext), yylineno);
	yylval = createNode("FLOAT", yylloc.first_line, yylloc.first_column, atof(yytext));
	return FLOAT;
}

"//" {
	if (debug && debug_l) printf("One Line Comment\n");
	char c = input();
	while (c != '\n') {
		if (debug && debug_l) printf("c = %c\n", c);
		c = input();
	}
}

"/*" {
	int state = 0;
	char c = input();
	while (state != 2) {
		if (state == 0 && c == '*')
			state = 1;
		if (state == 1 && c == '/')
			state = 2;
		if (state == 1 && c == '*')
			state = 1;
		else if (state == 1)
			state = 0;
		if (debug && debug_l) printf("c = %c\n", c);
		c = input();
	}

}

"+" {
	if(debug && debug_l) printf("PLUS\n");
	yylval = createNode("PLUS", yylloc.first_line, yylloc.first_column);
	return PLUS;
}
"-" {
	if(debug && debug_l) printf("MINUS\n");
	yylval = createNode("MINUS", yylloc.first_line, yylloc.first_column);
	return MINUS;
}
"*" {
	if(debug && debug_l) printf("STAR\n");
	yylval = createNode("STAR", yylloc.first_line, yylloc.first_column);
	return STAR;
}
"/" {
	if(debug && debug_l) printf("DIV\n");
	yylval = createNode("DIV", yylloc.first_line, yylloc.first_column);
	return DIV;
}
"&&" {
	if(debug && debug_l) printf("AND\n");
	yylval = createNode("AND", yylloc.first_line, yylloc.first_column);
	return AND;
}
"||" {
	if(debug && debug_l) printf("OR\n");
	yylval = createNode("OR", yylloc.first_line, yylloc.first_column);
	return OR;
}
"!" {
	if(debug && debug_l) printf("NOT\n");
	yylval = createNode("NOT", yylloc.first_line, yylloc.first_column);
	return NOT;
}

"=" {
	if (debug && debug_l) printf("ASSIGNOP\n");
	yylval = createNode("ASSIGNOP", yylloc.first_line, yylloc.first_column);
	return ASSIGNOP;
}
">" {
	if (debug && debug_l) printf("RELOPG\n");
	yylval = createNode("RELOPG", yylloc.first_line, yylloc.first_column);
	return RELOPG;
}
"<" {
	if (debug && debug_l) printf("RELOPL\n");
	yylval = createNode("RELOPL", yylloc.first_line, yylloc.first_column);
	return RELOPL;
}
">=" {
	if (debug && debug_l) printf("RELOPGE\n");
	yylval = createNode("RELOPGE", yylloc.first_line, yylloc.first_column);
	return RELOPGE;
}
"<=" {
	if (debug && debug_l) printf("RELOPLE\n");
	yylval = createNode("RELOPLE", yylloc.first_line, yylloc.first_column);
	return RELOPLE;
}
"==" {
	if (debug && debug_l) printf("RELOPE\n");
	yylval = createNode("RELOPE", yylloc.first_line, yylloc.first_column);
	return RELOPE;
}
"!=" {
	if (debug && debug_l) printf("RELOPNE\n");
	yylval = createNode("RELOPNE", yylloc.first_line, yylloc.first_column);
	return RELOPNE;
}

";" {
	if (debug && debug_l) printf("SEMI\n");
	yylval = createNode("SEMI", yylloc.first_line, yylloc.first_column);
	return SEMI;
}
"," {
	if (debug && debug_l) printf("COMMA\n");
	yylval = createNode("COMMA", yylloc.first_line, yylloc.first_column);
	return COMMA;
}
"." {
	if (debug && debug_l) printf("DOT\n");
	yylval = createNode("DOT", yylloc.first_line, yylloc.first_column);
	return DOT;
}
"int"|"float" {
	if (debug && debug_l) printf("TYPE\n");
	char* temp = malloc(32);
	int i = 0;
	for (; i < yyleng; i++) {
		temp[i] = yytext[i];
	}
	temp[i] = 0;
	yylval = createNode("TYPE", yylloc.first_line, yylloc.first_column, temp);
	return TYPE;
}
"(" {
	if (debug && debug_l) printf("LP\n");
	yylval = createNode("LP", yylloc.first_line, yylloc.first_column);
	return LP;
}
")" {
	if (debug && debug_l) printf("RP\n");
	yylval = createNode("RP", yylloc.first_line, yylloc.first_column);
	return RP;
}
"[" {
	if (debug && debug_l) printf("LB\n");
	yylval = createNode("LB", yylloc.first_line, yylloc.first_column);
	return LB;
}
"]" {
	if (debug && debug_l) printf("RB\n");
	yylval = createNode("RB", yylloc.first_line, yylloc.first_column);
	return RB;
}
"{" {
	if (debug && debug_l) printf("LC\n");
	yylval = createNode("LC", yylloc.first_line, yylloc.first_column);
	return LC;
}
"}" {
	if (debug && debug_l) printf("RC\n");
	yylval = createNode("RC", yylloc.first_line, yylloc.first_column);
	return RC;
}

"struct" {
	if(debug && debug_l) printf("STRUCT\n");
	yylval = createNode("STRUCT", yylloc.first_line, yylloc.first_column);
	return STRUCT;
}
"return" {
	if(debug && debug_l) printf("RETURN\n");
	yylval = createNode("RETURN", yylloc.first_line, yylloc.first_column);
	return RETURN;
}

"if" {
	if(debug && debug_l) printf("IF\n");
	yylval = createNode("IF", yylloc.first_line, yylloc.first_column);
	return IF;
}
"else" {
	if(debug && debug_l) printf("ELSE\n");
	yylval = createNode("ELSE", yylloc.first_line, yylloc.first_column);
	return ELSE;
}
"while" {
	if(debug && debug_l) printf("WHILE\n");
	yylval = createNode("WHILE", yylloc.first_line, yylloc.first_column);
	return WHILE;
}
{IDletterNoDigit}{IDletter}* {
	if(debug && debug_l) printf("ID %s at line %d\n", yytext, yylineno);
	char* temp = malloc(32);
	int i = 0;
	for (; i < yyleng; i++) {
		temp[i] = yytext[i];
	}
	temp[i] = 0;
	yylval = createNode("ID", yylloc.first_line, yylloc.first_column, temp);
	return ID;
}

" " {

}
"\t" {
	yycolumn += 3; 
}
. {
	printf ("Error type A at Line %d: Mysterious characters \'%s\'\n", yylineno, yytext);
	printAble = 0;
}

%%
