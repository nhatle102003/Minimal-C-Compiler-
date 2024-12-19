#ifndef CODEGEN_H

#define MAXREGISTER 8

#include "tree.h"

typedef struct identifier {
    char *label;
    bool is_arg;
    bool is_locl_var;
    int *fp_off_set;
    int *sp_off_set;
    struct identifier * next;
    struct identifier * prev;
} identifier;

void funcGenFooter();

void funcGen(tree *node);

void funcCodeGen(tree *root);

void newLabel(char *identifier);

void codeGen(tree *root);

int nextReg();

void allocate_global_vars(tree* root);

void loadVar(tree* root, char* registr, identifier* working_id_head);

/*macro stuff*/
void genOutput();

int getArgId(tree *node);

void genArgCode(tree *node);

void genLoopBody(tree *root);

void genLoopBodyContent(tree *root);


#endif