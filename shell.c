#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "filesystem.h"

#define MAX_COMMAND_LENGTH 256
#define MAX_ARGUMENTS_NUM  3
#define MAX_INPUT_SIZE     2048

// GLOBAL VARIABLES
char command[MAX_COMMAND_LENGTH];
char current_dir[8];



void parse_filename(char* argv, char **filename, char **ext){

    char sep[2]=".";
    char *fn = malloc(sizeof(char)*8);
    char *fe = malloc(sizeof(char)*3);

    //strtok changes the original string
    //so we create another string to work on
    int argv_len = strlen(argv);
    char argv_temp[argv_len];
    strcpy(argv_temp, argv);
    char *token = strtok(argv_temp, sep);
    if(token != 0){
        strcpy(fn, token);
    }
    token = strtok(NULL, ".");
    if(token != 0){
        strcpy(fe, token);
    }
    *filename = fn;
    *ext = fe;
}
// COMMANDS

/*
 * Creates an empty directory inside the current one.
 */
void mkdir(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: mkdir <dirname>\n");
        return;
    }

    int ret = create_directory(argv[1]);
    if (ret == -1) 
        fprintf(stderr, "An error occurred in creating new directory.\n");
}

/*
 * Writes in an existing file
 */
void my_write(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 3) {
        printf("Usage: write <seek_offset> <filename>\n");
        return;
    }
    char *filename;
    char *ext;
    parse_filename(argv[2], &filename, &ext);
    FileHandle* fh = open_file(filename, ext);
    if (fh == NULL) {
        free((void*)filename);
        free((void*)ext);
        fprintf(stderr, "An error occurred in opening file.\n");
        return;
    }

    int ret;
    int starting_point = atoi(argv[1]);
    ret = seek_file(fh, starting_point);
    if (ret) {
        fprintf(stderr, "An error occurred in seeking in file.\n");
        free((void*)filename);
        free((void*)ext);
        close_file(fh);
        return;
    }

    printf("Enter text: ");
    char* str = calloc(MAX_INPUT_SIZE, sizeof(char));
    fgets(str, MAX_INPUT_SIZE, stdin);
    str[strlen(str) - 1] = 0x00;
    ret = write_file(fh, str);
    if (ret!=strlen(str)+1) {
        fprintf(stderr, "An error occurred in writing in file.\n");
        free((void*)filename);
        free((void*)ext);
        free(ext);
        close_file(fh);
        return;
    }
    free((void*)filename);
    free((void*)ext); 
    free(str);
    close_file(fh);
}

/*
 * Prints the content of the given file
 */
void cat(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 3) {
        printf("Usage: cat <seek_offset> <filename>\n");
        return;
    }

    char *filename;
    char *ext;
    parse_filename(argv[2], &filename, &ext);

    FileHandle* fh = open_file(filename, ext);
    if (fh == NULL) {
        fprintf(stderr, "An error occurred in opening file.\n");
        free((void*)filename);
        free((void*)ext); 
        return;
    }
    int ret;
    int starting_point = atoi(argv[1]);
    ret = seek_file(fh, starting_point);
    if (ret) {
        fprintf(stderr, "An error occurred in seeking in file.\n");
        free((void*)filename);
        free((void*)ext);
        close_file(fh);
        return;
    }
    int size = fh->entry->size;
    char *buffer = malloc(sizeof(char)*size);
    ret = read_file(fh, buffer);
    if (ret!=size) {
        fprintf(stderr, "An error occurred in reading from file.\n");
        free(buffer);
        free((void*)filename);
        free((void*)ext); 
        close_file(fh);
        return;
    }

    printf("%s\n", buffer);

    free(buffer);
    free((void*)filename);
    free((void*)ext); 
    close_file(fh);
}

/*
 * Creates an empty file 
 */
void touch(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: touch <filename>\n");
        return;
    }
    char *filename;
    char *ext;
    parse_filename(argv[1], &filename, &ext);

    int ret = create_file(filename, ext, 0, NULL);
    if (ret){
        fprintf(stderr, "An error occurred in creating new file.\n");
        free((void*)filename);
        free((void*)ext); 
    }
    free((void*)filename);
    free((void*)ext); 
}

/*
 * Sets the current directory to one given
 */
void cd(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: cd <dirname>\n");
        return;
    }
    
    int ret = change_directory(argv[1]);
    if (ret) 
        fprintf(stderr, "An error occurred in changing directory.\n");
    
    strcpy(current_dir, get_current_directory()->filename);
}

/*
 * List the current directory content.
 */
void ls(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 1) {
        printf("Usage: ls\n");
        return;
    }
    list_directory();
}

/*
 * Removes a file or an empty dir from the file system.
 */
void rm(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: rm <filename>\n");
        return;
    }
    char *filename;
    char *ext;
    parse_filename(argv[1], &filename, &ext);
    int ret;
    if(strcmp(ext, "")==0){
        ret = erase_dir(filename, false);
    }else{
        ret = erase_file(filename, ext);
    }
    if (ret){
        free((void*)filename);
        free((void*)ext); 
        fprintf(stderr, "An error occurred in removing.\n");
    }
    free((void*)filename);
    free((void*)ext); 
}

/*
 * Removes a dir even if it is not empty.
 */

void rmf(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {

    if (argc != 2) {
        printf("Usage: rmf <dirname>\n");
        return;
    }
    
    if (strcmp(argv[1], "/") == 0) {
        printf("Maybe another day\n");
        return;
    }

    erase_dir(argv[1], true);
}

void help(int argc, char* argv[MAX_ARGUMENTS_NUM + 1]) {
    
    if (argc != 1) {
        printf("Usage: help\n");
        return;
    }
    printf("Available commands:\n");
    printf("mkdir: create a new directory in the current one.\n");
    printf("write: writes into an existing file.\n");
    printf("cat: prints out the content of an existing file.\n");
    printf("touch: create an empty file in the current directory.\n");
    printf("cd: change the current directory.\n");
    printf("ls: list all the files in the current directory.\n");
    printf("rm: remove a file or an empty directory.\n");
    printf("rmf: remove a file or a not empty directory.\n");
    printf("help: command inception.\n");
    printf("exit: exit the shell.\n");
}


void do_command_loop(void) {

    do {
        char* argv[MAX_ARGUMENTS_NUM + 1] = {NULL};

        printf("%s> ", current_dir);
        fgets(command, MAX_COMMAND_LENGTH, stdin);
        int argc=0;
        argv[0] = strtok(command, " ");
        if(argv[0]!=NULL) argc++;
        argv[1] = strtok(NULL, " ");
        if(argv[1]!=NULL) argc++;
        argv[2] = strtok(NULL, "\n");
        if(argv[2]!=NULL) argc++;

        if (argv[1] == NULL) 
            argv[0][strlen(argv[0]) - 1] = 0x00;

        if (strcmp(argv[0], "mkdir") == 0) {
            mkdir(argc, argv); 
        }
        else if (strcmp(argv[0], "write") == 0) {
            my_write(argc, argv); 
        }
        else if (strcmp(argv[0], "cat") == 0) {
            cat(argc, argv); 
        }
        else if (strcmp(argv[0], "touch") == 0) {
            touch(argc, argv); 
        }
        else if (strcmp(argv[0], "cd") == 0) {
            cd(argc, argv); 
        }
        else if (strcmp(argv[0], "ls") == 0) {
            ls(argc, argv); 
        }
        else if (strcmp(argv[0], "rm") == 0) {
            rm(argc, argv); 
        }
        else if (strcmp(argv[0], "rmf") == 0) {
            rmf(argc, argv); 
        }
        else if (strcmp(argv[0], "help") == 0) {
            help(argc, argv); 
        }
        else if (strcmp(argv[0], "exit") == 0) {
            printf("Cyaa!\n");
            exit(EXIT_SUCCESS);
        }
        else {
            printf("Invalid command\n");
        }
    } while(1);
}


int main(int argc, char * argv[]) {
 
    int res = fat_initialize("disk.img");

    strcpy(current_dir, get_current_directory()->filename);
    
    if (res) {
        fprintf(stderr, "[ERROR] Cannot initialize file system.\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Welcome, run 'help' to see available commands!\n");
    do_command_loop();
    return 0;
}
