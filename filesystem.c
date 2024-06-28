#include "filesystem.h"

//global variables

char *image_file; //the actual image
FATBS *fbs;          //image informations
DirectoryEntry *current_directory;
//char *root;
char *data;
int *fat;

/*
    Initializes FAT by mapping a memory location.
    Also initializes constants
*/
int fat_initialize(const char* image_path){
    
    fbs = malloc(sizeof(FATBS));
    fbs->bytes_per_sector = SECTOR_SIZE;
    fbs->root_entry_count = MAX_DIR_ENTRIES;
    fbs->sectors_per_cluster = SECTOR_PER_CLUSTER;
    fbs->total_sectors = TOTAL_SECTORS;
    fbs->cluster_size = fbs->bytes_per_sector*fbs->sectors_per_cluster;
    fbs->fat_entries = (fbs->total_sectors*fbs->bytes_per_sector)/(fbs->cluster_size);
    fbs->fat_size = fbs->fat_entries*4; // FAT32 entries are 4 bytes each
    fbs->fat_sectors = fbs->fat_size/fbs->bytes_per_sector;
    fbs->root_sectors = (fbs->root_entry_count*DIRECTORY_SIZE)/fbs->bytes_per_sector;
    fbs->root_size = fbs->root_sectors * fbs->bytes_per_sector;
    fbs->data_sectors = (fbs->total_sectors-(fbs->fat_sectors));
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

    //root = image_file + fbs->fat_size;
    data = image_file + fbs->fat_size;
    fat = (int*)image_file;
    //initialize fat
    memset(image_file, FAT_EMPTY, fbs->fat_size);

    //initialize root
    //memset(root, FAT_EMPTY, fbs->root_size);

    //initialize data
    memset(data, FAT_EMPTY, fbs->data_size);
    int first_cluster = free_cluster_index(); 
    
    current_directory = (DirectoryEntry*)&data[first_cluster*fbs->cluster_size];
    current_directory->first_cluster = first_cluster;

    create_directory("ROOT");
    list_directory();
    change_directory("ROOT");
    return 0;

}

DirectoryEntry* get_current_directory(){
    return current_directory;
}
FATBS* get_fbs(){
    return fbs;
}


/*
    Find a free cluster in the FAT
*/
int free_cluster_index(){

    int cluster = -1;

    for(int i = 0; i<fbs->fat_entries; i++){
        if(fat[i]==FAT_EMPTY){
            cluster = i;
            break;
        }
    }
    return cluster;

}
DirectoryEntry* find_free_directory_entry() {

    int cluster = current_directory->first_cluster;

    while(cluster < FAT_EOF){
        for(int i = 1; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){

            DirectoryEntry* entry = &current_directory[i];
            if(entry->filename[0]==0x00 || (unsigned char) entry->filename[0]==DELETED_DIR_ENTRY){
                return entry;
            }
        }
        cluster = fat[cluster];
    }
    return NULL;
}

int change_directory(const char* dir_name){
    
    printf("cd %s> ", dir_name);
    int cluster = current_directory->first_cluster;
    while(cluster < FAT_EOF){
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            if(strcmp(current_directory[i].filename,dir_name)==0){
                if(current_directory[i].is_directory){
                    current_directory = &current_directory[i];
                    printf("Current directory: %s\n\n", current_directory->filename);
                    return 0; 
                }else{
                    printf("Not a directory: (%s)\n\n", dir_name);
                    return -1;
                }
            }
        }
        cluster = fat[cluster];
    }
    return CDERROR;
}


// A pseudo ls function
void list_directory(){

    printf("ls (%s/)> ", current_directory->filename);
    int cluster = current_directory->first_cluster;
    while(cluster < FAT_EOF){

        for(int i = 1; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry* entry = &current_directory[i];
            if(strcmp(entry->filename, ".")==0){
                printf("dot\n");
            }
            if(strcmp(entry->filename, "..")==0){
                printf("dotdot\n");
            }
            if(entry->filename[0]==0x00){
                printf("\n");
                return;
            }
            if((unsigned char) entry->filename[0]==DELETED_DIR_ENTRY){
                continue;
            }

            if(entry->is_directory){
                printf("%s/\t", entry->filename);
            }else{
                printf("%s.%s\t", entry->filename, entry->ext);
            }
        }
        cluster = fat[cluster];
    }
    printf("\n\n");
}



/*
    Creates a file in the current directory
*/
int create_file(const char* name, const char* ext, int size, const char* filedata){

    printf("touch %s> ", name);
    DirectoryEntry *entry = find_free_directory_entry();
    if(entry==NULL){
        printf("Directory is full\n");
        return FILECREATERROR;
    }

    strncpy(entry->filename, name, 8);
    strncpy(entry->ext, ext, 3);
    entry->size = size;
    entry->is_directory = 0;

    int cluster = free_cluster_index();
    if(cluster==-1){
        printf("No free cluster available \n");
        return FILECREATERROR;
    }

    entry->parent_directory = current_directory;
    entry->first_cluster = cluster;
    int cluster_size = fbs->cluster_size;

    //cluster_size-1 makes the result round up
    int clusters_needed = (size + cluster_size - 1) / cluster_size;
    int current_cluster = cluster;

    for(int i = 0; i < clusters_needed; i++){


        memcpy(&data[current_cluster*cluster_size], &filedata[i*cluster_size], cluster_size);
        
        int next_cluster = free_cluster_index(fat, fbs);

        // The last cluster is marked as 0xFFFFFFF8
        if(i == clusters_needed - 1){
            next_cluster = FAT_EOF;
        }

        //update fat
        fat[current_cluster] = next_cluster;
        
        current_cluster = next_cluster;
    
    }
    printf("File %s created\n\n", name);
    return 0;
}


/*
    Creates a new directory in the current one
*/

int create_directory(const char* name){

    printf("mkdir %s> ", name);
    DirectoryEntry* entry = find_free_directory_entry();
    if(entry == NULL){
        printf("No empty directory slot available\n");
        return DIRCREATERROR;
    }    
    strncpy(entry->filename, name, 8);
    entry->size = 0;    
    entry->is_directory = 1;  
    int cluster = free_cluster_index();
    if(cluster==-1){
        printf("No free cluster available \n");
        return DIRCREATERROR;
    }
    entry->first_cluster = cluster;

    int cluster_size = fbs->cluster_size;
    DirectoryEntry* new_directory = (DirectoryEntry*)&data[cluster*cluster_size];
    fat[cluster]=FAT_EOF;
    memset(new_directory, 0, cluster_size);
    //if we're creating root directory, we don't need the . and .. directories 
    if(current_directory!=(DirectoryEntry*)data){
        strncpy(new_directory[0].filename, ".", 8);
        new_directory[0].first_cluster = cluster; 
        new_directory[0].is_directory=1;

        strncpy(new_directory[1].filename, "..", 8);
        new_directory[1].is_directory=1;
        new_directory[1].first_cluster = current_directory->first_cluster;
    }    
    
    printf("Directory %s created\n\n", name);
    

    return 0;
}

int erase_file(const char* filename){
    int entry_index = -1;
    int cluster = current_directory->first_cluster;
    while(cluster < FAT_EOF){

        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry *entry = &current_directory[i];
            if(strcmp(entry->filename, filename)==0){
                entry_index = i;
                break;
            }
        }
        cluster = fat[cluster];
    }
    if(entry_index==-1){
        return FILEDELERROR;
    }

    int cluster_size = fbs->cluster_size;

    int current_cluster = current_directory[entry_index].first_cluster;

    printf("\nrm %s> ", filename);
    while(current_cluster!=FAT_EOF && current_directory[entry_index].size>0){
        memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
        int next_cluster = fat[current_cluster];
        fat[current_cluster] = 0x00;
        current_cluster = next_cluster;
        current_directory[entry_index].size--;
    }

    memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
    current_directory[entry_index].filename[0]= DELETED_DIR_ENTRY;
    
    
    printf("DONE\n\n");
    return 0;
}
int erase_directory(const char* filename);


void print_image(unsigned int max_bytes_to_read){


    if(max_bytes_to_read>fbs->bytes_per_sector*fbs->total_sectors){
        max_bytes_to_read=fbs->bytes_per_sector*fbs->total_sectors;
    }

    for(int i = 0; i<max_bytes_to_read; i++){
        printf(" <%02x> ", *(fat+i));
    }
    printf("\n");
}


// TODO find_file

int read_file(const char* filename, char* buffer){

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

    int cluster_size = fbs->cluster_size;

    int current_cluster = current_directory[index].first_cluster;

    int bytes_read = 0;
    while(current_cluster!=FAT_EOF && bytes_read<current_directory[index].size){
        memcpy(&buffer[current_cluster*cluster_size], &data[current_cluster*cluster_size], cluster_size);
        bytes_read+=cluster_size;
        current_cluster = fat[current_cluster];
    }
    return bytes_read;
    



}