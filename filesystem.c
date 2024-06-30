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
    fbs->total_sectors = TOTAL_SECTORS;
    fbs->cluster_size = SECTOR_SIZE*SECTOR_PER_CLUSTER;
    fbs->fat_entries = (fbs->total_sectors*fbs->bytes_per_sector)/(fbs->cluster_size);
    fbs->fat_size = fbs->fat_entries*4; // FAT32 entries are 4 bytes each
    fbs->data_size = (fbs->total_sectors-(fbs->fat_size/fbs->bytes_per_sector))*fbs->bytes_per_sector;

    int fd = open(image_path, O_RDWR | O_CREAT | O_TRUNC, 0666);



    if(fd==-1){
        printf("Error creating or opening the image file\n");
        return INITERROR;
    }
    if(ftruncate(fd, fbs->total_sectors*fbs->bytes_per_sector)){
        printf("Error creating the image file\n");
        return INITERROR;
    }


    image_file = mmap(NULL, fbs->total_sectors*fbs->bytes_per_sector, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

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
    





    //ROOT DIRECTORY CREATION
    current_directory = (DirectoryEntry*)data;
    current_directory->first_cluster = 0;
    strncpy(current_directory->filename, "ROOT", 8);
    memset(current_directory->ext, 0, 3);
    current_directory->entry_count=0;
    current_directory->is_directory=1;
    current_directory->parent_directory=NULL;
    fat[0]=FAT_EOF;
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

    for(int i = 1; i<fbs->fat_entries; i++){
        if(fat[i]==FAT_EMPTY){
            cluster = i;
            break;
        }
    }
    return cluster;

}
DirectoryEntry* find_free_directory_entry() {

    int cluster = current_directory->first_cluster;

    while(cluster != FAT_EOF){
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster * fbs->cluster_size];
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry* entry = &dir[i];
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
    while(cluster != FAT_EOF){
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster * fbs->cluster_size];
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry* entry = &dir[i];
            if(strcmp(entry->filename,dir_name)==0){
                int new_cluster = entry->first_cluster;
                if(entry->is_directory){
                    current_directory = (DirectoryEntry*)&data[new_cluster*fbs->cluster_size];
                    if(strcmp(entry->filename, "..")==0) {

                        printf("Current directory: %s\n\n", current_directory->filename);
                    }else {
                        printf("Current directory: %s\n\n", dir_name);
                    }
                    return 0; 
                }else{
                    printf("Not a directory: (%s)\n\n", dir_name);
                    return -1;
                }
            }
        }
        cluster = fat[cluster];
    }
    printf("No %s directory found\n\n", dir_name);
    return CDERROR;
}



//pseudo ls function 
void list_directory() {
    printf("ls (%s/)> ", current_directory->filename);
    int cluster = current_directory->first_cluster;

    while (cluster != FAT_EOF) {
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster * fbs->cluster_size];
        
        for (int i = 0; i < fbs->cluster_size / sizeof(DirectoryEntry); i++) {
            DirectoryEntry* entry = &dir[i];

            if (entry->filename[0] == 0x00) { // Empty entry, stop reading
                printf("\n\n");
                return;
            }
            if ((unsigned char)entry->filename[0] == DELETED_DIR_ENTRY) { // Deleted entry, skip
                continue;
            }
            if (entry->is_directory) {
                printf("%.8s/\t", entry->filename);
            } else {
                printf("%.8s.%.3s\t", entry->filename, entry->ext);
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
    if(cluster==-1) {
        printf("No free cluster available \n");
        return DIRCREATERROR;
    }
    fat[cluster]=FAT_EOF;
    int cluster_size = fbs->cluster_size;
    DirectoryEntry* new_directory = (DirectoryEntry*)&data[cluster*cluster_size];
    
    memset(new_directory, 0, cluster_size);


    //For each subdirectory, create . and .. directories
    entry->first_cluster = cluster;
    strncpy(new_directory[0].filename, ".", 8);
    new_directory[0].first_cluster = cluster;
    new_directory[0].is_directory=1;
    new_directory[0].size=0;
    strncpy(new_directory[0].ext, "", 3);

    strncpy(new_directory[1].filename, "..", 8);
    new_directory[1].is_directory=1;
    new_directory[1].first_cluster = current_directory->first_cluster;
    new_directory[1].size=0;
    strncpy(new_directory[1].ext, "", 3);

    printf("Directory %s created\n\n", name);
    return 0;
}


DirectoryEntry *get_file(const char* filename, const char* ext, char is_dir){

    int entry_index = -1;
    int cluster = current_directory->first_cluster;
    while (cluster!=FAT_EOF) {
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++) {
            if(strcmp(current_directory[i].filename,filename)==0 && strcmp(current_directory[i].ext, ext)==0 && current_directory[i].is_directory == is_dir){
                entry_index = i;
                break;
            }
        }
        if(entry_index != -1) break;
        cluster=fat[cluster];
    }

    if(entry_index == -1) return NULL;

    return &current_directory[entry_index];
}


int erase_file(const char* filename, const char* ext){
    
    DirectoryEntry *file = get_file(filename, ext, 0);

    int cluster_size = fbs->cluster_size;
    int current_cluster = file->first_cluster;

    printf("\nrm %s.%s> ", filename, ext);
    while(current_cluster!=FAT_EOF && file->size>0){
        memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
        int next_cluster = fat[current_cluster];
        fat[current_cluster] = 0x00;
        current_cluster = next_cluster;
        file->size--;
    }

    memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
    file->filename[0]= DELETED_DIR_ENTRY;

    printf("DONE\n\n");
    return 0;
}

char is_empty_directory(DirectoryEntry* dir){

    char res=1;
    int cluster = dir->first_cluster;

    while(cluster != EOF){
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry *entry = &dir[i];
            if(entry->filename[0]!=0x00 
            && (unsigned char) entry->filename[0]!=DELETED_DIR_ENTRY 
            && (strcmp(entry->filename, ".")!=0 && strcmp(entry->filename, "..")!=0 ) ){
                return 0;
            }
        }
        cluster = fat[cluster];
    }
    return res;

}




int erase_dir(const char* dirname){


    printf("rmdir %s/> \n", dirname);
    DirectoryEntry *dir = get_file(dirname, "", 1);

    if(dir == NULL){
        return FILEDELERROR;
    }

    char is_empty = is_empty_directory(dir);
    if(is_empty){

        int cluster_size = fbs->cluster_size;
        int current_cluster = dir->first_cluster;
        while(current_cluster!=FAT_EOF){
            memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
            int next_cluster = fat[current_cluster];
            fat[current_cluster] = 0x00;
            current_cluster = next_cluster;
       }
       memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
       dir->filename[0]= DELETED_DIR_ENTRY;
    }else{
        printf("Directory not empty\n");
        return FILEDELERROR;
    }


    return 0;



}
void print_image(unsigned int max_bytes_to_read){
    if(max_bytes_to_read>fbs->bytes_per_sector*fbs->total_sectors){
        max_bytes_to_read=fbs->bytes_per_sector*fbs->total_sectors;
    }
    for(int i = 0; i<max_bytes_to_read; i++){
        printf(" <%02x> ", *(fat+i));
    }
    printf("\n");
}


int read_file(const char* filename, const char* ext, char* buffer){

    int index = -1;
    int cluster = current_directory->first_cluster;
    while (cluster!=FAT_EOF) {
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++) {
            if(strcmp(current_directory[i].filename,filename)==0 && strcmp(current_directory[i].ext, ext)==0) {
                index = i;
                break;
            }

        }
        if(index != -1) break;
        cluster=fat[cluster];
    }

    if(index == -1){
        printf("File not found in directory %s\n", current_directory->filename);
        return FILEREADERR;
    }



    cluster = current_directory[index].first_cluster;

    int bytes_read = 0;
    int cluster_size = fbs->cluster_size;
    int file_size = current_directory[index].size;
    while(cluster!=FAT_EOF && file_size>0){

        // If the file size > cluster size (i.e. 512 bytes) we read 512 bytes and continue the while loop
        // If the file size < cluster size, we just read the amount of bytes that we need
        int bytes_to_read = (file_size > cluster_size) ? cluster_size : file_size;
        memcpy(buffer + bytes_read, &data[cluster*cluster_size], bytes_to_read);
        bytes_read += bytes_to_read;
        file_size -= bytes_to_read;
        cluster = fat[cluster];
    }
    return bytes_read;

}