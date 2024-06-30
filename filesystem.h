#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#define SECTOR_SIZE 512
#define SECTOR_PER_CLUSTER 1
#define TOTAL_SECTORS 65563
#define MAX_DIR_ENTRIES 65536

#define FAT_EMPTY 0x00000000
#define FAT_EOF 0x0FFFFFF8
#define FAT_INUSE 0xFFFFFFFF


typedef struct{

    // Number of bytes in a sector
	int bytes_per_sector;

    // Number of FAT entries
	int fat_entries;
    
    // Size of clusters
    int cluster_size;

    // FAT size in bytes
	int fat_size;
    
    // Size in bytes of the Data Area
    int data_size;

    // Total number of sectors
	int total_sectors;
}FATBS;


#define DIRECTORY_SIZE 32
#define DELETED_DIR_ENTRY 0xE5

typedef struct DirectoryEntry{
    char filename[8];           // Filename (8 characters)
    char ext[3];                // Extension (3 characters)

    char is_directory;     

    struct DirectoryEntry* parent_directory;     
    int first_cluster;     // First cluster number
    int size;              // File size in bytes
    int entry_count;
} __attribute__((packed)) DirectoryEntry;


#define DIRCREATERROR -1
#define FILECREATERROR -2
#define CDERROR -3
#define INITERROR -4
#define FILEDELERROR -5
#define FILEREADERR -6



// Function to initialize a FAT-formatted "disk" 
int fat_initialize(const char* image_path);

FATBS* get_fbs();
DirectoryEntry* find_free_directory_entry();
DirectoryEntry* get_current_directory();
int free_cluster_index();


int create_directory(const char* name);
int create_file(const char* name, const char* ext, int size, const char* filedata);
int read_file(const char* filename, const char* ext, char* buffer);
int erase_file(const char* filename, const char* ext, char is_dir);
int erase_dir(const char* dirname);
DirectoryEntry *get_file(const char* filename, const char* ext, char is_dir);
char is_empty_directory(DirectoryEntry* dir);



int change_directory(const char* dir_name);
void list_directory();



void print_image(unsigned int max_bytes_to_read);









