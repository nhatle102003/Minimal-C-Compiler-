#include "codegen.h"

//global stuff
int current_register = -1;
int local_variable_allocated = 0;
char start_label[100]; char end_label[100]; char generic_label[100];
char var_label[10];
char func_identifier[10];
char LHS_register[10]; char RHS_regsiter[10];
char *saved_register[8] = {"$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7"}; 
bool enter_function = false;
bool assgn_flag = false;
bool assgn_global = false;
bool assgn_local = false;
bool cond_check = false;
bool less_than_check = false;
bool greater_than_check = false;
bool arg_flag = false;
bool loop_flag = false;
bool loop_body_flag = false;
bool loop_met = false;
bool equal_check = false;
char loop_cond[100];
char loop_end[100];
char locl_LHS_registr[10];
char locl_RHS_registr[10];
char loop_prod_registr[10];
bool add_flag = false; bool sub_flag = false; bool expr_flag = true;

identifier *working_id_head = NULL;
identifier *working_id_tail = NULL;
identifier *util_id_pntr = NULL;

symEntry *function_called = NULL;

enum nodeTypes {PROGRAM, DECLLIST, DECL, VARDECL, TYPESPEC, FUNDECL,
                FORMALDECLLIST, FORMALDECL, FUNBODY, LOCALDECLLIST,
                STATEMENTLIST, STATEMENT, COMPOUNDSTMT, ASSIGNSTMT,
                CONDSTMT, LOOPSTMT, RETURNSTMT, EXPRESSION, RELOP,
                ADDEXPR, ADDOP, TERM, MULOP, FACTOR, FUNCCALLEXPR,
                ARGLIST, INTEGER, IDENTIFIER, VAR, ARRAYDECL, CHAR,
                FUNCTYPENAME};

enum opType {ADD, SUB, MUL, DIV, LT, LTE, EQ, GTE, GT, NEQ};

/*function to allocate registers and set up necessarily 
memory on the stack, also open a new file to write the 
assembly code into*/
void codeGenHeader(){
    printf("# Global variable allocations:\n");
    printf(".data\n");
    allocate_global_vars(ast);
    printf("\n");
    //code section
    printf(".text\n");
    printf(".globl main   # Declare the main label as the global entry point, needed for SPIM\n");

    printf("main:\n");
    printf("    jal startmain\n");
    printf("    li $v0, 10\n");
    printf("    syscall\n");
    printf("\n");
}

// Sets a new label value to print
void newLabel(char *identifier){
    // If the identifier is not null, associate the identifier with the start/end labels
    if(identifier){
	// Set the maximum length to the size of the label array size
        snprintf(start_label, sizeof(start_label), "start%s", identifier);
        snprintf(end_label, sizeof(end_label), "end%s", identifier);
    }
    /* If the identifier returns null, then store the current label count in generic_label
       and increment the label count */
    else{
        static int label_count = 1;
        snprintf(generic_label, sizeof(generic_label), "L%d", label_count);
        label_count++;
    }
}

// Prints global vars in the code gen
void allocate_global_vars(tree *root){
    // Variable to iterate through the globals
    int i;
    {   
	    // If the global variable exists and is of scalar type, proceed
            for(i = 0; i < MAXIDS; i++){
                if(global_scope->strTable[i] && global_scope->strTable[i]->symbol_type == SCALAR){
			// Store the ID of the global in var_label and print it to the code gen
                        snprintf(var_label, sizeof(var_label), "var%s", global_scope->strTable[i]->id);
                        printf("%s:     .word 0\n", var_label);
                }
            }
            
    }
}

// Iterates the registers in code gen
int nextReg(){
    // Increment current_register
    current_register++;
    // If the current register count exceeds the max, loop back to register 0
    if(current_register == MAXREGISTER){
        current_register = -1;
        current_register++;
    }
    // Return the current register
    return current_register;
}

// Primary code gen block
void codeGen(tree *root){
    // Print the program name if it's the start of the program
    if(root->nodeKind == PROGRAM){
        codeGenHeader();
    }
    // If the root is a function declaration, set enter_function to true
    else if(root->nodeKind == FUNDECL){
        enter_function =  true;
    }
    // If the root is a function and enter_function has been set to true, set up printing process
    else if(root->nodeKind == FUNCTYPENAME && enter_function){
	// Iterative values
        int j, k;
	// Iterate through each of the root's children
        for(j = 0; j < root->numChildren; j++){
            tree* node = getChild(root, j);
	    // If the child is an identifier, call newlabel and end the iteration
            if(node->nodeKind == IDENTIFIER){
                newLabel(node->scope->strTable[node->val]->id);
                break;  
            }
        }
        //allocate necessary register for each function call
        printf("%s:\n", start_label);
        printf("    sw $fp, ($sp)\n");
	    printf("    move $fp, $sp\n");
	    printf("    addi $sp, $sp, -4\n");

        printf("    # Saving registers\n");
	// Iterate through each register and print them accordingly
        for(k = 0; k < MAXREGISTER; k++){
            printf("    sw %s, ($sp)\n", saved_register[k]);
	        printf("    addi $sp, $sp, -4\n");
        }
    }
    // If the root is a function argument, set arg_flag to true
    else if(root->nodeKind == FORMALDECL){
        arg_flag = true;
        

        for(int i = 0; i < root->numChildren; i++){
            tree* node = getChild(root, i);
            if(node->nodeKind == IDENTIFIER){
                identifier *newID = (identifier*)malloc(sizeof(identifier));
                if(!working_id_head && !working_id_tail){
                    newID->label = malloc(strlen(node->scope->strTable[node->val]->id) + 1);
                    strcpy(newID->label, node->scope->strTable[node->val]->id);
                    newID->is_locl_var = false;
                    newID->is_arg = true;
                    newID->next = NULL;
                    newID->prev = NULL;
                    newID->fp_off_set = malloc(sizeof(int));
                    *newID->fp_off_set = 1;
                    working_id_head = newID;
                }else{
                    bool found = false;
                    newID->label = malloc(strlen(node->scope->strTable[node->val]->id) + 1);
                    strcpy(newID->label, node->scope->strTable[node->val]->id);
                    newID->is_locl_var = false;
                    newID->is_arg = true;
                    newID->next = NULL;
                    newID->fp_off_set = malloc(sizeof(int));
                    identifier * tail = working_id_tail;
                    while(tail){
                        if(tail->is_arg){
                            *newID->fp_off_set = *tail->fp_off_set + 1;
                            found = true;
                            break;
                        }
                        tail = tail->prev;
                    }
                    if(!found) *newID->fp_off_set = 1;
                    identifier * curr = working_id_head;

                    while(curr->next != NULL){
                        curr = curr->next;
                    }
                    curr->next = newID;
                    newID->prev = curr;                    
                }
                working_id_tail= newID;
            }
        }
    }
    // If the root is a function body, set up printing process
    else if(root->nodeKind == FUNBODY){
	// Call funcCodeGen with the root
        funcCodeGen(root);
	// Iterative values
        int j, k;
	// If the generic_label is an empty string, print it as is
        if(strcmp(generic_label, "")){
            printf("%s:\n", generic_label);
            strcpy(generic_label, "");
        }
	// Print the end label
        printf("%s:\n", end_label);
	// If there is at least one local var allocated, proceed
        if(local_variable_allocated > 0) {
	    // Print space de-allocation for the local variables
            printf("    # Deallocating space for %d local variables\n", local_variable_allocated);
	    // While there is at least one local var allocated, print to a register and decrement
            while(local_variable_allocated){
                printf("    addi $sp, $sp, 4\n");
                local_variable_allocated--;
            }
	}
	// Print register reload
        printf("    # Reloading registers\n");

	// Iterate through all possible registers in reverse
        for(j = MAXREGISTER - 1; j >= 0; --j){
	    // print an addi op and a lw with the iterated register
            printf("    addi $sp, $sp, 4\n");
            printf("    lw %s, ($sp)\n", saved_register[j]);
        }
	// Print fp op
        printf("    # Setting FP back to old value\n");
        printf("    addi $sp, $sp, 4\n");
        printf("    lw $fp, ($sp)\n");
        printf("    # Return to caller\n");
        printf("    jr $ra\n");

	// If end_label does NOT say endmain, call genOutput
        if(!strcmp(end_label, "endmain")){
            genOutput();
        }
    }
    int i;
    // Iterate through each of the root's children
    for(i = 0; i < root->numChildren; i++){
        // If the child node is not null, then recursively call codeGen with each child
        if(getChild(root, i)){
            codeGen(getChild(root, i));
        }
    }
}

// Code gen block for functions
void funcCodeGen(tree *root){
    // If the root is an assignment node and it is not set to loop, set the assignment flag to true
    if(root->nodeKind == ASSIGNSTMT && !loop_body_flag){
        assgn_flag = true;
    }
    // If the root is a list of parameters, set local assignment to true
    else if(root->nodeKind == LOCALDECLLIST){
        assgn_local = true;
    }
    // If the root is an identifier and the assignment flag is set, do an additional check
    else if(root->nodeKind == IDENTIFIER && assgn_flag){
	// If the scope of the root value is not set, then set global assignment to true and store the var label
        if(!root->scope->strTable[root->val]){
            snprintf(var_label, sizeof(var_label), "var%s", global_scope->strTable[root->val]->id);
            assgn_global = true;
        }
    }
    /* If the root is a variable declaration and the local assignment is true, print and increment
       the local variables allocated */
    else if(root->nodeKind == VARDECL && assgn_local){
        for(int i = 0; i < root->numChildren; i++){
            tree* node = getChild(root, i);
            if(node->nodeKind == IDENTIFIER){
                identifier *newID = (identifier*)malloc(sizeof(identifier));
                if(!working_id_head && !working_id_tail){
                    newID->label = malloc(strlen(node->scope->strTable[node->val]->id) + 1);
                    strcpy(newID->label, node->scope->strTable[node->val]->id);
                    newID->is_locl_var = true;
                    newID->is_arg = false;
                    newID->next = NULL;
                    newID->prev = NULL;
                    newID->sp_off_set = malloc(sizeof(int));
                    *newID->sp_off_set = 1;
                    working_id_head = newID;
                }else{
                    bool found = false;
                    newID->label = malloc(strlen(node->scope->strTable[node->val]->id) + 1);
                    strcpy(newID->label, node->scope->strTable[node->val]->id);
                    newID->is_locl_var = true;
                    newID->is_arg = false;
                    newID->next = NULL;
                    newID->sp_off_set = malloc(sizeof(int));
                    identifier * tail = working_id_tail;
                    while(tail){
                        if(tail->is_locl_var){
                            *newID->sp_off_set = *tail->sp_off_set + 1;
                            found = true;
                            break;
                        }
                        tail = tail->prev;
                    }
                    if(found) *newID->sp_off_set = 1;
                    identifier * curr = working_id_head;
                    while(curr->next != NULL){
                        curr = curr->next;
                    }
                    curr->next = newID;
                    newID->prev = curr;  
                }
                working_id_tail= newID;
            }
        }
        local_variable_allocated++;
        printf("\n");
        printf("    #Allocating %d local variable\n", local_variable_allocated);
        printf("    addi $sp, $sp, -4\n");
        printf("\n");
        assgn_local = false;
    }
    /* If the root is an integer, the assignment flag is true, a function was not called, and the 
       potential loop condition has not been met, then continue */
    else if(root->nodeKind == INTEGER && assgn_flag && !function_called && !loop_met){
	// Allocate memory for a char with a size 10x as large as a typical char variable
        char *registr = (char*) malloc(10 * sizeof(char));
	// If the global assignment flag is set, call nextReg for the next index and perform string/print ops
        if(assgn_global){
            int index = nextReg();
	    // Copy the register char into saved_registers at the index
            strcpy(registr, saved_register[index]);
	    // Print the corresponding li and sw assembly ops
            printf("    li %s, %d\n", registr, root->val);
            printf("    sw %s, %s\n", registr, var_label);
	    // Reset the assgn global flag
            assgn_global = false;
	// If the global assignment flag is not set, instead print the alternative sw op
        }else{
            int index = nextReg();
            strcpy(registr, saved_register[index]);
            printf("    li %s, %d\n", registr, root->val);
            printf("    sw %s, 4($sp)\n", registr);
        }
        // deallocate the register char and reset the assignment flag to false
        free(registr);
        assgn_flag = false;
	// Additionally reset the loop met flag
        if(loop_met){
            loop_met = false;
        }
    }
    // If the root is a function call, print a save in the return address
    else if(root->nodeKind == FUNCCALLEXPR){
        printf("    #Saving return address\n");
        printf("    sw $ra, ($sp)\n");
	// var for holding argument IDs
        int arg;
	// Iterate through each of the root children
        for(int i = 0; i < root->numChildren; i++){
            tree* node = getChild(root, i);
	    // If the child node exists, additionally check if it's an argument list
            if(node)
                if(node->nodeKind == ARGLIST){
		    // If it is, set arg to the result of getArgId with the child as a parameter
                    arg = getArgId(node);
                }
        }
	// Iterate through each of the children again
        for(int i = 0; i < root->numChildren; i++){
            tree* node = getChild(root, i);
	    // If the child node exists, additionally check if the child is an identifier
            if(node){
                if(node->nodeKind == IDENTIFIER){
		    // If the child value is within the global scope and the ID of that value is 'output', continue
                    if(global_scope->strTable[node->val] && !strcmp(global_scope->strTable[node->val]->id, "output")){
			// Allocate memory for a register and global_var register, with a size 10x and 100x of char, respectively
                        char *registr = (char*) malloc(10 * sizeof(char));
                        char *globl_var = (char*) malloc(100 * sizeof(char));
			// Set a new int to nextReg and copy register to the index returned in saved_register
                        int index = nextReg();
                        strcpy(registr, saved_register[index]);
                	// If the argument in strTable of the child node does not exist, continue
                        if(!node->scope->strTable[arg]){
			    // Iterate through all possible IDs
                            for(int k = 0; k < MAXIDS; k++){
				// Get the child scope and assign it to a table node pointer
                                table_node* locl_scope = getScopeChild(node->scope, k);
				// If the argument exists in the local scope, continue
                                if(locl_scope ->strTable[arg]){
				    // If the argument flag was set, print the lw op and reset the flag to false
                                    identifier *head = working_id_head;
                                    while(head){
                                        if(!strcmp(locl_scope ->strTable[arg]->id, head->label)){
                                            if(head->is_locl_var){
                                                printf("    lw %s, %d($sp)\n", registr, (*head->sp_off_set) * 4);
                                            }
                                            else{
                                                printf("    lw %s, %d($fp)\n", registr, (*head->fp_off_set) * 4);
                                            }
                                            break;
                                        }
                                        head = head->next;
                                    }
                                    // Cease any remaining iteration
                                    break;
                                }
                            }

                        }
			// If the argument does in fact exist, continue
                        else{
			    /* If the argument is not a global, then store the corresponding ID and print the 
				proper lw op */
                            if(!strcmp(node->scope->strTable[arg]->scope, "global")){
                                snprintf(globl_var, sizeof(globl_var), "var%s", global_scope->strTable[arg]->id);
                                printf("    lw %s, %s\n", registr, globl_var);
                            }
                        }
			// Print the remaining ops in code gen
                        printf("    sw %s, -4($sp)\n", registr);
                        printf("    addi $sp, $sp, -8\n");
                        printf("    jal startoutput\n");
                        printf("    addi $sp, $sp, 4\n");

                        printf("    addi $sp, $sp, 4\n");
                        printf("    lw $ra, ($sp)\n");
                        printf("\n");
                        printf("    # Move return value into another reg\n");
                        strcpy(registr, saved_register[nextReg()]);
                        printf("    move %s, $2\n", registr);
                        free(registr);
                        free(globl_var);
                    }
		    // If the child is not global or the ID is not output, continue
                    else{
			// If the node val does not exist in the scope, then assume it exists in the global scope
                        if(!node->scope->strTable[node->val]){
                            function_called = global_scope->strTable[node->val];
                            strcpy(func_identifier, global_scope->strTable[node->val]->id);
                        }
			// Otherwise, proceed as normal
                        else{
                            function_called = node->scope->strTable[node->val];
                            strcpy(func_identifier, node->scope->strTable[node->val]->id);
                        }  
			// Holds number of arguments in the function called
                        int arg_count = function_called->size; 
			// Allocate memory for a register and label, with a size 10x and 100x of char, respectively
                        char *registr = (char*) malloc(10 * sizeof(char));
                        char *label = (char*) malloc(100 * sizeof(char));
			// Iterate through each of the root's children
                        for(int i = 0; i < root->numChildren; i++){
                            tree* node = getChild(root, i);
			    // IF the child is an argument list, print it accordingly
                            if(node->nodeKind == ARGLIST){
                                printf("    # Evaluating and storing arguments\n");
                                genArgCode(node);
                            }
                        }
			// print the remaining ops in code gen
                        printf("    addi $sp, $sp, -4\n");
                        printf("\n");
                        printf("    # Jump to callee\n");
                        strcpy(label, "start");
                        strcat(label, func_identifier);
                        printf("\n");
                        printf("    # jal will correctly set $ra as well\n");
                        printf("    jal %s\n", label);

                        printf("\n");
                        // While there is still at least one argument, deallocate each and decrement arg_count
                        while(arg_count > 0){
                            printf("    # Deallocating space for arguments\n");
                            printf("    addi $sp, $sp, 4\n");
                            arg_count--;
                        }
                        // Finish printing remaining ops
                        printf("\n");
                        // # Resetting return address
                        printf("    addi $sp, $sp, 4\n");
                        printf("    lw $ra, ($sp)\n");

                        // # Move return value into another reg
                        strcpy(registr, saved_register[nextReg()]);
                        printf("    move %s, $2\n", registr);
			// Deallocate register and label
                        free(registr);
                        free(label);
                        
                    }
                }
            }
        }
    }
    // If the root is a condition statement (ironic), set condition check flag to true
    else if(root->nodeKind == CONDSTMT){
        cond_check = true;
    }
    // If the root is a var and the condition check flag is set, continue
    else if(root->nodeKind == VAR && cond_check){
        tree * id_node = getChild(root, 0);
	// Allocate memory for a register char 10 times the size of a char
        char *registr = (char*) malloc(10 * sizeof(char));
	// Copy the register to the next availible register and then copy LHS register to register
        strcpy(registr, saved_register[nextReg()]);
        strcpy(LHS_register, registr);
	// Print the lw op
        printf("    # Conditional statement\n");
        printf("    # Variable expression\n");

        loadVar(id_node, LHS_register, working_id_head);

	// Deallocate memory from register
        free(registr);
    }
    // If the root is a relop and the root value is LT, set the less than check to true
    else if(root->nodeKind == RELOP && root->val == LT){
        less_than_check = true;
    }
    // IF the root is a relop and the root value is GT, set the greater than check to true
    else if(root->nodeKind == RELOP && root->val == GT){
        greater_than_check = true;
    }
    else if(root->nodeKind == RELOP && root->val == EQ){
        equal_check = true;
    }
    // If the root is an integer and the condition flag is set, continue
    else if(root->nodeKind == INTEGER && cond_check){
	// Memory allocation for reg, prod_reg, bool_reg, and label
        char *registr = (char*) malloc(10 * sizeof(char));
        char *prod_registr = (char*) malloc(10 * sizeof(char));
        char *bool_registr = (char*) malloc(10 * sizeof(char));
        char *label = (char*) malloc(100 * sizeof(char));
	// Save RH register to the next availible and print the li op
        strcpy(registr, saved_register[nextReg()]);
        strcpy(RHS_regsiter, registr);
        printf("    # Integer expression\n");
        printf("    li %s, %d\n", RHS_regsiter, root->val);
	// If the less than check is true, allocate and print the sub/slt/beq op
        if(less_than_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            strcpy(registr, saved_register[nextReg()]);
            strcpy(bool_registr, registr);
            printf("    slt %s, %s, $0\n", registr, prod_registr);
            newLabel(NULL);
            printf("    beq %s, $0, %s\n", bool_registr, generic_label);
            printf("    # True case\n");
	    // Reset less than check
            less_than_check = false;
        }
	// If the greater than check is true, allocate and print accordingly
        else if(greater_than_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            strcpy(registr, saved_register[nextReg()]);
            strcpy(bool_registr, registr);
            printf("    slt %s, $0, %s\n", registr, prod_registr);
            newLabel(NULL);
            printf("    beq %s, $0, %s\n", bool_registr, generic_label);
            printf("    # True case\n");
            greater_than_check = false;
        }
	// If the equal check is true, then leave a simple sub/bne op
        else if(equal_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            newLabel(NULL);
            printf("    bne %s, $0, %s\n", prod_registr, generic_label);
            printf("    # True case\n");
            equal_check = false;
        }
	// Free memory and set condition check flag to false
        free(registr);
        free(prod_registr);
        free(bool_registr);
        free(label);
        cond_check = false;
    }
    // If the root is a loop statement, set loop flags to true
    else if(root->nodeKind == LOOPSTMT){
        loop_flag = true;
        loop_body_flag = true;
        loop_met = true;
	// Begin codegen on loop
        printf("    # Loop\n");
        genLoopBody(root);
    }
    // If the root is a simple statement and the loop body flag is set, begin loop body code gen
    else if(root->nodeKind == STATEMENT && loop_body_flag){
        genLoopBodyContent(root);
        printf("    b %s\n", loop_cond);
        printf("%s: \n", loop_end);
	// Reset loop body flag
        loop_body_flag = false;
        
    }
    // Iterate through the root children
    int i;
    for(i = 0; i < root->numChildren; i++){
	// Call recursively on each child
        if(getChild(root, i)){
            funcCodeGen(getChild(root, i));
        }
    }
}

// Get the ID of an argument
int getArgId(tree *node){
    // If the node exists, continue
    if(node)
	// If the node is an identifier, return its value
        if(node->nodeKind == IDENTIFIER){
            return node->val;
        }
    // Iterate through the node's children
    int i;
    for(i = 0; i < node->numChildren; i++){
	// Call recursivel on each child
        if(getChild(node, i)){
            int res = getArgId(getChild(node, i));
	    // If the child exists, return the ID
            if (res != -1)
                return res;
        }
    }
    // When all children are iterated, return -1
    return -1;
}
// Basic codegen print for output
void genOutput(){
    printf("# output function\n");
    printf("startoutput:\n");
    printf("    # Put argument in the output register\n");
    printf("    lw $a0, 4($sp)\n");
    printf("    # print int is syscall 1\n");    
    printf("    li $v0, 1\n");
    printf("    syscall\n");
    printf("    # jump back to caller\n");
    printf("    jr $ra\n");
}

// Code gen for arguments
void genArgCode(tree *node){
    // Allocate memory for reg
    char *registr = (char*) malloc(10 * sizeof(char));
    // IF the node is an integer, save to a register and print the li/sw/addi op
    if(node->nodeKind == INTEGER){
        strcpy(registr, saved_register[nextReg()]);
        printf("    li %s, %d\n", registr, node->val);

        printf("    sw %s, -4($sp)\n", registr);
        printf("    addi $sp, $sp, -4\n");
    }
    // Iterate through each child and call recursively on each
    for(int i = 0; i < node->numChildren; i++){
        if(getChild(node, i)){
            genArgCode(getChild(node, i));
        }
    }
}

bool loaded = false;
// codegen for loop body
void genLoopBodyContent(tree *root){
    // If the root is a var and the loaded flag is false, save the local LHS register and print the lw op
    if(root->nodeKind == VAR && !loaded){
        strcpy(locl_LHS_registr, saved_register[nextReg()]);
        tree * id_node = getChild(root, 0);
        
        loadVar(id_node, locl_LHS_registr, working_id_head);

	// Set loaded flag to true
        loaded = true;
    }
    // If the root is an add op and the root value is add, set the add flag
    else if(root->nodeKind == ADDOP && root->val == ADD){
        add_flag = true;
    }
    // If the root is an add op and the root value is sub, set the sub flag
    else if(root->nodeKind == ADDOP && root->val == SUB){
        sub_flag = true;
    }
    // If the root is an integer, store rhs register and print li op
    else if(root->nodeKind == INTEGER){
        strcpy(locl_RHS_registr, saved_register[nextReg()]);
        printf("    li %s, %d\n", locl_RHS_registr, root->val);
	// If it's an add op, print accordingly
        if(add_flag){
            printf("    # Arithmetic expression\n");
            strcpy(loop_prod_registr, saved_register[nextReg()]);
            printf("    add %s, %s, %s\n", loop_prod_registr, locl_LHS_registr, locl_RHS_registr);
            printf("    # Assignment\n");
            printf("    sw %s, 4($sp)\n", loop_prod_registr);
	    // Reset add and loaded flags
            add_flag = false;
            loaded = false;
        }
	// Otherwise, print with the sub flag
        else if(sub_flag){
            printf("    # Arithmetic expression\n");
            strcpy(loop_prod_registr, saved_register[nextReg()]);
            printf("    sub %s, $%s, %s\n", loop_prod_registr, locl_LHS_registr, locl_RHS_registr);
            printf("    # Assignment\n");
            printf("    sw %s, 4($sp)\n", loop_prod_registr);
	    // Reset sub and loaded flags
            sub_flag = false;
            loaded = false;
        }
    }
    // Iterate through the root children and call recursively on each
    for(int i = 0; i < root->numChildren; i++){
        if(getChild(root, i)) genLoopBodyContent(getChild(root, i));
    }
}

// Codegen for loop body
void genLoopBody(tree *root){
    // If the root is a var and the loop flag is set, continue
    if(root->nodeKind == VAR && loop_flag){
	// Nullify newLabel and create a generic label
        newLabel(NULL);
        strcpy(loop_cond, generic_label);
        strcpy(generic_label, "");
        printf("%s: \n", loop_cond);
	// Allocate memory for reg and save LHS to register
        char *registr = (char*) malloc(10 * sizeof(char));
        strcpy(registr, saved_register[nextReg()]);
        strcpy(LHS_register, registr);
	// Print the lw op
        printf("    # Conditional statement\n");
        printf("    # Variable expression\n");
        loadVar(getChild(root, 0), LHS_register, working_id_head);
	// Deallocate reg
        free(registr);
    }
    // If the root is a relop and the root value is lt, set less than flag
    else if(root->nodeKind == RELOP && root->val == LT){
        less_than_check = true;
    }
    // If the root is a relop and the root value is gt, set greater than flag
    else if(root->nodeKind == RELOP && root->val == GT){
        greater_than_check = true;
    }
    else if(root->nodeKind == RELOP && root->val == EQ){
        equal_check = true;
    }
    // If the root is an integer and the loop flag is set, continue
    else if(root->nodeKind == INTEGER && loop_flag){
	// Allocate memory for reg, prod_reg, and bool_reg
        char *registr = (char*) malloc(10 * sizeof(char));
        char *prod_registr = (char*) malloc(10 * sizeof(char));
        char *bool_registr = (char*) malloc(10 * sizeof(char));
        // Save rhs register and print li op
        strcpy(registr, saved_register[nextReg()]);
        strcpy(RHS_regsiter, registr);
        printf("    # Integer expression\n");
        printf("    li %s, %d\n", RHS_regsiter, root->val);
	// If the less than flag is set, allocate registers and print codegen accordingly
        if(less_than_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            strcpy(registr, saved_register[nextReg()]);
            strcpy(bool_registr, registr);
            printf("    slt %s, %s, $0\n", registr, prod_registr);
            newLabel(NULL);
            strcpy(loop_end, generic_label);
            strcpy(generic_label, "");
            printf("    beq %s, $0, %s\n", bool_registr, loop_end);
	// If the greater than flag is set, allocate registers and print codegen accordingly
        }
         else if(greater_than_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            strcpy(registr, saved_register[nextReg()]);
            strcpy(bool_registr, registr);
            printf("    slt %s, $0, %s\n", registr, prod_registr);
            newLabel(NULL);
            printf("    beq %s, $0, %s\n", bool_registr, generic_label);
            printf("    # True case\n");
            greater_than_check = false;
        }
	// If the equal check is true, then leave a simple sub/bne op
        else if(equal_check){
            strcpy(registr, saved_register[nextReg()]);
            strcpy(prod_registr, registr);
            printf("    sub %s, %s, %s\n", prod_registr, LHS_register, RHS_regsiter);
            newLabel(NULL);
            printf("    bne %s, $0, %s\n", prod_registr, generic_label);
            printf("    # True case\n");
            equal_check = false;
        }
	// Deallocate memory and reset loop flag
        free(registr);
        free(prod_registr);
        free(bool_registr);
        loop_flag = false;
    }
    // Iterate through the root children and call recursively on each
    for(int i = 0; i < root->numChildren; i++){
        if(getChild(root, i)){
            genLoopBody(getChild(root, i));
        }
    }
}

void loadVar(tree* node, char* reg, identifier* working_id_head){

    identifier *head = working_id_head;
    while(head){
        if(!strcmp(node->scope->strTable[node->val]->id, head->label)){
            if(head->is_locl_var){
                printf("    lw %s, %d($sp)\n", reg, (*head->sp_off_set) * 4);
            }
            else{
                printf("    lw %s, %d($fp)\n", reg, (*head->fp_off_set) * 4);
            }
            break;
        }
        head = head->next;
    }
}   
