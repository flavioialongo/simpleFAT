#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>


#define SECTOR_SIZE 512
#define CLUSTER_SIZE 8
#define TOTAL_SECTORS 65563
#define FAT_ENTRIES 512
#define ROOT_DIR_ENTRIES 224

#define FAT_EMPTY 0x00000000
#define FAT_EOF 0xFFFFFFF8



typedef struct{

    // Number of bytes in a sector
	uint32_t bytes_per_sector;

    // Number of sectors in a cluster
	uint32_t sectors_per_cluster;
    
    // Number of FAT entries
	uint32_t fat_entries;
    
    // FAT size in bytes
	uint32_t fat_size;

    // Number of entries in the Root Directory
	uint32_t root_entry_count;
    
    // Number of sectors for the Root Directory
    uint32_t root_sectors;
    
    // Size in bytes of Root Directory
    uint32_t root_size;

    // Number of sectors for FAT
    uint32_t fat_sectors;

    // Number of sectors for the Data Area
    uint32_t data_sectors;

    // Size in bytes of the Data Area
    uint32_t data_size;

    // Size in bytes of Directories Entry
    uint32_t directory_size;

    // Total number of sectors
	uint32_t total_sectors;
}FATBS;


#define DIRECTORY_SIZE 32
#define DELETED_DIR_ENTRY 0xE5
typedef struct DirectoryEntry{
    char filename[8];           // Filename (8 characters)
    char ext[3];                // Extension (3 characters)

    bool is_directory;     

    struct DirectoryEntry* parent_directory;     
    uint16_t create_time;       // Creation time
    uint16_t create_date;       // Creation date
    uint16_t first_cluster;     // First cluster number
    uint32_t size;         // File size in bytes

} __attribute__((packed)) DirectoryEntry;


#define DIRCREATERROR -1
#define FILECREATERROR -2
#define CDERROR -3
#define INITERROR -4
#define FILEDELERROR -5




// Function to initialize a FAT-formatted "disk" 
int fat_initialize(const char* image_path);

// Function to retrieve the pointer to the root directory
uint8_t *root_address();

// Function to retrieve the pointer to the data area
uint8_t *data_address();

DirectoryEntry* get_current_directory();
int create_directory(const char* name);
int create_file(const char* name, const char* ext, uint32_t size, const uint8_t* filedata);
int read_file(const char* filename, char* buffer);
int erase_file(const char* filename);





int change_directory(const char* dir_name);
void list_directory();
int free_cluster_index();





void print_root_content();
void print_image(unsigned int max_bytes_to_read);









