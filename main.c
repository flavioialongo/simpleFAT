#include "filesystem.h"

int main(int argc, char** argv) {

    if(argc<2){
        printf("Usage: ./myfat <path-to-img-file>\n");
        return -1;
    }

    char* imgpath = argv[1];
    // Initialize disk
    int res = fat_initialize(imgpath);
    if(res){
        printf("Cannot initialize FAT image\n");
        return -1;
    }
    // Create a file
    const char fileData1[] = "Hello, FAT32(1)!";
    create_file("HELLO", "TXT", sizeof(fileData1), (uint8_t*)fileData1);  
    const char fileData2[] = "Hello, FAT32!(2)";
    create_file("CIAO", "JPG", sizeof(fileData2), (uint8_t*)fileData2);  
    const char fileData3[] = "Hello, FAT132!(3)";
    create_file("HOLA", "IMG", sizeof(fileData3), (uint8_t*)fileData3);  
    print_root_content();

    char *buffer = malloc(sizeof(char)*(strlen(fileData1)+1));

    res = read_file("HELLO", buffer);
    if(res<sizeof(fileData1)){
        printf("Invalid read\n");
        free(buffer);
        return -1;
    }
    
    printf("READ: %s\n", buffer);
    free(buffer);
    return 0;
}