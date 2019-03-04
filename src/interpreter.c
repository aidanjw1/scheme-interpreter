#include <stdio.h>
#include <string.h>
#include "value.h"
#include "interpreter.h"
#include "talloc.h"
#include "assert.h"
#include "linkedlist.h"
#include "tokenizer.h"
#include "parser.h"


// Checks for the value of a symbol if it has been defined in the current frame
// Returns error if undefined so far
Value *lookUpSymbol(Value *symbol, Frame *frame) {
    Value *current = frame->bindings;
    while (current->type != NULL_TYPE) {
        Value *param1 = car(car(current));
        Value *bind1 = car(cdr(car(current)));
        if (!strcmp(param1->s, symbol->s)) {
            return bind1;
        }
        current = cdr(current);
    }
    if (frame->parent != NULL){
        return lookUpSymbol(symbol, frame->parent);
    }
    else {
        printf("%s: undefined; cannot reference an identifier before its definition", symbol->s);
        texit(1);
    }
    return symbol;
}

// Checks if a symbol has already been defined, to prevent duplicates.
bool symbolDefined(Value *symbol, Value *bindings) {
    Value *current = bindings;
    while (current->type != NULL_TYPE) {
        if (!strcmp(car(car(current))->s, symbol->s)) {
            return true;
        }
        current = cdr(current);
    }
    return false;
}

// Evaluates the arguments of an if statement.
// If true, returns first arg
// If false, returns second arg
Value *evalIf(Value *args, Frame *frame) {
    if (length(args) != 3) {
        printf("if: bad syntax in if");
        texit(1);
    }
    Value *condition = eval(car(args), frame);
    if(condition->type != BOOL_TYPE) {
        printf("Evaluation Error");
        texit(1);
    }
    Value *first = car(cdr(args));
    Value *second = car(cdr(cdr(args)));
    if (!strcmp(condition->s, "#t")) {
        return eval(first, frame);
    }
    else {
        return eval(second, frame);
    }
}

// Evaluates the arguments of a let statement
// Creates a set of bindings, evaluates the al other arguments
// Returns the last argument
Value *evalLet(Value* args, Frame *frame) {
    if (length(args) < 2) {
        printf("let: bad syntax (missing binding pairs or body)");
        texit(1);
    }
    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->parent = frame;
    newFrame->bindings = makeNull();

    Value *letBindings = car(args);
    Value *body = cdr(args);

    Value *current = letBindings;
    while(current->type != NULL_TYPE) {
        Value *binding = car(current);
        if(length(binding) == 2 && car(binding)->type == SYMBOL_TYPE) {
            if (symbolDefined(car(binding), newFrame->bindings)) {
                printf("let: duplicate identifier in: %s", car(binding)->s);
                texit(1);
            }
            if (car(cdr(binding))->type == SYMBOL_TYPE || car(cdr(binding))->type == CONS_TYPE) {
                Value *test = cons(eval(car(cdr(binding)), frame), makeNull());
                binding = cons(car(binding), test);
            }
            newFrame->bindings = cons(binding, newFrame->bindings);
            current = cdr(current);
        }
        else {
            printf("let: bad syntax (not an identifier)");
            texit(1);
        }
    }
    Value *result = makeNull();
    while (body->type != NULL_TYPE) {
        result = eval(car(body), newFrame);
        body = cdr(body);
    }
    return result;
}

// Evaluates arguments of a when statement
// If true, evaluates all and returns the last
// If false, returns null
Value *evalWhen(Value* args, Frame *frame) {
    Value *condition = eval(car(args), frame);
    if(condition->type != BOOL_TYPE) {
        printf("Evaluation Error");
        texit(1);
    }
    Value *result = makeNull();
    if(!strcmp(condition->s, "#f")) {
        return result;

    }
    Value *whenArgs = cdr(args);
    while (whenArgs->type != NULL_TYPE) {
        result = eval(car(whenArgs),  frame);
        whenArgs = cdr(whenArgs);
    }
    return result;
}

// Evaluates arguments of an unless statement
// If false, evaluates all and returns the last
// If true, returns null
Value *evalUnless(Value* args, Frame *frame) {
    Value *condition = eval(car(args), frame);
    if(condition->type != BOOL_TYPE) {
        printf("Evaluation Error");
        texit(1);
    }
    Value *result = makeNull();
    if(!strcmp(condition->s, "#t")) {
        return result;

    }
    Value *whenArgs = cdr(args);
    while (whenArgs->type != NULL_TYPE) {
        result = eval(car(whenArgs),  frame);
        whenArgs = cdr(whenArgs);
    }
    return result;
}

// Evaluates argument of a display statement
// Returns the value to be displayed
Value *evalDisplay(Value *args, Frame *frame) {
    if(length(args) != 1) {
        printf("display: arity mismatch;\n"
               " the expected number of arguments does not match the given number.");
        texit(1);
    }
    Value *arg = car(args);
    Value *new = makeNull();
    new->type = VOID_TYPE;
    if (arg->type == STR_TYPE) {
        char *newString = talloc(sizeof(char[255]));
        newString[0] = '\0';
        if (arg->s[1] == '"') {
            newString = "\0";
        }
        else {
            int current = 1;
            while (arg->s[current] != '"') {
                newString[current-1] = arg->s[current];
                ++current;
            }
            newString[current-1] = '\0';
        }
        printf("%s", newString);
        return new;
    }
    else {
        Value *result = eval(arg, frame);
        if (result->type == STR_TYPE) {
            result = evalDisplay(cons(result, makeNull()), frame);
        }
        printTree(result);
        return new;
    }
}

// Evaluates argument of a lambda function
// Returns a closure with the proper code
Value *evalLambda(Value *args, Frame *frame){
    Value *closure = makeNull();
    closure->type = CLOSURE_TYPE;
    closure->cl.frame = frame;

    Value *params = car(args);
    Value *current = params;
    while(current->type != NULL_TYPE) {
        if(car(current)->type != SYMBOL_TYPE) {
            printf("lambda: not an identifier");
            texit(1);
        }
        current = cdr(current);
    }
    closure->cl.paramNames = params;

    Value *body = cdr(args);
    closure->cl.functionCode = body;
    return closure;
}

// Evaluates body of a define function
// Returns void_type Value*, creates binding in current frame
Value *evalDefine(Value *args, Frame *frame) {
    Value *v = makeNull();
    v->type = VOID_TYPE;
    if(length(args) < 2) {
        printf("define: bad syntax");
        texit(1);
    }
    else if(length(args) > 2) {
        printf("define: bad syntax (multiple expressions after identifier)");
        texit(1);
    }
    Value *var = car(args);
    Value *expr = car(cdr(args));

    if (var->type == CONS_TYPE) {
        Value *first = car(var);
        if(first->type != SYMBOL_TYPE) {
            printf("define: bad syntax (not an identifier for procedure name, and not a nested procedure form)");
            texit(1);
        }
        Value *closure1 = evalLambda(cons(cdr(var), cdr(args)), frame);
        Value *binding = makeNull();
        binding = cons(first, cons(closure1, binding));
        frame->bindings = cons(binding, frame->bindings);
        return v;
    }

    if(var->type != SYMBOL_TYPE) {
        printf("define: not an identifier for procedure argument");
        texit(1);
    }
    Value *binding = makeNull();
    binding = cons(var, cons(eval(expr, frame), binding));
    frame->bindings = cons(binding, frame->bindings);
    return v;
}

// Applies the code of a closure to given arguments
Value *apply(Value *function, Value *args) {
    if(function->type != CLOSURE_TYPE) {
        printf("application: not a procedure;\n"
               " expected a procedure that can be applied to arguments");
        texit(1);
    }

    Frame *frame = talloc(sizeof(Frame));
    frame->parent = function->cl.frame;
    frame->bindings = makeNull();

    Value *currentParam = function->cl.paramNames;
    Value *currentArg = args;
    if (length(currentParam) != length(currentArg)) {
        printf("#<procedure>: arity mismatch;\n"
               " the expected number of arguments does not match the given number\n");
        texit(1);
    }
    while(currentParam->type != NULL_TYPE) {
        Value *binding = makeNull();
        binding = cons(car(currentParam), cons(car(currentArg), binding));
        frame->bindings = cons(binding, frame->bindings);
        currentParam = cdr(currentParam);
        currentArg = cdr(currentArg);
    }
    Value *current = function->cl.functionCode;
    Value *result = makeNull();
    while(current->type != NULL_TYPE){
        result = eval(car(current), frame);
        current = cdr(current);
    }
    return result;
}

// Evaluates a each argument in a list of arguments.
Value *evalEach(Value *args, Frame *frame) {
    Value *current = args;
    Value *newArgs = makeNull();
    while (current->type != NULL_TYPE) {
        Value *result = eval(car(current), frame);
        newArgs = cons(result, newArgs);
        current = cdr(current);
    }
    newArgs = reverse(newArgs);
    return newArgs;
}

// Calls evaluation on the parse tree,
// Prints out each evaluation to a new line.
void interpret(Value *tree) {
    Frame *global = talloc(sizeof(Frame));
    global->bindings = makeNull();
    global->parent = NULL;
    Value *current = tree;
    while(current->type != NULL_TYPE) {
        Value *result = eval(car(current), global);
        if (result->type != VOID_TYPE) {
            printTree(result);
            printf("\n");
        }
        current = cdr(current);
    }
}

// Evaluates the parse tree returned by our parser, token by token.
Value *eval(Value *expr, Frame *frame) {
    Value *newTree = makeNull();
    switch(expr->type) {
        case INT_TYPE: {
            return expr;
        }
        case DOUBLE_TYPE: {
            return expr;
        }
        case BOOL_TYPE: {
            return expr;
        }
        case STR_TYPE: {
            return expr;
        }
        case SYMBOL_TYPE: {
            return lookUpSymbol(expr, frame);
        }
        case CONS_TYPE: {
            Value *first = car(expr);
            Value *args = cdr(expr);

            // Error checking
            if (first->type != SYMBOL_TYPE && first->type != CLOSURE_TYPE) {
                printf("application: not a procedure;\n"
                       " expected a procedure that can be applied to arguments");
                texit(1);
            }

            Value *result;
            if (!strcmp(first->s,"if")) {
                result = evalIf(args,frame);
                return result;
            }
            else if (!strcmp(first->s,"let")) {
                result = evalLet(args,frame);
                return result;
            }

            else if (!strcmp(first->s,"display")) {
                result = evalDisplay(cdr(expr), frame);
                return result;
            }

            else if (!strcmp(first->s,"when")) {
                result = evalWhen(cdr(expr), frame);
                return result;
            }

            else if (!strcmp(first->s,"unless")) {
                result = evalUnless(cdr(expr), frame);
                return result;
            }

            else if (!strcmp(first->s,"quote")) {
                return cdr(expr);
            }

            else if (!strcmp(first->s,"define")) {
                return evalDefine(cdr(expr), frame);
            }

            else if (!strcmp(first->s,"lambda")) {
                return evalLambda(cdr(expr), frame);
            }

            else {

                // If not a special form, evaluate the first, evaluate the args, then
                // apply the first to the args.
                Value *evaledOperator = eval(first, frame);
                Value *evaledArgs = evalEach(args, frame);
                return apply(evaledOperator,evaledArgs);
            }
        }
        default:
            break;
    }
    return newTree;
}
