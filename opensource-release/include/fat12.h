#ifndef FAT12_H
#define FAT12_H

#include "types.h"


struct __attribute__((packed)) FAT12BootSector {
    uint8_t  jump[3];              
    char     oem_name[8];          
    uint16_t bytes_per_sector;     
    uint8_t  sectors_per_cluster;  
    uint16_t reserved_sectors;     
    uint8_t  fat_count;            
    uint16_t root_entry_count;     
    uint16_t total_sectors;        
    uint8_t  media_descriptor;     
    uint16_t sectors_per_fat;      
    uint16_t sectors_per_track;    
    uint16_t head_count;           
    uint32_t hidden_sectors;       
    uint32_t large_sector_count;   
};


struct __attribute__((packed)) FAT12DirectoryEntry {
    char     filename[8];          
    char     extension[3];         
    uint8_t  attributes;           
    uint8_t  reserved;             
    uint8_t  creation_time_ms;     
    uint16_t creation_time;        
    uint16_t creation_date;        
    uint16_t last_access_date;     
    uint16_t first_cluster_high;   
    uint16_t modification_time;    
    uint16_t modification_date;    
    uint16_t first_cluster;        
    uint32_t file_size;            
};


#define FAT12_ATTR_READ_ONLY  0x01
#define FAT12_ATTR_HIDDEN     0x02
#define FAT12_ATTR_SYSTEM     0x04
#define FAT12_ATTR_VOLUME_ID  0x08
#define FAT12_ATTR_DIRECTORY  0x10
#define FAT12_ATTR_ARCHIVE    0x20


class FAT12 {
public:
    static void initialize();
    static bool mount();
    static bool read_file(const char* filename, uint8_t* buffer, uint32_t max_size, uint32_t* bytes_read);
    static bool write_file(const char* filename, const uint8_t* buffer, uint32_t size);
    static bool create_file(const char* filename);
    static bool create_directory(const char* dirname);
    static bool delete_file(const char* filename);
    static bool delete_directory(const char* dirname);
    static bool list_directory();
    static FAT12DirectoryEntry* get_root_directory();
    static uint16_t get_root_entry_count();

private:
    static bool read_boot_sector();
    static bool read_root_directory();
    static bool write_root_directory();
    static bool write_fat_table();
    static uint16_t get_next_cluster(uint16_t cluster);
    static void set_next_cluster(uint16_t cluster, uint16_t value);
    static uint16_t find_free_cluster();
    static bool read_cluster(uint16_t cluster, uint8_t* buffer);
    static bool write_cluster(uint16_t cluster, const uint8_t* buffer);
    static FAT12DirectoryEntry* find_file(const char* filename);
    static FAT12DirectoryEntry* find_free_entry();

    static FAT12BootSector boot_sector;
    static uint8_t fat_table[4608];  
    static FAT12DirectoryEntry root_directory[224];  
    static bool mounted;
};

#endif
