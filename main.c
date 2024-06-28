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
    const char file1[] = "Hello, FAT32(1)!";
    res = create_file("HELLO", "TXT", sizeof(file1), (char*)file1);  
    if(res) goto cleanup;
    
    list_directory();


    res = create_directory("TEST");
    if(res) goto cleanup;
    
    res = change_directory("TEST");
    if(res) goto cleanup;

    res = create_file("TESTFI1", "TXT", sizeof(file1), (char*)file1);  
    if(res) goto cleanup;
    
    res = create_file("TESTFI3", "IMG", sizeof(file1), (char*)file1);  
    if(res) goto cleanup;

    res = create_file("TESTFI2", "JPG", sizeof(file1), (char*)file1);  
    if(res) goto cleanup;

    res = create_directory("TESTDIR");  
    if(res) goto cleanup;

    list_directory();


    res = change_directory("TESTDIR");  
    if(res) goto cleanup;
    
    res = create_file("REM", "TXT", sizeof(file1), (char*)file1);
    if(res) goto cleanup;

    list_directory();

    res = erase_file("REM");
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
    if(res == CDERROR){
        printf("No directory found with that name\n");
        return -1;
    }
    if(res == INITERROR){
        printf("Initialize error\n");
        return -1;
    }
    if(res == FILEDELERROR){
        printf("No file found with that name\n");
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