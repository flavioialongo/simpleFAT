#include <iso646.h>
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

    char sep[2]=".\n";
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
    token = strtok(NULL, sep);
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
        printf("An error occurred while creating new directory.\n");
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
        printf("An error occurred while opening file.\n");
        return;
    }

    int ret;
    int starting_point = atoi(argv[1]);
    ret = seek_file(fh, starting_point);
    if (ret) {
        printf("An error occurred while seeking file.\n");
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
        printf("An error occurred while writing file.\n");
        free((void*)filename);
        free((void*)ext);
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
        printf("An error occurred while opening file.\n");
        free((void*)filename);
        free((void*)ext); 
        return;
    }
    int ret;
    int starting_point = atoi(argv[1]);
    ret = seek_file(fh, starting_point);
    if (ret) {
        printf("An error occurred while seeking.\n");
        free((void*)filename);
        free((void*)ext);
        close_file(fh);
        return;
    }
    int size = fh->entry->size;
    char *buffer = malloc(sizeof(char)*size);
    ret = read_file(fh, buffer);
    if (ret!=size) {
        printf("An error occurred while reading from file.\n");
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
        fprintf(stderr, "An error occurred while creating new file.\n");
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
        printf("An error occurred while changing directory.\n");
    
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
        printf("An error occurred while removing.\n");
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
    erase_dir(argv[1], true);
}

void copy_file_sh(int argc, char* argv[MAX_ARGUMENTS_NUM+1]){

    if(argc!=2){
        printf("Usage: copy <filename>\n");
        return;
    }
    FILE* file = fopen(argv[1], "r");
    if(file==NULL){
        printf("An error occurred while opening file %s\n", argv[1]);
        return;
    }
    int res = fseek(file, 0, SEEK_END);
    if(res == -1) {
        printf("An error occurred while retrieving file stats\n");
        return;
    }
    int filesize = ftell(file);
    if(filesize == -1) {
        printf("An error occurred while retrieving file stats\n");
        return;
    }
    res = fseek(file, 0, SEEK_SET);
    if(res == -1) {
        printf("An error occurred while retrieving file stats\n");
        return;
    }
    char * buffer = malloc(sizeof(char)*filesize);
    char new_file_name[1024];
    printf("Insert the filename for the new filesystem: ");
    fgets(new_file_name, 1024, stdin);
    int bytes_read=0;
    while(bytes_read<filesize){
        bytes_read+=fread(buffer, 1, filesize, file);
    }




    char *fn;
    char* fe;
    parse_filename(new_file_name, &fn, &fe);
    res = create_file(fn, fe, filesize, buffer);

    if(res==-1) {
        printf("An error occurred while creating the file in the filesystem\n");
        free((void*)fn);
        free((void*)fe);
        return;
    }
    free((void*)fn);
    free((void*)fe);
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
    printf("copy: copies a file from your operating system into the created filesystem.\n");
    printf("space: prints how much space is used in KBytes");
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

        if(strcmp(argv[0], "copy")==0){
            argv[1] = strtok(NULL, "\n");
            if(argv[1]!=NULL) argc++;
        }else{
            argv[1] = strtok(NULL, " \n");
            if(argv[1]!=NULL) argc++;
            argv[2] = strtok(NULL, "\n");
            if(argv[2]!=NULL) argc++;
        }

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
        }else if(strcmp(argv[0], "copy")==0){
            copy_file_sh(argc, argv);
        }else if(strcmp(argv[0], "space")==0){
            print_used_space();
        }
        else if (strcmp(argv[0], "help") == 0) {
            help(argc, argv); 
        }
        else if (strcmp(argv[0], "exit") == 0) {
            printf("Goodbye!\n");
            if(fat_close()){
                printf("An error occurred while closing the mmap\n");
                continue;
            }
            exit(EXIT_SUCCESS);
        }
        else {
            printf("Invalid command\n");
        }
    } while(1);
}


int main(int argc, char * argv[]) {
 
    int res = fat_initialize("disk.img");
    
    if (res) {
        fprintf(stderr, "[ERROR] Cannot initialize file system.\n");
        exit(EXIT_FAILURE);
    }
    strcpy(current_dir, get_current_directory()->filename);

    printf("Welcome, run 'help' to see available commands!\n");
    do_command_loop();
    return 0;
}
