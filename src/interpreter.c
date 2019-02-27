#include <stdio.h>
#include <string.h>
#include "value.h"
#include "interpreter.h"
#include "talloc.h"
#include "assert.h"
#include "linkedlist.h"
#include "tokenizer.h"
#include "parser.h"

Value *lookUpSymbol(Value *symbol, Frame *frame) {
    Value *current = frame->bindings;
    while (current->type != NULL_TYPE) {
        if (!strcmp(car(car(current))->s, symbol->s)) {
            return car(cdr(car(current)));
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
}

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

Value *evalLet(Value* args, Frame *frame) {
    Frame *newFrame = talloc(sizeof(Frame));
    newFrame->parent = frame;
    newFrame->bindings = makeNull();

    Value *letBindings = car(args);
    Value *body = car(cdr(args));

    Value *current = letBindings;
    while(current->type != NULL_TYPE) {
        Value *binding = car(current);
        if(length(binding) == 2 && car(binding)->type == SYMBOL_TYPE) {
            if (symbolDefined(car(binding), newFrame->bindings)) {
                printf("let: duplicate identifier in: %s", car(binding)->s);
                texit(1);
            }
            if (car(cdr(binding))->type == SYMBOL_TYPE) {
                binding = cons(car(binding), lookUpSymbol(car(cdr(binding)), frame));
            }
            newFrame->bindings = cons(binding, newFrame->bindings);
            current = cdr(current);
        }
        else {
            printf("let: bad syntax (not an identifier)");
            texit(1);
        }
    }

    return eval(body, newFrame);
}

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

Value *evalDisplay(Value *args, Frame *frame) {
    if(length(args) != 1) {
        printf("display: arity mismatch;\n"
               " the expected number of arguments does not match the given number.");
        texit(1);
    }
    Value *arg = car(args);
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
        Value *new = makeNull();
        new->type = STR_TYPE;
        new->s = newString;
        return new;
    }
    else {
        return eval(arg, frame);
    }
//    else if (arg->type == CONS_TYPE) {
//        Value *new = makeNull();
//        Value *current = arg;
//        while (current->type != NULL_TYPE) {
//            Value *newCell = cons(car(current), makeNull());
//            Value *s = evalDisplay(newCell, frame);
//            new = cons(s, new);
//            current = cdr(current);
//        }
//        new = reverse(new);
//        return new;
//    }
    //return eval(car(args), frame);
}

void interpret(Value *tree) {
    Frame *global = talloc(sizeof(Frame));
    global->bindings = makeNull();
    global->parent = NULL;
    Value *current = tree;
    while(current->type != NULL_TYPE) {
        Value *result = eval(car(current), global);
        printTree(result);
        printf("\n");
        current = cdr(current);
    }
}
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
            if (first->type != SYMBOL_TYPE) {
                printf("application: not a procedure;\n"
                       " expected a procedure that can be applied to arguments");
                texit(1);
            }

            Value *result = makeNull();
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
                result = cdr(expr);
                return result;
            }

            else {
                printf("application: not a procedure;\n"
                       " expected a procedure that can be applied to arguments\n"
                       "given: %s", first->s);
                texit(1);
            }

            return newTree;
        }
        default:
            break;
    }
    return newTree;
}
