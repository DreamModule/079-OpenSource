#include "fat12.h"
#include "string.h"
#include "io.h"

FAT12BootSector FAT12::boot_sector;
uint8_t FAT12::fat_table[4608];
FAT12DirectoryEntry FAT12::root_directory[224];
bool FAT12::mounted = false;

static inline void insw(uint16_t port, void* addr, uint32_t count) {
    asm volatile("cld; rep insw" : "+D"(addr), "+c"(count) : "d"(port) : "memory");
}

static inline void outsw(uint16_t port, const void* addr, uint32_t count) {
    asm volatile("cld; rep outsw" : "+S"(addr), "+c"(count) : "d"(port) : "memory");
}

static const int ATA_TIMEOUT = 100000; 


static bool ata_wait_ready() {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        if (!(inb(0x1F7) & 0x80)) return true;
        io_wait();
    }
    return false;
}


static bool ata_wait_data() {
    for (int i = 0; i < ATA_TIMEOUT; i++) {
        uint8_t status = inb(0x1F7);
        if (status & 0x08) return true;  
        if (status & 0x01) return false; 
        io_wait();
    }
    return false;
}


static bool ata_read_sector(uint32_t lba, uint8_t* buffer) {
    if (!ata_wait_ready()) return false;

    
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);  
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x20);  

    if (!ata_wait_data()) return false;

    
    insw(0x1F0, buffer, 256);  

    return true;
}


static bool ata_write_sector(uint32_t lba, const uint8_t* buffer) {
    if (!ata_wait_ready()) return false;

    
    outb(0x1F6, 0xE0 | ((lba >> 24) & 0x0F));
    outb(0x1F2, 1);  
    outb(0x1F3, (uint8_t)lba);
    outb(0x1F4, (uint8_t)(lba >> 8));
    outb(0x1F5, (uint8_t)(lba >> 16));
    outb(0x1F7, 0x30);  

    if (!ata_wait_data()) return false;

    
    outsw(0x1F0, buffer, 256);  

    
    outb(0x1F7, 0xE7);  
    if (!ata_wait_ready()) return false;

    return true;
}

void FAT12::initialize() {
    mounted = false;
}

bool FAT12::read_boot_sector() {
    uint8_t sector_buffer[512];

    if (!ata_read_sector(0, sector_buffer)) {
        return false;
    }

    memcpy(&boot_sector, sector_buffer, sizeof(FAT12BootSector));

    
    if (boot_sector.bytes_per_sector != 512) {
        return false;
    }

    return true;
}

bool FAT12::mount() {
    if (mounted) {
        return true;
    }

    
    if (!read_boot_sector()) {
        return false;
    }

    
    uint32_t fat_start = boot_sector.reserved_sectors;
    for (uint16_t i = 0; i < boot_sector.sectors_per_fat; i++) {
        if (!ata_read_sector(fat_start + i, fat_table + (i * 512))) {
            return false;
        }
    }

    
    uint32_t root_start = fat_start + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    uint32_t root_sectors = (boot_sector.root_entry_count * 32 + 511) / 512;

    for (uint16_t i = 0; i < root_sectors; i++) {
        if (!ata_read_sector(root_start + i, (uint8_t*)root_directory + (i * 512))) {
            return false;
        }
    }

    mounted = true;
    return true;
}

uint16_t FAT12::get_next_cluster(uint16_t cluster) {
    
    uint16_t fat_offset = cluster + (cluster / 2);  
    uint16_t fat_value = *(uint16_t*)(fat_table + fat_offset);

    if (cluster & 1) {
        
        fat_value >>= 4;
    } else {
        
        fat_value &= 0x0FFF;
    }

    
    if (fat_value >= 0xFF8) {
        return 0xFFFF;  
    }

    return fat_value;
}

bool FAT12::read_cluster(uint16_t cluster, uint8_t* buffer) {
    if (!mounted) {
        return false;
    }

    
    uint32_t fat_start = boot_sector.reserved_sectors;
    uint32_t root_start = fat_start + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    uint32_t root_sectors = (boot_sector.root_entry_count * 32 + 511) / 512;
    uint32_t data_start = root_start + root_sectors;

    
    uint32_t sector = data_start + ((cluster - 2) * boot_sector.sectors_per_cluster);

    
    for (uint8_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
        if (!ata_read_sector(sector + i, buffer + (i * 512))) {
            return false;
        }
    }

    return true;
}

FAT12DirectoryEntry* FAT12::find_file(const char* filename) {
    if (!mounted) {
        return nullptr;
    }

    
    char name[8];
    char ext[3];
    memset(name, ' ', 8);
    memset(ext, ' ', 3);

    int i = 0;
    while (filename[i] && filename[i] != '.' && i < 8) {
        name[i] = filename[i];
        if (name[i] >= 'a' && name[i] <= 'z') {
            name[i] -= 32;  
        }
        i++;
    }

    if (filename[i] == '.') {
        i++;
        int j = 0;
        while (filename[i] && j < 3) {
            ext[j] = filename[i];
            if (ext[j] >= 'a' && ext[j] <= 'z') {
                ext[j] -= 32;  
            }
            i++;
            j++;
        }
    }

    
    for (int i = 0; i < boot_sector.root_entry_count; i++) {
        FAT12DirectoryEntry* entry = &root_directory[i];

        
        if (entry->filename[0] == 0x00) {
            break;
        }

        
        if ((uint8_t)entry->filename[0] == 0xE5) {
            continue;
        }

        
        if (entry->attributes & (FAT12_ATTR_VOLUME_ID | FAT12_ATTR_DIRECTORY)) {
            continue;
        }

        
        bool match = true;
        for (int j = 0; j < 8; j++) {
            if (entry->filename[j] != name[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            for (int j = 0; j < 3; j++) {
                if (entry->extension[j] != ext[j]) {
                    match = false;
                    break;
                }
            }
        }

        if (match) {
            return entry;
        }
    }

    return nullptr;
}

bool FAT12::read_file(const char* filename, uint8_t* buffer, uint32_t max_size, uint32_t* bytes_read) {
    if (!mounted) {
        return false;
    }

    FAT12DirectoryEntry* entry = find_file(filename);
    if (!entry) {
        return false;
    }

    uint32_t file_size = entry->file_size;
    if (file_size > max_size) {
        file_size = max_size;
    }

    uint16_t cluster = entry->first_cluster;
    uint32_t bytes_copied = 0;
    uint32_t bytes_per_cluster = boot_sector.sectors_per_cluster * 512;
    uint8_t cluster_buffer[4096]; 

    while (cluster != 0xFFFF && bytes_copied < file_size) {
        if (!read_cluster(cluster, cluster_buffer)) {
            return false;
        }

        uint32_t bytes_to_copy = bytes_per_cluster;
        if (bytes_copied + bytes_to_copy > file_size) {
            bytes_to_copy = file_size - bytes_copied;
        }

        memcpy(buffer + bytes_copied, cluster_buffer, bytes_to_copy);
        bytes_copied += bytes_to_copy;

        cluster = get_next_cluster(cluster);
    }

    if (bytes_read) {
        *bytes_read = bytes_copied;
    }

    return true;
}

bool FAT12::list_directory() {
    if (!mounted) {
        return false;
    }

    
    
    return true;
}

FAT12DirectoryEntry* FAT12::get_root_directory() {
    if (!mounted) {
        return nullptr;
    }
    return root_directory;
}

uint16_t FAT12::get_root_entry_count() {
    if (!mounted) {
        return 0;
    }
    return boot_sector.root_entry_count;
}


bool FAT12::write_root_directory() {
    if (!mounted) {
        return false;
    }

    uint32_t fat_start = boot_sector.reserved_sectors;
    uint32_t root_start = fat_start + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    uint32_t root_sectors = (boot_sector.root_entry_count * 32 + 511) / 512;

    for (uint16_t i = 0; i < root_sectors; i++) {
        if (!ata_write_sector(root_start + i, (uint8_t*)root_directory + (i * 512))) {
            return false;
        }
    }

    return true;
}


bool FAT12::write_fat_table() {
    if (!mounted) {
        return false;
    }

    uint32_t fat_start = boot_sector.reserved_sectors;

    
    for (uint8_t fat_num = 0; fat_num < boot_sector.fat_count; fat_num++) {
        uint32_t fat_offset = fat_start + (fat_num * boot_sector.sectors_per_fat);
        for (uint16_t i = 0; i < boot_sector.sectors_per_fat; i++) {
            if (!ata_write_sector(fat_offset + i, fat_table + (i * 512))) {
                return false;
            }
        }
    }

    return true;
}


void FAT12::set_next_cluster(uint16_t cluster, uint16_t value) {
    uint16_t fat_offset = cluster + (cluster / 2);
    uint16_t* fat_entry = (uint16_t*)(fat_table + fat_offset);

    if (cluster & 1) {
        
        *fat_entry = (*fat_entry & 0x000F) | (value << 4);
    } else {
        
        *fat_entry = (*fat_entry & 0xF000) | value;
    }
}


uint16_t FAT12::find_free_cluster() {
    for (uint16_t cluster = 2; cluster < 4096; cluster++) {
        uint16_t next = get_next_cluster(cluster);
        if (next == 0) {
            return cluster;
        }
    }
    return 0xFFFF; 
}


bool FAT12::write_cluster(uint16_t cluster, const uint8_t* buffer) {
    if (!mounted) {
        return false;
    }

    uint32_t fat_start = boot_sector.reserved_sectors;
    uint32_t root_start = fat_start + (boot_sector.fat_count * boot_sector.sectors_per_fat);
    uint32_t root_sectors = (boot_sector.root_entry_count * 32 + 511) / 512;
    uint32_t data_start = root_start + root_sectors;

    uint32_t sector = data_start + ((cluster - 2) * boot_sector.sectors_per_cluster);

    for (uint8_t i = 0; i < boot_sector.sectors_per_cluster; i++) {
        if (!ata_write_sector(sector + i, buffer + (i * 512))) {
            return false;
        }
    }

    return true;
}


FAT12DirectoryEntry* FAT12::find_free_entry() {
    if (!mounted) {
        return nullptr;
    }

    for (int i = 0; i < boot_sector.root_entry_count; i++) {
        FAT12DirectoryEntry* entry = &root_directory[i];

        
        if (entry->filename[0] == 0x00 || (uint8_t)entry->filename[0] == 0xE5) {
            return entry;
        }
    }

    return nullptr;
}


bool FAT12::create_file(const char* filename) {
    if (!mounted) {
        return false;
    }

    
    if (find_file(filename)) {
        return false; 
    }

    
    FAT12DirectoryEntry* entry = find_free_entry();
    if (!entry) {
        return false; 
    }

    
    memset(entry->filename, ' ', 8);
    memset(entry->extension, ' ', 3);

    int i = 0;
    while (filename[i] && filename[i] != '.' && i < 8) {
        char c = filename[i];
        if (c >= 'a' && c <= 'z') c -= 32; 
        entry->filename[i] = c;
        i++;
    }

    if (filename[i] == '.') {
        i++;
        int j = 0;
        while (filename[i] && j < 3) {
            char c = filename[i];
            if (c >= 'a' && c <= 'z') c -= 32;
            entry->extension[j] = c;
            i++;
            j++;
        }
    }

    
    entry->attributes = 0x00; 
    entry->reserved = 0;
    entry->creation_time_ms = 0;
    entry->creation_time = 0;
    entry->creation_date = 0;
    entry->last_access_date = 0;
    entry->first_cluster_high = 0;
    entry->modification_time = 0;
    entry->modification_date = 0;
    entry->first_cluster = 0; 
    entry->file_size = 0;

    
    return write_root_directory();
}


bool FAT12::create_directory(const char* dirname) {
    if (!mounted) {
        return false;
    }

    
    if (find_file(dirname)) {
        return false;
    }

    FAT12DirectoryEntry* entry = find_free_entry();
    if (!entry) {
        return false;
    }

    
    memset(entry->filename, ' ', 8);
    memset(entry->extension, ' ', 3);

    int i = 0;
    while (dirname[i] && i < 8) {
        char c = dirname[i];
        if (c >= 'a' && c <= 'z') c -= 32;
        entry->filename[i] = c;
        i++;
    }

    
    entry->attributes = FAT12_ATTR_DIRECTORY;
    entry->reserved = 0;
    entry->first_cluster = 0;
    entry->file_size = 0;

    return write_root_directory();
}


bool FAT12::delete_file(const char* filename) {
    if (!mounted) {
        return false;
    }

    FAT12DirectoryEntry* entry = find_file(filename);
    if (!entry) {
        return false; 
    }

    
    if (entry->attributes & FAT12_ATTR_DIRECTORY) {
        return false;
    }

    
    entry->filename[0] = 0xE5;

    
    uint16_t cluster = entry->first_cluster;
    while (cluster < 0xFF8 && cluster != 0) {
        uint16_t next = get_next_cluster(cluster);
        set_next_cluster(cluster, 0); 
        cluster = next;
    }

    
    write_fat_table();
    return write_root_directory();
}


bool FAT12::delete_directory(const char* dirname) {
    if (!mounted) {
        return false;
    }

    FAT12DirectoryEntry* entry = find_file(dirname);
    if (!entry) {
        return false;
    }

    
    if (!(entry->attributes & FAT12_ATTR_DIRECTORY)) {
        return false;
    }

    
    entry->filename[0] = 0xE5;

    
    if (entry->first_cluster != 0) {
        set_next_cluster(entry->first_cluster, 0);
        write_fat_table();
    }

    return write_root_directory();
}


bool FAT12::write_file(const char* filename, const uint8_t* buffer, uint32_t size) {
    if (!mounted) {
        return false;
    }

    FAT12DirectoryEntry* entry = find_file(filename);
    if (!entry) {
        
        if (!create_file(filename)) {
            return false;
        }
        entry = find_file(filename);
        if (!entry) {
            return false;
        }
    }

    
    if (entry->first_cluster != 0) {
        uint16_t cluster = entry->first_cluster;
        while (cluster < 0xFF8 && cluster != 0) {
            uint16_t next = get_next_cluster(cluster);
            set_next_cluster(cluster, 0);
            cluster = next;
        }
        entry->first_cluster = 0;
    }

    
    uint32_t bytes_per_cluster = boot_sector.sectors_per_cluster * 512;
    uint32_t clusters_needed = (size + bytes_per_cluster - 1) / bytes_per_cluster;

    
    uint16_t first_cluster = find_free_cluster();
    if (first_cluster == 0xFFFF) {
        return false; 
    }

    entry->first_cluster = first_cluster;
    entry->file_size = size;

    uint16_t current_cluster = first_cluster;
    uint32_t bytes_written = 0;

    for (uint32_t i = 0; i < clusters_needed; i++) {
        
        
        uint8_t cluster_buffer[4096];
        memset(cluster_buffer, 0, bytes_per_cluster);

        uint32_t bytes_to_write = (size - bytes_written > bytes_per_cluster) ?
                                   bytes_per_cluster : (size - bytes_written);

        memcpy(cluster_buffer, buffer + bytes_written, bytes_to_write);

        if (!write_cluster(current_cluster, cluster_buffer)) {
            return false;
        }

        bytes_written += bytes_to_write;

        
        if (i < clusters_needed - 1) {
            uint16_t next_cluster = find_free_cluster();
            if (next_cluster == 0xFFFF) {
                return false;
            }
            set_next_cluster(current_cluster, next_cluster);
            current_cluster = next_cluster;
        } else {
            
            set_next_cluster(current_cluster, 0xFFF); 
        }
    }

    
    write_fat_table();
    return write_root_directory();
}
