#include "filesystem.h"



uint8_t *root_address(uint8_t* image_file, FATBS *fbs){
    return image_file + fbs->fat_size;
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
    fbs->fat_size = fbs->fat_entries*4; // FAT32 entries are 4 bytes each
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
    memset(image_file, FAT_EMPTY, fbs->fat_size);

    //initialize root
    memset(root, FAT_EMPTY, fbs->root_size);

    //initialize data
    memset(data, FAT_EMPTY, fbs->data_size);


    return image_file;

}



int free_cluster_index(uint8_t* image_file, FATBS *fbs){

    int cluster = -1;

    for(uint32_t i = 0; i<fbs->fat_size; i++){
        if(image_file[i*32]==FAT_EMPTY){
            cluster = i;
            break;
        }
    }
    return cluster;

}


int create_file(uint8_t* image_file, FATBS *fbs, const char* name, const char* ext, uint32_t size, const uint8_t* filedata){

    // retrieve the starting points of root directory and data area
    uint8_t* root = root_address(image_file, fbs);
    uint8_t* data = data_address(image_file, fbs);

    // find an empty slot in the root directory
    int index = -1;
    for(unsigned int i = 0; i< fbs->root_entry_count; i++){
        if(root[i*32]==FAT_EMPTY){
            index = i;
            break;
        }
    }

    if(index == -1){
        printf("No empty space available \n");
        return -1;
    }


    // create the actual entry 
    DirectoryEntry* entry = (DirectoryEntry*)&root[index*32];
    strncpy(entry->filename, name, 8);
    strncpy(entry->ext, ext, 3);
    entry->file_size = size;
    entry->is_directory = false;
    

    int cluster = free_cluster_index(image_file, fbs);
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
        
        int next_cluster = free_cluster_index(image_file, fbs);

        // The last cluster is marked as 0xFFFFFFF8
        if(i == clusters_needed - 1){
            next_cluster = FAT_EOF;
        }

        //update fat
        image_file[current_cluster*32] = next_cluster;
        
        current_cluster = next_cluster;
    
    }
    return 0;
}


void print_root_content(uint8_t *root, FATBS *fbs){
    // Display the contents of the root directory
    for (uint32_t i = 0; i < fbs->root_entry_count; i++) {
        DirectoryEntry* entry = (DirectoryEntry*)&root[i * 32];
        if (entry->filename[0] != 0x00000000) {
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


// TODO find_file

int read_file(uint8_t *image_file, FATBS *fbs, const char* path, char* buffer){

    // TODO tokenize path 
    printf("BUFFER: %s\n", path);
    uint8_t *root = root_address(image_file, fbs);
    uint8_t *data = data_address(image_file, fbs);
    int index = -1;
    DirectoryEntry* entry;
    for(uint32_t i = 0; i<fbs->root_entry_count; i++){
        entry = (DirectoryEntry*)&root[i * 32];
        if(entry->filename[0]==0x00000000){
            continue;
        }
        if(strcmp(entry->filename, path)==0){
            index = i;
            break;
        }
    }
    if(index == -1){
        printf("File not found\n");
        return -1;
    }

    uint32_t cluster_size = SECTOR_SIZE*CLUSTER_SIZE;

    uint32_t current_cluster = entry->first_cluster;

    uint32_t bytes_read = 0;
    while(current_cluster!=FAT_EOF && bytes_read<entry->file_size){
        memcpy(&buffer[current_cluster*cluster_size], &data[current_cluster*cluster_size], cluster_size);
        bytes_read+=cluster_size;
        current_cluster = image_file[current_cluster*32];
    }
    return bytes_read;
    



}