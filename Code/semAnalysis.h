#ifndef _SEMANALYSIS_H_
#define _SENANALYSIS_H_

#define HASHLIST_LEN 0x3fff
#define STRUCT_NUM 1024
#define FUNC_NUM 1024
#define MAX_VAR_LEN 32

#include "ParseTree.h"
#include "typeSys.h"

int varcnt;

struct HashNode {
	char* name;
	TypeNode* type;
	int offset; //in struct
	Operand ICope;
	struct HashNode* next;
};
typedef struct HashNode HashNode;


HashNode* HNListVar[HASHLIST_LEN];
HashNode* HNListFunc[HASHLIST_LEN];

HashNode** structHashTableList[STRUCT_NUM];
int sindex;

void examineStruct(Node* root, int sindex);
void examineVarDef(Node* root, int errortype);
void examineFuncDef(Node* root);
HashNode* examineVarUse(Node* root, int errortype);
HashNode* examineFuncUse(Node* root);

void examineSem(Node* r);

void initialType(Node* root);
void initialTypeExt(Node* root);

void fillInType(Node* r);

void fillInExpType(Node* r);

unsigned int hash_pjw(char* name);

int addVar(char* name, TypeNode* type);
int addFunc(char* name, TypeNode* type);

HashNode* findVar(char* name);
HashNode* findFunc(char* name);

void addRW();
void delStrList();

#endif