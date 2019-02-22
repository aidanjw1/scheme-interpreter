#include <stdio.h>
#include <string.h>
#include "tokenizer.h"
#include "value.h"
#include "linkedlist.h"
#include "talloc.h"
#include "parser.h"
#include "interpreter.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Invalid number of arguments: supply (only) name of input file");
        texit(1);
    }
    char *inputFileName = argv[1];
    char fullInputPath[2000];
    strcpy(fullInputPath, "../inputfiles/");
    strcat(fullInputPath, inputFileName);
    printf("Input filename is %s\n", fullInputPath);

    Value *list = tokenize(fullInputPath);
    Value *tree = parse(list);
    interpret(tree);

    tfree();
}
