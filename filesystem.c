#include "filesystem.h"



uint8_t *root_address(uint8_t* image_file, FATBS *fbs){
    return image_file + fbs->root_size;
}


uint8_t *data_address(uint8_t* image_file, FATBS *fbs){
    return image_file + fbs->fat_size+fbs->root_size;
}

uint8_t* fat_initialize(const char* image_path, FATBS *fbs){
    
    fbs->bytes_per_sector = SECTOR_SIZE;
    fbs->root_entry_count = ROOT_DIR_ENTRIES;
    fbs->sectors_per_cluster = CLUSTER_SIZE;
    fbs->fat_entries = FAT_ENTRIES;
    fbs->total_sectors = TOTAL_SECTORS;
    fbs->fat_size = fbs->fat_entries*3/2; // FAT12 entries are 1.5 (3/2) bytes each
    fbs->fat_sectors = fbs->fat_size/fbs->bytes_per_sector;
    fbs->root_sectors = (fbs->root_entry_count*32)/fbs->bytes_per_sector;
    fbs->root_size = fbs->root_sectors * fbs->bytes_per_sector;
    fbs->data_sectors = (fbs->total_sectors-(fbs->fat_sectors+fbs->root_sectors));
    fbs->data_size = fbs->data_sectors*fbs->bytes_per_sector;


    int fd = open(image_path, O_RDWR | O_CREAT | O_TRUNC, 0666);



    if(fd==-1){
        printf("Error creating or opening the image file\n");
        return NULL;
    }
    if(ftruncate(fd, fbs->total_sectors*fbs->bytes_per_sector)){
        printf("Error creating the image file\n");
        return NULL;
    }


    uint8_t *image_file = mmap(NULL, fbs->total_sectors*fbs->bytes_per_sector, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(image_file == MAP_FAILED){
        printf("Cannot mmap the image file\n");
        return NULL;
    }

    uint8_t *root = root_address(image_file, fbs);
    uint8_t *data = data_address(image_file, fbs);

    //initialize fat
    memset(image_file, 0x00, fbs->fat_size);

    //initialize root
    memset(root, 0x00, fbs->root_size);

    //initialize data
    memset(data, 0x00, fbs->data_size);


    return image_file;

}








int create_file(uint8_t* image_file, FATBS *fbs, const char* name, const char* ext, uint32_t size, const uint8_t* filedata){

    int index = -1;


    uint8_t* root = root_address(image_file, fbs);
    uint8_t* data = data_address(image_file, fbs);


    for(int i = 0; i< fbs->root_entry_count; i++){
        if(root[i+32]==0x00){
            index = i;
            break;
        }
    }

    if(index == -1){
        printf("No empty space available \n");
        return -1;
    }

    DirectoryEntry* entry = (DirectoryEntry*)&root[index*32];
    strncpy(entry->filename, name, 8);
    strncpy(entry->ext, ext, 3);
    entry->file_size = size;

    int cluster = -1;
    for(int i = 0; i< fbs->fat_entries; i++){

        // FAT entries are 1.5 bytes so we make 1.5bytes-steps
        if(image_file[i*3/2]==0x00){
            cluster = i;
            break;
        }
    }
    if(cluster==-1){
        printf("No free cluster available \n");
        return -1;
    }
    
    entry->first_cluster = cluster;
    uint32_t cluster_size = SECTOR_SIZE*CLUSTER_SIZE;

    //cluster_size-1 makes the result round up
    int clusters_needed = (size + cluster_size - 1) / cluster_size;
    int current_cluster = cluster;

    for(int i = 0; i < clusters_needed; i++){

        memcpy(&data[current_cluster*cluster_size], &filedata[i*cluster_size], cluster_size);
        
        int next_cluster = current_cluster+1;
        if(i == clusters_needed - 1){
            next_cluster = 0xFF8;
        }

        /*
            In FAT12 each cluster number is 12 bits. 
        
            Example: 0x123 is an entry of the FAT. 
            That means two clusters C1 and C2 both use this entry.

            The first byte (0x3) and 4 lower bits of the next byte (0x2) is for C1
            The higher 4 bits of 0x2 and the last byte (0x1) is for C2
        
            So we need to do some bit manipulations.
        */

        image_file[current_cluster*3/2] = next_cluster & 0xFF; // it takes just the lower 8 bits
        image_file[current_cluster*3/2+1] = (next_cluster>>8) & 0x0F; // it takes the higher 4 bits
        
        current_cluster = next_cluster;
    
    }
    return 0;
}

/*
int read_file(DirectoryEntry* entry, uint8_t *buffer){

    int bytes_read = -1;
    
    uint16_t current_cluster = entry->first_cluster;
    uint32_t file_size = entry->file_size;

    while(current<0xFF8 && bytes_read<file_size){

        uint32_t data_offset = current_cluster*CLUSTER_SIZE;
        uint32_t bytes_to_read = (file_size - bytes_read < CLUSTER_SIZE) ? (file_size - bytes_read) : CLUSTER_SIZE;

        memcpy(buffer + bytes_read, data+data_offset, bytes_to_read);
        bytes_read+=bytes_to_read;


        current_cluster



    }
    

    return bytes_read;
}
*/


void print_root_content(uint8_t *root, FATBS *fbs){
    // Display the contents of the root directory
    for (int i = 0; i < fbs->root_entry_count; i++) {
        DirectoryEntry* entry = (DirectoryEntry*)&root[i * 32];
        if (entry->filename[0] != 0x00) {
            printf("File: %.8s.%.3s, Size: %u bytes\n", entry->filename, entry->ext, entry->file_size);
        }
    }
}

void print_image(uint8_t *image_file, FATBS *fbs, unsigned int max_bytes_to_read){


    if(max_bytes_to_read>fbs->bytes_per_sector*fbs->total_sectors){
        max_bytes_to_read=fbs->bytes_per_sector*fbs->total_sectors;
    }

    for(uint32_t i = 0; i<max_bytes_to_read; i++){
        printf(" <%02x> ", *(image_file+i));
    }
    printf("\n");
}