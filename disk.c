#include "disk.h"

void disk_initialize(Disk *disk){
    memset(disk, 0, sizeof(Disk));    
    printf("Disk initialized\n");
}