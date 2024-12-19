#ifndef STRTAB_H
#define STRTAB_H
#define MAXIDS 1000
#define MAXCHILDRENS 100

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

enum dataType {INT_TYPE, CHAR_TYPE, VOID_TYPE};
enum symbolType {SCALAR, ARRAY, FUNCTION};

typedef struct param{
    int data_type;
    int symbol_type;
    struct param* next;
} param;

typedef struct symEntry{
    char* id;
    char* scope;
    int   data_type;
    int   symbol_type;
    int   symIndex;
    int   size; //Num elements if array, num params if function
    param*  params;
    /* You should use a linear linklist to keep track of all parameters passed to a function. The working_list_head should point to the beginning of the linklist and working_list_end should point to the end. Whenever a parameter is passed to a function, that node should also be added in this list. */
    param *working_list_pointer;
    param *working_list_end;
    param *working_list_head;
} symEntry;

extern param *working_list_head;
extern param *working_list_end;

typedef struct table_node{
    symEntry* strTable[MAXIDS];
    int numChildren;
    int paramCount;
    char*  super_scope;
    struct table_node* parent;
    struct table_node* first_child; // First subscope
    struct table_node* last_child;  // Most recently added subscope
    struct table_node* children[MAXCHILDRENS]; // Next subscopes that shares the same parent
} table_node; // Describes each node in the symbol table tree and is used to implement a tree for the nested scope as discussed in lecture 13 and 14.

extern table_node* current_scope; // A global variable that should point to the symbol table node in the scope tree as discussed in lecture 13 and 14.
extern table_node* global_scope;

extern table_node* symTableRoot;           //The symbol table tree root

/* Inserts a symbol into the current symbol table tree. Please note that this function is used to instead into the tree of symbol tables and NOT the AST. Start at the returned hash and probe until we find an empty slot or the id.  */
int ST_insert(char *id, char* scope, int data_type, int symbol_type);

/* The function for looking up if a symbol exists in the current_scope. Always start looking for the symbol from the node that is being pointed to by the current_scope variable*/
symEntry* ST_lookup(char *id, char *scope);

// Creates a new scope within the current scope and sets that as the current scope.
void new_scope();

// Moves towards the root of the sym table tree.
void up_scope();

void print_sym_tab();

// Adds parameter to a funciton in the tree
void addParam(symEntry *function, int data_type, int symbol_type);

// Checks a function parameter for validity
bool paramTypeChecking(symEntry* function, int data_type);

bool paramTypeAndSymbolChecking(symEntry* function, int data_type, int symbol_type);

// Checks a var assignment for validity
bool assgnTypeChecking(symEntry* variable, int data_type);

// Adds child to parent scope in the tree
void addChildToScope(table_node* parent, table_node* child);

void print_sym_table_util(table_node* root, int nestLevel);

extern table_node* get_current_scope();

#define nextAvailChildScope(node) node->children[node->numChildren]
#define getScopeChild(node, index) node->children[index]

#endif
