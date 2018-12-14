#ifndef _TYPESYS_H_
#define _TYPESYS_H_

struct DimNode {
	int dim;
	struct DimNode* next;
};
typedef struct DimNode DimNode;

struct TypeNode {
	enum {type_int, type_float, array, record} type;
	union {
		int ival;    //value of integer
		float fval;  //value of float
		int stridx;  //index of struct
	};
	struct TypeNode* args; //Args list of function
	int size; //int:4 float:4 arr&record: calculated later 
	DimNode* dims;
	struct TypeNode* left;
	struct TypeNode* right;
	struct TypeNode* argNext;
};
typedef struct TypeNode TypeNode;

TypeNode* createInteger(int val, char* name); //r->name == INT
TypeNode* createFloat(float val, char* name); //r->name == FLOAT
TypeNode* createRecord(int stridx, char* name); //r-name == STRUCT, val is struct index

TypeNode* createArray(int index, TypeNode* arr);

int judgeType(TypeNode* r);
int compType(TypeNode* r1, TypeNode* r2);
TypeNode* copyType(TypeNode* r);

void printType(TypeNode* r, int layer);

void delTypeList();

#endif