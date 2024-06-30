#include "filesystem.h"

int main(int argc, char** argv) {

    if(argc<2){
        printf("Usage: ./myfat <path-to-img-file>\n");
        return -1;
    }

    FATBS* fbs = get_fbs();
    printf("FBS SIZE: %lu\n", sizeof(*fbs));

    char* imgpath = argv[1];
    // Initialize disk
    int res = fat_initialize(imgpath);
    if(res) goto cleanup;

    res = create_directory("TEST");
    if(res) goto cleanup;

    res = change_directory("TEST");
    if(res) goto cleanup;
    char file[12]="helloworld1";

    res = create_file("HELLO", "TXT", 12, file);
    if(res) goto cleanup;

    list_directory();

    res = create_file("HELLO2", "IMG", 12, file);
    if(res)goto cleanup;

    res = create_directory("DELME");
    if(res)goto cleanup;

    res = change_directory("DELME");
    if(res) goto cleanup;

    res = create_file("HELLO3", "IMG", 12, file);
    if(res)goto cleanup;
    res = create_file("HELLO4", "IMG", 12, file);
    if(res)goto cleanup;

    res = change_directory("..");
    if(res) goto cleanup;

    list_directory();

    res = change_directory("..");
    if(res) goto cleanup;

    res = erase_dir("TEST", 1);
    if(res) goto cleanup;

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