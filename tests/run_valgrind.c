#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <talloc.h>

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Invalid number of arguments: supply (only) name of input file");
        texit(1);
    }
    char *inputFileName = argv[1];

    system("hostname");
    char valgrindCommand[2000];
    strcpy(valgrindCommand, "valgrind --leak-check=full ./interpreter");
    strcat(valgrindCommand, " < ../inputfiles/ ");
    strcat(valgrindCommand, inputFileName);
    system(valgrindCommand);
    return 0;
}
