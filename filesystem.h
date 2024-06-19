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


    // Total number of sectors
	uint32_t total_sectors;
}FATBS;


typedef struct {
    char filename[8];           // Filename (8 characters)
    char ext[3];                // Extension (3 characters)

    bool is_directory;     
    uint16_t create_time;       // Creation time
    uint16_t create_date;       // Creation date
    uint16_t first_cluster;     // First cluster number
    uint32_t file_size;         // File size in bytes

} DirectoryEntry;



// Function to initialize a FAT-formatted "disk" 
uint8_t* fat_initialize(const char* image_path, FATBS *fbs);

// Function to retrieve the pointer to the root directory
uint8_t *root_address(uint8_t* image_file, FATBS *fbs);

// Function to retrieve the pointer to the data area
uint8_t *data_address(uint8_t* image_file, FATBS *fbs);



int create_file(uint8_t* image_file, FATBS *fbs, const char* name, const char* ext, uint32_t size, const uint8_t* filedata);
int read_file(uint8_t *image_file, FATBS *fbs, const char* path, char* buffer);
int free_cluster_index(uint8_t* image_file, FATBS *fbs);

void print_root_content(uint8_t *image_file, FATBS *fbs);
void print_image(uint8_t *image_file, FATBS *fbs, unsigned int max_bytes_to_read);









