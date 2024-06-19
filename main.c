#include "filesystem.h"

int main(int argc, char** argv) {

    if(argc<2){
        printf("Usage: ./myfat <path-to-img-file>\n");
        return -1;
    }

    char* imgpath = argv[1];


    FATBS *fbs = malloc(sizeof(FATBS));
    // Initialize disk
    uint8_t* image_file = fat_initialize(imgpath, fbs);

    // Create a file
    const char fileData1[] = "Hello, FAT32(1)!";
    create_file(image_file, fbs, "HELLO", "TXT", sizeof(fileData1), (uint8_t*)fileData1);  
    const char fileData2[] = "Hello, FAT32!(2)";
    create_file(image_file, fbs, "CIAO", "JPG", sizeof(fileData2), (uint8_t*)fileData2);  
    const char fileData3[] = "Hello, FAT132!(3)";
    create_file(image_file, fbs, "HOLA", "IMG", sizeof(fileData3), (uint8_t*)fileData3);  
    print_root_content(root_address(image_file, fbs), fbs);

    char *buffer = malloc(sizeof(char)*(strlen(fileData1)+1));

    int res = read_file(image_file, fbs, "HELLO", buffer);
    if(res<sizeof(fileData1)){
        printf("Invalid read\n");
        free(fbs);
        free(buffer);
        return -1;
    }
    
    printf("READ: %s\n", buffer);

    free(fbs);
    free(buffer);
    return 0;
}