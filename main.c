#include "filesystem.h"

int main(int argc, char** argv) {

    if(argc<2){
        printf("Usage: ./myfat <path-to-img-file>\n");
        return -1;
    }

    char* imgpath = argv[1];
    // Initialize disk
    fat_initialize(imgpath);

    create_directory("test");
    change_directory("test");
    create_file("test", "txt", 5, "ciao");

    change_directory("..");
    erase_dir("test", true);

}