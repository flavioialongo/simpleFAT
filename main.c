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
    const char fileData[] = "Hello, FAT12!";
    print_image(image_file, fbs, 10);
    create_file(image_file, fbs, "HELLO", "TXT", sizeof(fileData), (uint8_t*)fileData);  
    print_image(image_file, fbs, 10);
    print_root_content(root_address(image_file, fbs), fbs);


    free(fbs);
    return 0;
}