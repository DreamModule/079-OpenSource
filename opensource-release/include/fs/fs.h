#ifndef FS_H
#define FS_H

#include "types.h"

namespace FileSystem {
    
    
    
    static const int MAX_FILE_CONTENT = 2048;

    enum FileType {
        TYPE_FILE,
        TYPE_DIRECTORY
    };

    enum FileExtension {
        EXT_NONE,
        EXT_TXT,
        EXT_CPP,
        EXT_C,
        EXT_H,
        EXT_ASM
    };

    struct FileNode {
        char name[64];
        FileType type;
        FileExtension extension;
        char content[MAX_FILE_CONTENT];  
        uint32_t content_size;
        FileNode* parent;
        FileNode* first_child;  
        FileNode* next_sibling;

        FileNode();
    };

    
    extern FileNode* current_dir;
    extern FileNode* root_dir;

    
    void initialize();

    
    FileNode* create_file(const char* name, FileExtension ext);
    FileNode* create_directory(const char* name);
    bool delete_node(const char* name);
    FileNode* find_node(const char* name);
    FileNode* find_node_in_dir(FileNode* dir, const char* name);

    
    bool change_directory(const char* path);
    bool go_to_parent();
    void get_current_path(char* buffer, int max_len);

    
    bool write_file(const char* name, const char* content);
    const char* read_file(const char* name);

    
    int list_directory(FileNode** results, int max_results);

    
    FileExtension get_extension_from_name(const char* name);
    const char* get_extension_string(FileExtension ext);
}

#endif
