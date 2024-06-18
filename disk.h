#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
 
#define SECTOR_SIZE 512     // Each track is divided into sectors of 512 bytes
#define CLUSTER_SIZE 1      // How many sectors does the cluster use
#define FAT_ENTRIES 128     // How many rows does the FAT have
#define SECTORS_NUMBER 2880 // Total number or sectors, 1.44 MB disk
#define ROOT_DIR_ENTRIES 16 // Number of entries in the root directory
#define ROOT_DIR_SIZE 32    //Size in bytes of root directory entry sizes (32 bytes)

typedef struct{
    uint16_t fat[FAT_ENTRIES];     // FAT of 16 bit entries
    uint8_t root[ROOT_DIR_ENTRIES][32]; 
    uint8_t data[(SECTORS_NUMBER - (1 + ( FAT_ENTRIES * 2) / SECTOR_SIZE + (ROOT_DIR_ENTRIES * ROOT_DIR_SIZE ) / SECTOR_SIZE )) * SECTOR_SIZE];
} Disk;


// Function to initialize a FAT-formatted "disk" 
void disk_initialize(Disk *disk);



