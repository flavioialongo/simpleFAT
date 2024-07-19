#include "filesystem.h"

//global variables
char *image_file;       //the actual image
FATBS *fbs;             //FAT boot sector
DirectoryEntry *current_directory;
char *data;
int *fat;

/*
    Initializes FAT by mapping a memory location.
    Also initializes constants
*/
int fat_initialize(const char* image_path){
    
    int fd = open(image_path, O_RDWR | O_CREAT | O_TRUNC, 0666);



    if(fd==-1){
        return INITERROR;
    }
    if(ftruncate(fd, TOTAL_SECTORS*SECTOR_SIZE)){
        return INITERROR;
    }

    

    image_file = mmap(NULL, TOTAL_SECTORS*SECTOR_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    if(image_file == MAP_FAILED){
        return INITERROR;
    }

    fbs = (FATBS*)image_file;
    fbs->bytes_per_sector = SECTOR_SIZE;
    fbs->total_sectors = TOTAL_SECTORS;
    fbs->cluster_size = SECTOR_SIZE*SECTOR_PER_CLUSTER;
    fbs->fat_entries = (fbs->total_sectors)*fbs->bytes_per_sector/fbs->cluster_size;
    fbs->fat_size = fbs->fat_entries*4; // FAT32 entries are 4 bytes each

    fbs->data_size = (fbs->total_sectors-(fbs->fat_size/fbs->bytes_per_sector))*fbs->bytes_per_sector - sizeof(FATBS);

    data = image_file + fbs->fat_size+sizeof(FATBS);
    fat = (int*)(image_file+sizeof(FATBS));

    //initialize fat
    memset(image_file+sizeof(FATBS), FAT_EMPTY, fbs->fat_size);

    //initialize root
    //memset(root, FAT_EMPTY, fbs->root_size);

    //initialize data
    memset(data, FAT_EMPTY, fbs->data_size);
    
    //ROOT DIRECTORY CREATION
    current_directory = (DirectoryEntry*)data;
    current_directory->first_cluster = 0;
    strncpy(current_directory->filename, "ROOT", 8);
    memset(current_directory->ext, 0, 3);
    current_directory->is_directory=true;
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

    int cluster = FATFULL;

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
    
    int cluster = current_directory->first_cluster;
    while(cluster != FAT_EOF){
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster * fbs->cluster_size];
        for(int i=1; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry* entry = &dir[i];
            if(strcmp(entry->filename,dir_name)==0){
                if(entry->is_directory){
                    int new_cluster = entry->first_cluster;
                    current_directory = (DirectoryEntry*)&data[new_cluster*fbs->cluster_size];
                    return 0; 
                }else{
                    printf("Not a directory: (%s)\n\n", dir_name);
                    return FILENOTFOUND;
                }
            }
        }
        cluster = fat[cluster];
    }
    return FILENOTFOUND;
}



//pseudo ls function 
void list_directory() {
    int cluster = current_directory->first_cluster;

    while (cluster != FAT_EOF) {
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster * fbs->cluster_size];
        
        //we start from i=1 because the first position is the directory herself
        for (int i = 1;i < fbs->cluster_size / sizeof(DirectoryEntry); i++) {
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

    DirectoryEntry *entry = find_free_directory_entry();
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
    entry->first_cluster = cluster;
    fat[cluster] = FAT_INUSE;
    int cluster_size = fbs->cluster_size;

    //cluster_size-1 makes the result round up
    int clusters_needed = (size + cluster_size) / cluster_size;
    int current_cluster = cluster;

    int i;
    for(i = 0; i < clusters_needed; i++){

        //handle empty initialization
        if(filedata == NULL) {
            memset(&data[current_cluster*cluster_size], 0, cluster_size);
        }else {
            memcpy(&data[current_cluster*cluster_size], &filedata[i*cluster_size], cluster_size);
        }

        
        int next_cluster = free_cluster_index();
        fat[next_cluster]=FAT_INUSE;
        // The last cluster is marked as 0xFFFFFFF8
        if(i == clusters_needed - 1){
            next_cluster = FAT_EOF;
        }

        //update fat
        fat[current_cluster] = next_cluster;
        
        current_cluster = next_cluster;
    
    }
    return 0;
}


/*
    Creates a new directory in the current one
*/

int create_directory(const char* name){

    DirectoryEntry* entry = find_free_directory_entry();
    
    if(entry == NULL){
        printf("No empty directory slot available\n");
        return DIRCREATERROR;
    }    
    strncpy(entry->filename, name, 8);
    entry->size = 0;    
    entry->is_directory = true;
    int cluster = free_cluster_index();
    if(cluster==-1) {
        printf("No free cluster available \n");
        return DIRCREATERROR;
    }
    fat[cluster]=FAT_EOF;
    entry->first_cluster=cluster;
    int cluster_size = fbs->cluster_size;
    DirectoryEntry* new_directory = (DirectoryEntry*)&data[cluster*cluster_size];
    
    memset(new_directory, 0, cluster_size);

    new_directory[0] = *entry;

    //For each subdirectory, create the .. directory
    strncpy(new_directory[1].filename, "..", 8);
    new_directory[1].is_directory=true;
    new_directory[1].first_cluster = current_directory->first_cluster;
    new_directory[1].size=0;
    strncpy(new_directory[1].ext, "", 3);

    return 0;
}


DirectoryEntry *get_file(const char* filename, const char* ext, bool is_dir){

    int cluster = current_directory->first_cluster;
    int cluster_size = fbs->cluster_size;
    while (cluster!=FAT_EOF) {
        DirectoryEntry* dir = (DirectoryEntry*)&data[cluster*cluster_size];
        for(int i = 0; i<fbs->cluster_size/sizeof(DirectoryEntry); i++) {
            DirectoryEntry * entry = &dir[i];
            if(entry->filename[0]==0x00) {
                break;
            }
            if(strcmp(entry->filename,filename)==0 && strcmp(entry->ext, ext)==0 && entry->is_directory == is_dir){
                return entry;
            }
        }
        cluster=fat[cluster];
    }

    return NULL;

}


int erase_file(const char* filename, const char* ext){
    
    DirectoryEntry *file = get_file(filename, ext, false);

    if(file == NULL) {
        return FILENOTFOUND;
    }
    int cluster_size = fbs->cluster_size;
    int current_cluster = file->first_cluster;

    while(current_cluster!=FAT_EOF && file->size>0){
        memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
        int next_cluster = fat[current_cluster];
        fat[current_cluster] = 0x00;
        current_cluster = next_cluster;
        file->size--;
    }
    file->filename[0]= DELETED_DIR_ENTRY;

    return 0;
}

void erase_empty_directory(DirectoryEntry* dir){

    int cluster_size = fbs->cluster_size;
    int current_cluster = dir->first_cluster;
    while(current_cluster!=FAT_EOF){
        memset(&data[current_cluster*cluster_size], 0x00000000, cluster_size);
        int next_cluster = fat[current_cluster];
        fat[current_cluster] = 0x00;
        current_cluster = next_cluster;
    }
    dir->filename[0]= DELETED_DIR_ENTRY;
}
bool is_empty_directory(DirectoryEntry* dir){

    bool res=true;
    int cluster = dir->first_cluster;
    int cluster_size = fbs->cluster_size;
    while(cluster != FAT_EOF){
        DirectoryEntry *d = (DirectoryEntry*)&data[cluster*cluster_size];
        for(int i = 1; i<fbs->cluster_size/sizeof(DirectoryEntry); i++){
            DirectoryEntry *entry = &d[i];
            if(entry->filename[0]!=0x00 
            && (unsigned char) entry->filename[0]!=DELETED_DIR_ENTRY 
            && strcmp(entry->filename, "..")!=0 ){
                return false;
            }
        }
        cluster = fat[cluster];
    }
    return res;

}

int erase_dir(const char* dirname, bool rf){

    DirectoryEntry *dir = get_file(dirname, "", 1);
    int res=-1;
    if(dir == NULL){
        return FILENOTFOUND;
    }

    bool is_empty = is_empty_directory(dir);
    if(is_empty){
        erase_empty_directory(dir);
    }else{
        if(rf==true){
            int cluster_size = fbs->cluster_size;
            int current_cluster = dir->first_cluster;

            // we temporarely set dir as the current directory, in order to recursively delete everything
            DirectoryEntry* temp = current_directory;
            current_directory=dir;

            //traverse the directory to search for files / directories and delete them
            while(current_cluster!=FAT_EOF){
                DirectoryEntry * d = (DirectoryEntry*) &data[cluster_size*current_cluster];
                for(int i = 1; i<cluster_size/sizeof(DirectoryEntry); i++) {
                    DirectoryEntry *entry = &d[i];
                    if(entry->filename[0]==0x00 || (unsigned char) entry->filename[0]==DELETED_DIR_ENTRY) {
                        continue;
                    }
                    if(strcmp(entry->filename, "..")==0) {
                        continue;
                    }
                    if(entry->is_directory==true) {
                        res = erase_dir(entry->filename, rf);
                        if (res) return FILENOTFOUND;
                    }else {
                        res = erase_file(entry->filename, entry->ext);
                        if(res) return FILENOTFOUND;
                    }
                }
                current_cluster=fat[current_cluster];
            }

            current_directory = temp;
            erase_empty_directory(dir);
        }else{ // no recursive
            printf("Directory not empty\n");
            return DIRNOTEMPTY;
        }
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

FileHandle *open_file(const char* filename, const char* ext) {

    DirectoryEntry* entry = get_file(filename, ext, false);
    if(entry==NULL) {
        return NULL;
    }
    FileHandle * file = malloc(sizeof(FileHandle));
    file->entry = entry;
    file->current_cluster = entry->first_cluster;
    file->position = 0;
    return file;
}

void close_file(FileHandle* file){
    file->entry=NULL;
    file->current_cluster=0;
    file->position=0;
    free(file);
}

int seek_file(FileHandle* file, int offset) {

    if(offset>file->entry->size) {
        return -1;
    }

    int cluster_index = offset/fbs->cluster_size;
    int current_cluster = file->current_cluster;
    for(int i = 0; i<cluster_index; i++) {
        current_cluster = fat[current_cluster];
        // Reached end of fat chain unexpectedly
        if(current_cluster == FAT_EOF) {
            return -1;
        }
    }
    file->current_cluster = current_cluster;
    file->position = offset % fbs->cluster_size;

    return 0;
}



int read_file(FileHandle* file, char* buffer){

    int cluster = file->current_cluster;
    int bytes_read = 0;
    int cluster_size = fbs->cluster_size;
    int file_size = file->entry->size;

    bool is_first_cluster = true;
    while(cluster!=FAT_EOF && file_size>0){

        // If the file size > cluster size (i.e. 512 bytes) we read 512 bytes and continue the while loop
        // If the file size < cluster size, we just read the amount of bytes that we need
        int bytes_to_read;
        if(is_first_cluster) {
            bytes_to_read = (file_size > cluster_size-file->position) ? cluster_size : file_size;
            memcpy(buffer + bytes_read, &data[cluster*cluster_size+file->position], bytes_to_read);
            is_first_cluster=false;
        }else {
            bytes_to_read = (file_size > cluster_size) ? cluster_size : file_size;
            memcpy(buffer + bytes_read, &data[cluster*cluster_size], bytes_to_read);
        }
        bytes_read += bytes_to_read;
        file_size -= bytes_to_read;
        cluster = fat[cluster];
    }
    return bytes_read;

}

int write_file(FileHandle* file, char* buffer) {

    int file_size = file->entry->size;
    int buffer_size = strlen(buffer)+1;
    int bytes_written = 0;
    int clusters_to_add = 0;
    int cluster_size = fbs->cluster_size;
    // Expand file size
    if(buffer_size+file->position>file_size) {
        file->entry->size = buffer_size + file->position;
        // We round up es. 513 bytes require 2 clusters
        clusters_to_add =  (buffer_size+file->position)/fbs->cluster_size;
        if(clusters_to_add!=0) {
            int new_cluster_chain_index = free_cluster_index();
            // expand file size
            for(int i = 0; i<clusters_to_add; i++) {
                memset(&data[new_cluster_chain_index*cluster_size], 0, cluster_size);
                int next_cluster = free_cluster_index();
                // The last cluster is marked as 0xFFFFFFF8
                if(i == clusters_to_add - 1){
                    next_cluster = FAT_EOF;
                    fat[new_cluster_chain_index] = next_cluster;
                    break;
                }
                //update fat
                fat[new_cluster_chain_index] = next_cluster;
                new_cluster_chain_index = next_cluster;
            }

            int current_cluster = file->entry->first_cluster;
            //append new cluster chain
            while(current_cluster != FAT_EOF) {
                if(fat[current_cluster] == FAT_EOF) {
                    fat[current_cluster] = new_cluster_chain_index;
                    break;
                }
                current_cluster = fat[current_cluster];
            }
        }

    }

    int current_cluster = file->current_cluster;
    bool is_first_cluster = true;
    while(current_cluster!=FAT_EOF && buffer_size>0){

        int bytes_to_write;

        // In order to support write after a seek, if we're writing in the first
        // cluster of the FileHandler, we need to take care of the offset
        if(is_first_cluster) {
            bytes_to_write = (buffer_size > cluster_size-file->position) ? cluster_size : buffer_size;
            memcpy(&data[current_cluster*cluster_size+file->position], buffer + bytes_written, bytes_to_write);
            is_first_cluster = false;
        }else {
            bytes_to_write = (buffer_size > cluster_size) ? cluster_size : buffer_size;
            memcpy(&data[current_cluster*cluster_size], buffer + bytes_written, bytes_to_write);
        }
        bytes_written += bytes_to_write;
        buffer_size -= bytes_written;
        current_cluster = fat[current_cluster];
    }

    return bytes_written;
}