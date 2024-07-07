#include "filesystem.h"

int main(int argc, char** argv) {

    if(argc<2){
        printf("Usage: ./myfat <path-to-img-file>\n");
        return -1;
    }

    char* imgpath = argv[1];
    // Initialize disk
    int res = fat_initialize(imgpath);
    if(res) goto cleanup;

    res = create_directory("TEST");
    if(res) goto cleanup;

    res = change_directory("TEST");
    if(res) goto cleanup;
    char file_content[12]="helloworld1";

    res = create_file("HELLO", "TXT", 0, NULL);
    if(res) goto cleanup;
    res = create_file("HELLO2", "IMG", 12, file_content);
    if(res)goto cleanup;

    list_directory();

    FileHandle* hello1 = open_file("HELLO", "TXT");
    FileHandle* hello2 = open_file("HELLO2", "IMG");

    if(hello1 == NULL) {
        printf("Open 1 failed\n");
    }
    if(hello2 == NULL) {
        printf("Open 2 failed\n");
    }

    char * buffer1 = malloc(sizeof(char)*sizeof(file_content));
    res = read_file(hello2, buffer1);

    printf("READ 1: %s\n", buffer1);

    res = write_file(hello1, file_content);

    res = read_file(hello1, buffer1);

    printf("READ 2: %s\n", buffer1);

    res = seek_file(hello1, 3);

    res = read_file(hello1, buffer1);
    printf("READ 2: %s\n", buffer1);

    res = write_file(hello1, file_content);
    char * buffer2 = malloc(sizeof(char)*(sizeof(file_content)+hello1->position));

    res = read_file(hello1, buffer2);
    printf("READ 2: %s\n", buffer2);

    res = seek_file(hello1, 0);


    res = read_file(hello1, buffer2);
    printf("READ 2: %s\n", buffer2);



    char test[1024];
    memset(test, 'b', 1022);
    test[1023]='\0';

    res = create_file("HELLO3", "TXT", 0, NULL);
    if(res) goto cleanup;

    FileHandle *hello3 = open_file("HELLO3", "TXT");

    res = write_file(hello3, test);
    char * buffer3 = malloc(1024*sizeof(char));
    res = read_file(hello3, buffer3);

    printf("READ 3: %s\n", buffer3);


    res = erase_file("HELLO3", "TXT");
    printf("TEST: %d\n", free_cluster_index());

    list_directory();



cleanup:
    if(res == DIRCREATERROR){
        printf("Cannot create new directory in %s\n", get_current_directory()->filename);
        return -1;
    }
    if(res == FILECREATERROR){
        printf("Cannot create file in directory %s \n", get_current_directory()->filename);
        return -1;
    }
    if(res == FILENOTFOUND){
        printf("File/Directory not found in directory %s \n", get_current_directory()->filename);
    }
    if(res == INITERROR){
        printf("Initialize error\n");
        return -1;
    }


    /*
    char *buffer = malloc(sizeof(char)*(strlen(file1)+1));


    res = read_file("HELLO", buffer);
    if(res<sizeof(file1)){
        printf("Invalid read\n");
        free(buffer);
        return -1;
    }
    
    printf("READ: %s\n", buffer);
    
    free(buffer);
    */
    return 0;
}