#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "linkedlist.h"
#include "value.h"
#include "talloc.h"
#include "assert.h"
#include <ctype.h>

FILE *inputFile = NULL;
char misc[] = {'!', '$', '%', '&', '*', '/', ':', '<', '=', '>', '?', '~', '_', '^'};
char brackets_quote[] = {'(', '[', ']', ')', '\''};

// Cycles to the next character in the file, ignores comments.
void nextChar(char *charRead, bool inString) {
    *charRead = (char)fgetc(inputFile);

    if (*charRead == ';' && !inString){
        while (*charRead != (char)10) {
            *charRead = (char)fgetc(inputFile);
            if(*charRead == EOF) return;
        }
        *charRead = (char)fgetc(inputFile);
    }
}

// Concatenate a character onto the end of a string
char *catLetter(char const *string, char const *c) {
    char *new = talloc(sizeof(char[255]));
    int i = 0;
    if (*string == (char)0) {
        new[0] = *c;
        new[1] = '\0';
        return new;
    }
    while(*(string + i) != (char)0) {
        new[i] = string[i];
        i ++;
    }
    new[i] = *c;
    new[i+1] = '\0';
    return new;
}

// Makes Value of corresponding type for single characters only
Value *makeStringValue(char const *s, valueType t) {
    Value *newVal = makeNull();
    newVal->type = t;
    char *new = talloc(sizeof(char[255]));
    new[0] = '\0';
    new = catLetter(new, s);
    newVal->s = new;
    return newVal;
}

// Checks if char is either a parenthesis or quote
bool isParenOrQuote(char const *s) {
    for (int i = 0; i < 5; i++) {
        if (*s == brackets_quote[i]) {
            return true;
        }
    }
    return false;
}


// Creates corresponding bracket Value for bracket char
Value *tokenizeBracket(char charRead) {
    valueType type = NULL_TYPE;
    switch (charRead) {
        case '(':
            type = OPEN_TYPE;
            break;
        case '[':
            type = OPEN_BRACKET_TYPE;
            break;
        case ')':
            type = CLOSE_TYPE;
            break;
        case ']':
            type = CLOSE_BRACKET_TYPE;
            break;
        default:
            break;
    }
    return makeStringValue(&charRead, type);
}

// Creates String type Value
Value *tokenizeString(char *charRead) {
    char* new = talloc(sizeof(char[255]));
    new[0] = '\0';
    new = catLetter(new, charRead);
    nextChar(charRead, true);
    while(*charRead != '"') {
        if (*charRead == EOF) {
            printf("Syntax Error: Missing \"");
            texit(1);
        }
        new = catLetter(new, charRead);
        nextChar(charRead, true);
    }
    new = catLetter(new, charRead);
    Value *stringVal = makeNull();
    stringVal->type = STR_TYPE;
    stringVal->s = new;
    return stringVal;
}

// Creates Int/Double type Value for a number
Value *tokenizeNumber(char *charRead, char sign) {
    valueType type = INT_TYPE;
    char* new = talloc(sizeof(char[255]));
    new[0] = '\0';
    while(*charRead != (char)32) {
        if(*charRead == '.') {
            if (type == DOUBLE_TYPE) {
                printf("Syntax error: Multiple use of dot within number");
                texit(1);
            }
            type = DOUBLE_TYPE;
        }
        else if (isParenOrQuote((charRead)) || *charRead == EOF || *charRead == (char)10 || *charRead == (char)13) {
            break;
        }
        else if (!isdigit(*charRead)) {
            printf("Syntax error: Improper number");
            texit(1);
        }
        new = catLetter(new, charRead);
        nextChar(charRead, false);
    }
    Value *numVal = makeNull();
    numVal->type = type;
    if (type == DOUBLE_TYPE) {
        if (sign == '-') {
            numVal->d = -1*atof(new);
        }
        else {
            numVal->d = atof(new);
        }
    }
    else{
        if (sign == '-') {
            numVal->i = -1*atoi(new);
        }
        else {
            numVal->i = atoi(new);
        }

    }
    return numVal;
}

// Creates Boolean type Value
Value *tokenizeBoolean(char *charRead) {
    char* new = talloc(sizeof(char[255]));
    new[0] = '\0';
    new = catLetter(new, charRead);
    nextChar(charRead, false);
    if(!(*charRead == 'f' || *charRead == 't')) {
        printf("Syntax error: Improper use of #");
        texit(1);
    }
    new = catLetter(new, charRead);
    nextChar(charRead, false);
    if (*charRead == EOF) {
        Value *boolVal = makeNull();
        boolVal->type = BOOL_TYPE;
        boolVal->s = new;
        return boolVal;
    }
    else if(!(isParenOrQuote(charRead) || *charRead == (char)32 || *charRead == EOF
            || *charRead == (char)10 || *charRead == (char)13)) {
        printf("Syntax Error: Missing space after boolean");
        texit(1);
    }
    Value *boolVal = makeNull();
    boolVal->type = BOOL_TYPE;
    boolVal->s = new;
    return boolVal;
}

// Checks if char is a valid <initial> according to grammar
bool isSymbolInitial (char const *charRead) {
    for(int i = 0; i < 14; i++) {
        if(*charRead == misc[i]) return true;
    }
    return (bool)isalpha(*charRead);
}

// Checks if char is a valid <subsequent> according to grammar
bool isSymbolSubsequent(char *s) {
    return (isSymbolInitial(s) || isdigit(*s) || (*s == '+') || (*s == '-') || (*s == '.'));
}

// Creates a Symbol Type Value
Value *tokenizeSymbol(char *charRead) {
    char *str = talloc(sizeof(char*));
    str[0] = '\0';
    while (*charRead != (char)32) {
        if(isParenOrQuote((charRead)) || *charRead == EOF || *charRead == (char)10 || *charRead == '\r') {
            break;
        }
        if (!isSymbolSubsequent(charRead)) {
            printf("Syntax Error: Improper symbol");
            texit(1);
        }
        str = catLetter(str, charRead);
        nextChar(charRead, false);
    }
    Value *symbolVal = makeNull();
    symbolVal->type = SYMBOL_TYPE;
    symbolVal->s = str;
    return symbolVal;
}

// Read all of the input from stdin, and return a linked list consisting of the
// tokens.
Value *tokenize(char *inputFileName) {
    inputFile = fopen(inputFileName, "r");
    char charRead;
    Value *list = makeNull();
    nextChar(&charRead, false);

    while(charRead != EOF) {

        // Brackets
        if (charRead == '(' || charRead == ')' || charRead == '[' || charRead == ']') {
            list = cons(tokenizeBracket(charRead), list);
            nextChar(&charRead, false);
        }

            // Strings
        else if (charRead == '"'){
            list = cons(tokenizeString(&charRead), list);
            nextChar(&charRead, false);

        }
            // Single Quote
        else if (charRead == '\'') {
            list = cons(makeStringValue(&charRead, SINGLE_QUOTE_TYPE), list);
            nextChar(&charRead, false);
        }
            // Symbols + or -
        else if (charRead == '+' || charRead == '-') {
            char sign = charRead;
            nextChar(&charRead, false);
            if(charRead == (char)32 || isParenOrQuote(&charRead)) {
                list = cons(makeStringValue(&sign, SYMBOL_TYPE), list);
            }
            else if (isdigit(charRead)){
                tokenizeNumber(&charRead, sign);
            }
            else {
                printf("Syntax error: Symbol starting with +/-");
                texit(1);
            }
        }

            // Symbols
        else if (isSymbolInitial(&charRead)) {
            list = cons(tokenizeSymbol(&charRead), list);
        }

            // Integers
        else if (isdigit(charRead) || charRead == '+' || charRead == '-') {
            char sign = '+';
            if (charRead == '-') sign = '-';
            list = cons(tokenizeNumber(&charRead, sign), list);
        }
            // Booleans
        else if (charRead == '#') {
            list = cons(tokenizeBoolean(&charRead), list);
        }



            // Dots
        else if (charRead == '.') {
            Value *dotVal = makeStringValue(&charRead, DOT_TYPE);
            nextChar(&charRead, false);
            if(!(charRead == (char)32 || isParenOrQuote(&charRead) || isdigit(charRead) || charRead == '"' || charRead == EOF)) {
                printf("Syntax Error: Dot misplacement");
                texit(1);
            }
            list = cons(dotVal, list);
        }
        else {
            nextChar(&charRead, false);
        }
    }
    fclose(inputFile);
    Value *revList = reverse(list);
    return revList;
}

// Displays the contents of the linked list as tokens, with type information
void displayTokens(Value *list) {
    Value *newList = list;
    while (newList->type == CONS_TYPE) {
        Value *listCar = newList->c.car;
        switch (listCar->type) {
            case INT_TYPE:
                printf("%i:Integer\n", listCar->i);
                break;
            case DOUBLE_TYPE:
                printf("%f:Double\n", listCar->d);
                break;
            case STR_TYPE:
                printf("%s:String\n", listCar->s);
                break;
            case OPEN_TYPE:
                printf("%s:Open\n", listCar->s);
                break;
            case OPEN_BRACKET_TYPE:
                printf("%s:Open Bracket\n", listCar->s);
                break;
            case CLOSE_TYPE:
                printf("%s:Close\n", listCar->s);
                break;
            case CLOSE_BRACKET_TYPE:
                printf("%s:Close Bracket\n", listCar->s);
                break;
            case BOOL_TYPE:
                printf("%s:Boolean\n", listCar->s);
                break;
            case SYMBOL_TYPE:
                printf("%s:Symbol\n", listCar->s);
                break;
            case DOT_TYPE:
                printf("%s:Dot\n", listCar->s);
                break;
            case SINGLE_QUOTE_TYPE:
                printf("%s:Single Quote\n", listCar->s);
            default:
                break;
        }
        newList = cdr(newList);
    }
}

