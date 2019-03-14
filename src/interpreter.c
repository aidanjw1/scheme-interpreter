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
    Value *currentArg = car(args);
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
    newArgs = cons(newArgs, makeNull());
    return newArgs;
}

// Bind a string to a primitive function.
void bind(char *name, Value *(*function)(struct Value *), Frame *frame) {
    Value *val = talloc(sizeof(Value));
    val->type = PRIMITIVE_TYPE;
    val->pf = function;
    Value *symbol = talloc(sizeof(Value));
    symbol->type = SYMBOL_TYPE;
    symbol->s = name;
    Value *binding = cons(symbol, cons(val, makeNull()));
    frame->bindings = cons(binding, frame->bindings);
}

// Primitive function for adding numbers.
Value *primitiveAdd(Value *args) {
    int resulti = 0;
    double resultd = 0;
    Value *current = car(args);
    Value *value = makeNull();
    value->type = INT_TYPE;
    while (current->type != NULL_TYPE) {
        if (car(current)->type == INT_TYPE) {
            resulti += car(current)->i;
        }
        else if (car(current)->type == DOUBLE_TYPE) {
            resultd += car(current)->d;
            value->type = DOUBLE_TYPE;
        }
        else {
            printf("+: contract violation\n"
                   "  expected: number?");
        }
        current = cdr(current);
    }
    if (value->type == DOUBLE_TYPE) {
        value->d = resulti + resultd;
    }
    else {
        value->i = resulti;
    }
    return value;
}

// Primitive function for multiplying numbers.
Value *primitiveMult(Value *args) {
    int resulti = 1;
    double resultd = 1;
    Value *current = car(args);
    Value *value = makeNull();
    value->type = INT_TYPE;
    while (current->type != NULL_TYPE) {
        if (car(current)->type == INT_TYPE) {
            resulti *= car(current)->i;
        }
        else if (car(current)->type == DOUBLE_TYPE) {
            resultd *= car(current)->d;
            value->type = DOUBLE_TYPE;
        }
        else {
            printf("*: contract violation\n"
                   "  expected: number?");
        }
        current = cdr(current);
    }
    if (value->type == DOUBLE_TYPE) {
        value->d = resulti * resultd;
    }
    else {
        value->i = resulti;
    }
    return value;

}

// Primitive function for dividing two numbers.
Value *primitiveDivide(Value *args) {
    args = car(args);
    Value *value = makeNull();
    value->type = INT_TYPE;
    Value *first = makeNull();
    Value *second = makeNull();
    if (length(args) == 1) {
        first->type = INT_TYPE;
        first->i = 1;
        second = car(args);
    }
    else if (length(args) == 0) {
        printf("/: arity mismatch;\n"
               " the expected number of arguments does not match the given number\n"
               "  expected: at least 1");
        texit(1);
    }
    else if (length(args) >2) {
        printf("/: arity mismatch;\n"
               " the expected number of arguments does not match the given number\n"
               "  expected: 2 or less");
        texit(1);
    }
    else {
        first = car(args);
        second = car(cdr(args));
    }
    if ((second->type == INT_TYPE && second->i == 0) || (second->type == DOUBLE_TYPE && second->d == 0)) {
        printf("/: division by zero");
        texit(1);
    }
    if (first->type == INT_TYPE && second->type == INT_TYPE) {
        if (first->i % second->i == 0) {
            value->i = first->i/second->i;
        }
        else {
            value->type = DOUBLE_TYPE;
            value->d = (float)first->i/(float)second->i;
        }
    }
    else {
        if (first->type == INT_TYPE && second->type == DOUBLE_TYPE) {
            value->type = DOUBLE_TYPE;
            value->d = (float)first->i/second->d;
        }
        else if (first->type == DOUBLE_TYPE && second->type == INT_TYPE) {
            value->type = DOUBLE_TYPE;
            value->d = first->d/(float)second->i;
        }
        else {
            value->type = DOUBLE_TYPE;
            value->d = first->d/second->d;
        }
    }
    return value;
}
// Primitive function for checking if something is nothing (whaaa? ¯\_(ツ)_/¯)
Value *primitiveNull(Value *args) {
    args = car(args);
    if (length(args) != 1) {
        printf("null?: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    Value *value = makeNull();
    value->type = BOOL_TYPE;
    value->s = "#t";
    if (car(args)->type != NULL_TYPE) {
        value->s = "#f";
    }
    return value;
}

// Primitive function for getting the car of a cons cell.
Value *primitiveCar(Value *args) {
    args = car(args);
    if(length(args) != 1) {
        printf("car: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    if(car(args)->type != CONS_TYPE) {
        printf("car: contract violation\n"
               "  expected: pair?");
        texit(1);
    }
    Value *lst = car(args);
    return car(lst);
}

// Primitive function for getting the cdr of a cons cell.
Value *primitiveCdr(Value *args) {
    args = car(args);
    if(length(args) != 1) {
        printf("cdr: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    if(car(args)->type != CONS_TYPE) {
        printf("cdr: contract violation\n"
               "  expected: pair?");
        texit(1);
    }
    Value *lst = car(args);
    return cdr(lst);
}

// Primitive function for creating a cons cell from two arguments.
Value *primitiveCons(Value *args) {
    args = car(args);
    if(length(args) != 2) {
        printf("cons: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    Value *consCell = cons(car(args), car(cdr(args)));
    return consCell;
}

// Primitive function for creating a list from the arguments.
Value *primitiveList(Value *args) {
    args = car(args);
    return args;
}

// Primitive function for appending values to a list.
Value *primitiveAppend(Value *args) {
    args = car(args);
    if (length(args) == 0) {
        return makeNull();
    }
    else if (length(args) == 1) {
        return car(args);
    }

    Value *newList = makeNull();
    Value *current = args;
    while (current->type != NULL_TYPE) {
        if(car(current)->type != CONS_TYPE && cdr(current)->type != NULL_TYPE) {
            printf("append: contract violation\n"
                   "  expected: list?");
            texit(1);
        }
        if(car(current)->type == CONS_TYPE) {
            Value *innerCurrent = car(current);
            while(innerCurrent->type != NULL_TYPE) {
                newList = cons(car(innerCurrent), newList);
                innerCurrent = cdr(innerCurrent);
            }
        }
        else {
            newList = cons(car(current), newList);
        }
        current = cdr(current);
    }
    newList = reverse(newList);
    return newList;

}

// Primitive function for eq, return #t if v1 and v2 refer to the same object, #f otherwise.
Value *primitiveEq(Value *args) {
    args = car(args);
    if(length(args) != 2) {
        printf("eq?: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    Value *first = car(args);
    Value *second = car(cdr(args));
    Value *valueT = makeNull();
    valueT->type = BOOL_TYPE;
    valueT->s = "#t";
    Value *valueF = makeNull();
    valueF->type = BOOL_TYPE;
    valueF->s = "#f";

    if (first != second) {
        return valueF;
    }
    return valueT;
}

// Primitive function for equality, compares if values of two Values are equal
Value *primitiveEqual(Value *args) {
    args = car(args);
    if(length(args) != 2) {
        printf("equal?: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }
    Value *first = car(args);
    Value *second = car(cdr(args));
    Value *valueT = makeNull();
    valueT->type = BOOL_TYPE;
    valueT->s = "#t";
    Value *valueF = makeNull();
    valueF->type = BOOL_TYPE;
    valueF->s = "#f";

    if (first->type == second->type) {
        switch(first->type) {
            case INT_TYPE: {
                if (first->i == second->i) {
                    return valueT;
                }
                return valueF;
            }
            case DOUBLE_TYPE: {
                if (first->d == second->d) {
                    return valueT;
                }
                return valueF;
            }
            case CONS_TYPE: {
                Value *currentFirst = first;
                Value *currentSecond = second;
                while(currentFirst->type != NULL_TYPE) {
                    Value *newArgs = cons(cons(car(currentFirst), cons(car(currentSecond), makeNull())), makeNull());
                    if(!(strcmp(primitiveEqual(newArgs)->s, "#f"))) {
                        return valueF;
                    }
                    currentFirst = cdr(currentFirst);
                    currentSecond = cdr(currentSecond);
                }
                return valueT;
            }
            case NULL_TYPE: {
                return valueT;
            }
            default: {
                if(!strcmp(first->s, second->s)) {
                    return valueT;
                }
                return valueF;
            }
        }
    }
    return valueF;
}

// Primitive function for >. Compare integer values.
Value *primitiveGreaterThan(Value *args) {
    args = car(args);
    Value *valueT = makeNull();
    valueT->type = BOOL_TYPE;
    valueT->s = "#t";
    Value *valueF = makeNull();
    valueF->type = BOOL_TYPE;
    valueF->s = "#f";

    if(length(args) == 0) {
        printf(">: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }

    Value *current = args;
    int previous = car(current)->i;
    while(current->type != NULL_TYPE) {
        if(car(current)->type != INT_TYPE) {
            printf(">: contract violation\n"
                   "  expected: number?");
            texit(1);
        }
        if(car(current)->i >= previous) {
            return valueF;
        }
        previous = car(current)->i;
        current = cdr(current);
    }
    return valueT;
}

// Primitive function for <. Compare integer values
Value *primitiveLessThan(Value *args) {
    args = car(args);
    Value *valueT = makeNull();
    valueT->type = BOOL_TYPE;
    valueT->s = "#t";
    Value *valueF = makeNull();
    valueF->type = BOOL_TYPE;
    valueF->s = "#f";

    if(length(args) == 0) {
        printf("<: arity mismatch;\n"
               " the expected number of arguments does not match the given number");
        texit(1);
    }

    Value *current = args;
    int previous = car(current)->i;
    while(current->type != NULL_TYPE) {
        if(car(current)->type != INT_TYPE) {
            printf("<: contract violation\n"
                   "  expected: number?");
            texit(1);
        }
        if(car(current)->i <= previous) {
            return valueF;
        }
        previous = car(current)->i;
        current = cdr(current);
    }
    return valueT;
}


// Calls evaluation on the parse tree,
// Prints out each evaluation to a new line.
void interpret(Value *tree) {
    Frame *global = talloc(sizeof(Frame));
    global->bindings = makeNull();
    global->parent = NULL;
    Value *current = tree;

    bind("+",primitiveAdd,global);
    bind("null?", primitiveNull, global);
    bind("car", primitiveCar, global);
    bind("cdr", primitiveCdr, global);
    bind("cons", primitiveCons, global);
    bind("equal?", primitiveEqual, global);
    bind("eq?", primitiveEq, global);
    bind("append", primitiveAppend, global);
    bind(">", primitiveGreaterThan, global);
    bind("<", primitiveLessThan, global);
    bind("list", primitiveList, global);
    bind("*", primitiveMult, global);
    bind("/", primitiveDivide, global);


    while(current->type != NULL_TYPE) {
        Value *result = eval(car(current), global);
        if (result->type == SINGLE_QUOTE_TYPE) {
            current = cdr(current);
            Value *quote = makeNull();
            quote->type = SYMBOL_TYPE;
            quote->s = "quote";
            result = cons(quote, cons(current, makeNull()));
            printTree(eval(result, global));
        }
        else if (result->type != VOID_TYPE) {
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
        case SINGLE_QUOTE_TYPE: {
            return expr;
        }
        case CONS_TYPE: {
            Value *first = car(expr);
            Value *args = cdr(expr);

            // Error checking
            if (first->type == CONS_TYPE) {
                return apply(eval(first, frame), args);
            }
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
                if (length(args) != 1) {
                    printf("quote: bad syntax");
                    texit(1);
                }
                return car(cdr(expr));
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
                if (evaledOperator->type == PRIMITIVE_TYPE) {
                    return evaledOperator->pf(evaledArgs);
                }
                return apply(evaledOperator,evaledArgs);
            }
        }
        default:
            break;
    }
    return newTree;
}
