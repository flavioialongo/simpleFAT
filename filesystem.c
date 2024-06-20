#include "filesystem.h"

//global variables

uint8_t *image_file; //the actual image
FATBS *fbs;          //image informations
DirectoryEntry *current_directory;



uint8_t *root_address(){
    return image_file + fbs->fat_size;
}


uint8_t *data_address(){
    return image_file + fbs->fat_size+fbs->root_size;
}

int fat_initialize(const char* image_path){
    
    fbs = malloc(sizeof(FATBS));
    fbs->bytes_per_sector = SECTOR_SIZE;
    fbs->root_entry_count = ROOT_DIR_ENTRIES;
    fbs->sectors_per_cluster = CLUSTER_SIZE;
    fbs->fat_entries = FAT_ENTRIES;
    fbs->total_sectors = TOTAL_SECTORS;
    fbs->fat_size = fbs->fat_entries*4; // FAT32 entries are 4 bytes each
    fbs->fat_sectors = fbs->fat_size/fbs->bytes_per_sector;
    fbs->root_sectors = (fbs->root_entry_count*DIRECTORY_SIZE)/fbs->bytes_per_sector;
    fbs->root_size = fbs->root_sectors * fbs->bytes_per_sector;
    fbs->data_sectors = (fbs->total_sectors-(fbs->fat_sectors+fbs->root_sectors));
    fbs->data_size = fbs->data_sectors*fbs->bytes_per_sector;
    fbs->directory_size = DIRECTORY_SIZE;

    int fd = open(image_path, O_RDWR | O_CREAT | O_TRUNC, 0666);



    if(fd==-1){
        printf("Error creating or opening the image file\n");
        return INITERROR;
    }
    if(ftruncate(fd, fbs->total_sectors*fbs->bytes_per_sector)){
        printf("Error creating the image file\n");
        return INITERROR;
    }


    image_file= mmap(NULL, fbs->total_sectors*fbs->bytes_per_sector, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(image_file == MAP_FAILED){
        printf("Cannot mmap the image file\n");
        return INITERROR;
    }

    uint8_t *root = root_address(image_file, fbs);
    uint8_t *data = data_address(image_file, fbs);

    //initialize fat
    memset(image_file, FAT_EMPTY, fbs->fat_size);

    //initialize root
    memset(root, FAT_EMPTY, fbs->root_size);

    //initialize data
    memset(data, FAT_EMPTY, fbs->data_size);

    int first_cluster = free_cluster_index(); 
    
    current_directory = (DirectoryEntry*)root;
    current_directory->first_cluster = first_cluster;

    return 0;

}

DirectoryEntry* get_current_directory(){
    return current_directory;
}

int free_cluster_index(){

    int cluster = -1;

    for(uint32_t i = 0; i<fbs->fat_size; i++){
        if(image_file[i*32]==FAT_EMPTY){
            cluster = i;
            break;
        }
    }
    return cluster;

}
DirectoryEntry* find_free_directory_entry(DirectoryEntry *current_directory) {

    if(current_directory==NULL){
        uint8_t *root = root_address();
        return find_free_directory_entry((DirectoryEntry*)root);
    }

    for (uint32_t i = 0; i<DIRECTORY_SIZE; i++) {
        if (current_directory[i].filename[0] == 0x00 || current_directory[i].filename[0]==DELETED_DIR_ENTRY) {
            return &current_directory[i];
        }
    }
    return NULL;
}

int change_directory(const char* dir_name){
    
    printf("cd %s> ", dir_name);
    for(uint32_t i = 0; i<DIRECTORY_SIZE; i++){
        if(strcmp(current_directory[i].filename,dir_name)==0){
            current_directory = &current_directory[i];
            printf("Current directory: %s\n\n", current_directory->filename);
            return 0; 
        }
    }
    
    return CDERROR;
}


// A pseudo ls function
void list_directory(){
    if(current_directory[0].filename[0]!=0x00){
        printf("current directory: %s\n", current_directory[0].filename);
    }
    printf("ls> ");
    for(uint32_t i = 1; i<DIRECTORY_SIZE; i++){
        
        if(current_directory[i].filename[0]!=FAT_EMPTY && (uint8_t) current_directory[i].filename[0]!=DELETED_DIR_ENTRY){
            
            if(current_directory[i].is_directory==true){
                printf("%s/\t", current_directory[i].filename);
            }else{
                printf("%.8s.%.3s(%u bytes)\t", current_directory[i].filename, current_directory[i].ext, current_directory[i].size);
            }
        }
    }
    printf("\n\n");
}


int create_file(const char* name, const char* ext, uint32_t size, const uint8_t* filedata){

    uint8_t* data = data_address();


    DirectoryEntry *entry = find_free_directory_entry(current_directory);
    if(entry==NULL){
        printf("Directory is full\n");
        return FILECREATERROR;
    }

    strncpy(entry->filename, name, 8);
    strncpy(entry->ext, ext, 3);
    entry->size = size;
    entry->is_directory = false;

    int cluster = free_cluster_index();
    if(cluster==-1){
        printf("No free cluster available \n");
        return FILECREATERROR;
    }

    entry->parent_directory = 0;
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

int create_directory(const char* name){


    uint8_t* data = data_address();

    DirectoryEntry* entry = find_free_directory_entry(current_directory);
    if(entry == NULL){
        printf("No empty directory slot available\n");
        return DIRCREATERROR;
    }    

    strncpy(entry->filename, name, 8);
    entry->size = 0;    
    entry->is_directory = true;

    int cluster = free_cluster_index();
    if(cluster==-1){
        printf("No free cluster available \n");
        return DIRCREATERROR;
    }
    entry->first_cluster = cluster;

    uint32_t cluster_size = SECTOR_SIZE*CLUSTER_SIZE;
    
    DirectoryEntry* new_directory = (DirectoryEntry*)&data[cluster*cluster_size];
    
    strncpy(new_directory[0].filename, ".", 8);
    new_directory[0].first_cluster = cluster; 
    
    //strncpy(new_directory[1].filename, "..", 8);
    //new_directory[1].first_cluster = current_directory->first_cluster;


    

    return 0;
}

int erase_file(const char* filename){
    uint8_t* data = data_address();
    int entry_index = -1;
    for(uint32_t i = 1; i<DIRECTORY_SIZE; i++){
        if(strcmp(current_directory[i].filename, filename)==0){
            entry_index = i;
            break;
        }
    }
    if(entry_index==-1){
        return FILEDELERROR;
    }

    uint32_t cluster_size = SECTOR_SIZE*CLUSTER_SIZE;

    uint32_t current_cluster = current_directory[entry_index].first_cluster;

    printf("rm %s> ", filename);
    while(current_cluster!=FAT_EOF && current_directory[entry_index].size>0){
        memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
        uint32_t next_cluster = image_file[current_cluster*32];
        image_file[current_cluster] = 0x00;
        current_cluster = next_cluster;
        current_directory[entry_index].size--;
    }

    memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
    current_directory[entry_index].filename[0]= DELETED_DIR_ENTRY;
    
    if(current_directory[entry_index].filename[0]==DELETED_DIR_ENTRY){
        printf("è vero\n");
    }
    
    printf("DONE\n\n");
    return 0;
}
int erase_directory(const char* filename);




void print_root_content(){
    uint8_t *root = root_address();
    // Display the contents of the root directory
    for (uint32_t i = 0; i < fbs->root_entry_count; i++) {
        DirectoryEntry* entry = (DirectoryEntry*)&root[i * 32];
        if (entry->filename[0] != 0x00) {
            printf("File: %.8s.%.3s, Size: %u bytes\n", entry->filename, entry->ext, entry->size);
        }
    }
}

void print_image(unsigned int max_bytes_to_read){


    if(max_bytes_to_read>fbs->bytes_per_sector*fbs->total_sectors){
        max_bytes_to_read=fbs->bytes_per_sector*fbs->total_sectors;
    }

    for(uint32_t i = 0; i<max_bytes_to_read; i++){
        printf(" <%02x> ", *(image_file+i));
    }
    printf("\n");
}


// TODO find_file

int read_file(const char* filename, char* buffer){

    // TODO tokenize path 
    uint8_t *data = data_address();
    int index = -1;
    for(uint32_t i = 0; i<DIRECTORY_SIZE; i++){
        if(strcmp(current_directory[i].filename,filename)==0){
            index = i;
            break;
        }
    }
    if(index == -1){
        printf("File not found in directory %s\n", current_directory->filename);
        return -1;
    }

    uint32_t cluster_size = SECTOR_SIZE*CLUSTER_SIZE;

    uint32_t current_cluster = current_directory[index].first_cluster;

    uint32_t bytes_read = 0;
    while(current_cluster!=FAT_EOF && bytes_read<current_directory[index].size){
        memcpy(&buffer[current_cluster*cluster_size], &data[current_cluster*cluster_size], cluster_size);
        bytes_read+=cluster_size;
        current_cluster = image_file[current_cluster*32];
    }
    return bytes_read;
    



}