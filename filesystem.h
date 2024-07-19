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
#define TOTAL_SECTORS 65536

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

typedef struct{
    char filename[8];           // Filename (8 characters)
    char ext[3];                // Extension (3 characters)
    bool is_directory;
    int first_cluster;     // First cluster number
    int size;              // File size in bytes
} __attribute__((packed)) DirectoryEntry;

typedef struct {
    DirectoryEntry* entry;    // Reference to the file's directory entry
    int position;        // Current position in the file
    int current_cluster; // Current cluster being accessed
} FileHandle;

#define DIRCREATERROR -1
#define FILECREATERROR -2
#define INITERROR -3
#define FILENOTFOUND -4
#define FILEREADERR -5
#define DIRNOTEMPTY -6
#define FATFULL -7


/*
    ALL FUNCTIONS EXCEPT WRITE/READ RETURN 0 IF NO ERRORS OCCUR
    WRITE/READ RETURNS NUMBER OF BYTES WRITTEN/READ
*/


/*
    Initializes a FAT-formatted "disk" 
    If errors occur INITERROR is returned 
*/ 
int fat_initialize(const char* image_path);

/*
    Returns the pointer to the current directory
*/
DirectoryEntry* get_current_directory();

/*
    Returns the pointer to the FatBootSector
*/
FATBS* get_fbs();

/*
    Returns the pointer to an empty slot in the current directory
    If no entries, returns NULL
*/
DirectoryEntry* find_free_directory_entry();

/*
    Returns the index of the first free cluster 
    If no free clusters exist FATFULL is returned
*/
int free_cluster_index();

/*
    Creates directory "name/"
    If
*/
int create_directory(const char* name);

/*
    Creates the file "name.ext" with given size and given content filedata
*/
int create_file(const char* name, const char* ext, int size, const char* filedata);

/*
    Removes the file "filename.ext" in the current directory
    If file is not found, returns FILENOTFOUND
*/
int erase_file(const char* filename, const char* ext);

/*

    Removes the directory "dirname/"
    If rf = true recursively remove everything
    If rf = false and directory is not empty, returns DIRNOTEMPTY
    If directory is not found, returns FILENOTFOUND
*/
int erase_dir(const char* dirname, bool rf);

/*
    Returns true if the directory is empty
*/
bool is_empty_directory(DirectoryEntry* dir);

/*
    Sets the current directory to dir_name
    dir_name/ should be in the current directory
    if not found FILENOTFOUND is returned
*/
int change_directory(const char* dir_name);


/*
    List all the directory entries (similar to ls)
*/
void list_directory();

/*
    Ausiliary function to get the pointer to the wanted file/directory
    If is_dir == 1, just search for DirectorEntry with is_directory set to 1
*/
DirectoryEntry *get_file(const char* filename, const char* ext, bool is_dir);

/*
    Writes buffer content in the file
    Returns number of bytes written
    This function can expand the file size
    Returns number of bytes written
*/
int write_file(FileHandle* file, char* buffer);

/*
    Read the content in the file "filename.ext" and saves it in a buffer
    memory for buffer should be allocated
    Returns number of bytes read
*/
int read_file(FileHandle* file, char* buffer);

/*
    Changes the position and the current cluster in the FileHandle
    Position is relative to current cluster:
    e.g. ClusterSize 512bytes, offset 513:
            FileHandle current_cluster is the second cluster in the chain
            FileHandle position is 1

    Returns -1 if offset is greater than the file size
*/
int seek_file(FileHandle* file, int offset);

/*
    Opens a file in the current directory
    Returns a FileHandle* if no errors occur, NULL otherwise
*/
FileHandle *open_file(const char* filename, const char* ext);

/*
    Closes a previously opened file
*/
void close_file(FileHandle* file);

/*
    Ausiliary function to print hexadecimal values of the img file
*/
void print_image(unsigned int max_bytes_to_read);









