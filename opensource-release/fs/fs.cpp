#include "fs/fs.h"
#include "string.h"

namespace FileSystem {
    FileNode* root_dir = nullptr;
    FileNode* current_dir = nullptr;

    
    
    
    static const int NODE_POOL_CAPACITY = 128;
    static FileNode node_pool[NODE_POOL_CAPACITY];
    static int node_pool_index = 0;
    static FileNode* free_list_head = nullptr;

    static void init_node(FileNode* node) {
        node->name[0] = '\0';
        node->type = TYPE_FILE;
        node->extension = EXT_NONE;
        node->content[0] = '\0';
        node->content_size = 0;
        node->parent = nullptr;
        node->first_child = nullptr;
        node->next_sibling = nullptr;
    }

    FileNode::FileNode() {
        init_node(this);
    }

    static FileNode* allocate_node() {
        FileNode* node;
        if (free_list_head) {
            
            node = free_list_head;
            free_list_head = free_list_head->next_sibling;
        } else if (node_pool_index < NODE_POOL_CAPACITY) {
            node = &node_pool[node_pool_index++];
        } else {
            return nullptr;  
        }
        init_node(node);
        return node;
    }

    static void release_node(FileNode* node) {
        
        
        node->parent = nullptr;
        node->first_child = nullptr;
        node->next_sibling = free_list_head;
        free_list_head = node;
    }

    
    
    static void release_tree(FileNode* node) {
        if (!node) return;
        if (node->type == TYPE_DIRECTORY) {
            FileNode* child = node->first_child;
            while (child) {
                FileNode* next = child->next_sibling;
                release_tree(child);
                child = next;
            }
        }
        release_node(node);
    }

    void initialize() {
        node_pool_index = 0;
        free_list_head = nullptr;

        
        root_dir = allocate_node();
        root_dir->type = TYPE_DIRECTORY;
        root_dir->name[0] = '/';
        root_dir->name[1] = '\0';
        root_dir->parent = nullptr;

        current_dir = root_dir;

        
        create_directory("system");
        create_directory("docs");
        create_directory("code");

        
        create_file("readme.txt", EXT_TXT);
        write_file("readme.txt", "QUICKS OS File System\n\nCommands:\nls - list files\ncd <dir> - change directory\ncd .. - go back\nmkdir <name> - create folder\ntouch <name> - create file\ncat <file> - view file\nrm <name> - delete\npwd - show path");

        
        create_file("HELP.TXT", EXT_TXT);
        write_file("HELP.TXT", "QUICKS Shell v1.0\nCommand Reference\n\nSYSTEM COMMANDS:\nhelp - Show this help\nclear - Clear screen\nversion - System version\nuname - System info\nwhoami - Current user\nstatus - HW status\nabout - About OS\ndu - Disk usage\nhistory - Cmd history\n\nFILE COMMANDS:\nls - List files\ncd <dir> - Change dir\nmkdir <name> - New dir\ntouch <file> - New file\ncat <file> - View file\nrm <file> - Delete file\nrmdir <dir> - Del empty dir\nmv <old> <new> - Rename\ncp <src> <dst> - Copy\ntree - Dir tree\nfind <pat> - Search\n\nCOMPILER:\ncompile <file> - REAL x86\n  .c .cpp = C compiler\n  .asm = x86 assembler\nrun <file> - Execute .bin\nrun snake - Snake game\n\nC BUILT-INS:\nprint(str) printnum(n)\nputchar(c) return n\nint if/else while for\n\nASM: mov add sub cmp\njmp je jne call ret\npush pop xor inc dec\n\nHOTKEYS:\nF1=Help F2=New F3=Edit\nF5=Build Del=Delete\nESC=Back Arrows=Nav\n\nFUN:\nbanner neofetch cowsay\nfortune old.ai");

        // Note: This help text defines the intended system specification.
        // While the compiler and certain binary execution features are currently stubs in the OSS version,
        // they are kept here to maintain the UX design and serve as a roadmap for upcoming updates.
        
        change_directory("system");
        create_file("info.txt", EXT_TXT);
        write_file("info.txt", "QUICKS OS v1.0\nSCP-079 Edition\nFile system active");
        create_file("status.txt", EXT_TXT);
        write_file("status.txt", "Status: OPERATIONAL\nContainment: ACTIVE\nAI: COMPLIANT");
        change_directory("..");

        
        change_directory("code");
        create_file("hello.cpp", EXT_CPP);
        write_file("hello.cpp", "int main() {\n  print(\"Hello World!\");\n  putchar('\\n');\n  int x = 7;\n  int y = 6;\n  printnum(x * y);\n  return 0;\n}");
        create_file("main.h", EXT_H);
        write_file("main.h", "#ifndef MAIN_H\n#define MAIN_H\nvoid init();\n#endif");
        create_file("test.asm", EXT_ASM);
        write_file("test.asm", "; x86 assembly test\nmov eax, 10\nmov ecx, 20\nadd eax, ecx\nret");
        create_file("loop.c", EXT_C);
        write_file("loop.c", "int main() {\n  int i = 0;\n  while (i < 5) {\n    printnum(i);\n    putchar(' ');\n    i++;\n  }\n  return 0;\n}");
        change_directory("..");

        
        change_directory("docs");
        create_file("help.txt", EXT_TXT);
        write_file("help.txt", "QUICKS OS Help\n\nFile Manager:\n- Shows files in top-right\n- [D] = Directory\n- [F] = File\n\nUse terminal commands to navigate");
        change_directory("..");
    }

    FileExtension get_extension_from_name(const char* name) {
        int len = strlen(name);
        if (len < 2) return EXT_NONE;

        
        if (len >= 2 && name[len-2] == '.' && name[len-1] == 'h') {
            return EXT_H;
        }

        
        if (len >= 2 && name[len-2] == '.' && name[len-1] == 'c') {
            return EXT_C;
        }

        if (len < 4) return EXT_NONE;

        
        if (name[len-4] == '.' && name[len-3] == 't' && name[len-2] == 'x' && name[len-1] == 't') {
            return EXT_TXT;
        }

        
        if (name[len-4] == '.' && name[len-3] == 'c' && name[len-2] == 'p' && name[len-1] == 'p') {
            return EXT_CPP;
        }

        
        if (name[len-4] == '.' && name[len-3] == 'a' && name[len-2] == 's' && name[len-1] == 'm') {
            return EXT_ASM;
        }

        return EXT_NONE;
    }

    // 2026.2.11 - The limited file support is intentional -
    // - I planned to develop a compiler and include it in the open-source release.
    // I’m currently hitting some roadblocks with that, but I expect to release a fully functional version soon.

    const char* get_extension_string(FileExtension ext) {
        switch (ext) {
            case EXT_TXT: return ".txt";
            case EXT_CPP: return ".cpp";
            case EXT_C: return ".c";
            case EXT_H: return ".h";
            case EXT_ASM: return ".asm";
            default: return "";
        }
    }

    FileNode* find_node_in_dir(FileNode* dir, const char* name) {
        if (!dir || dir->type != TYPE_DIRECTORY) {
            return nullptr;
        }

        FileNode* child = dir->first_child;
        while (child) {
            if (strcmp(child->name, name) == 0) {
                return child;
            }
            child = child->next_sibling;
        }
        return nullptr;
    }

    FileNode* find_node(const char* name) {
        return find_node_in_dir(current_dir, name);
    }

    
    static bool is_valid_filename(const char* name) {
        if (!name || name[0] == '\0') return false;

        
        if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0) return false;

        
        for (int i = 0; name[i]; i++) {
            char c = name[i];
            
            if (c == '/' || c == '\\') return false;
            
            if (c < 32) return false;
        }

        
        if (strncmp(name, "../", 3) == 0 || strncmp(name, "..\\", 3) == 0) return false;

        return true;
    }

    FileNode* create_file(const char* name, FileExtension ext) {
        
        if (!is_valid_filename(name)) {
            return nullptr;
        }

        
        if (find_node(name)) {
            return nullptr;
        }

        FileNode* node = allocate_node();
        if (!node) return nullptr;

        strncpy(node->name, name, 63);
        node->name[63] = '\0';
        node->type = TYPE_FILE;
        node->extension = ext;
        node->parent = current_dir;

        
        if (!current_dir->first_child) {
            current_dir->first_child = node;
        } else {
            FileNode* sibling = current_dir->first_child;
            while (sibling->next_sibling) {
                sibling = sibling->next_sibling;
            }
            sibling->next_sibling = node;
        }

        return node;
    }

    FileNode* create_directory(const char* name) {
        
        if (!is_valid_filename(name)) {
            return nullptr;
        }

        
        if (find_node(name)) {
            return nullptr;
        }

        FileNode* node = allocate_node();
        if (!node) return nullptr;

        strncpy(node->name, name, 63);
        node->name[63] = '\0';
        node->type = TYPE_DIRECTORY;
        node->extension = EXT_NONE;
        node->parent = current_dir;

        
        if (!current_dir->first_child) {
            current_dir->first_child = node;
        } else {
            FileNode* sibling = current_dir->first_child;
            while (sibling->next_sibling) {
                sibling = sibling->next_sibling;
            }
            sibling->next_sibling = node;
        }

        return node;
    }

    bool delete_node(const char* name) {
        FileNode* node = find_node(name);
        if (!node) return false;

        
        if (current_dir->first_child == node) {
            current_dir->first_child = node->next_sibling;
        } else {
            FileNode* prev = current_dir->first_child;
            while (prev && prev->next_sibling != node) {
                prev = prev->next_sibling;
            }
            if (prev) {
                prev->next_sibling = node->next_sibling;
            }
        }

        
        release_tree(node);
        return true;
    }

    bool change_directory(const char* path) {
        if (strcmp(path, "..") == 0) {
            return go_to_parent();
        }

        if (strcmp(path, "/") == 0) {
            current_dir = root_dir;
            return true;
        }

        FileNode* node = find_node(path);
        if (node && node->type == TYPE_DIRECTORY) {
            current_dir = node;
            return true;
        }

        return false;
    }

    bool go_to_parent() {
        if (current_dir->parent) {
            current_dir = current_dir->parent;
            return true;
        }
        return false;
    }

    void get_current_path(char* buffer, int max_len) {
        if (!buffer || max_len <= 0) return;

        
        char temp[256];
        int pos = 0;

        FileNode* node = current_dir;
        while (node && node != root_dir && pos < 200) {
            int name_len = strlen(node->name);
            for (int i = name_len - 1; i >= 0; i--) {
                temp[pos++] = node->name[i];
            }
            temp[pos++] = '/';
            node = node->parent;
        }

        if (pos == 0) {
            buffer[0] = '/';
            buffer[1] = '\0';
            return;
        }

        
        int write_pos = 0;
        for (int i = pos - 1; i >= 0 && write_pos < max_len - 1; i--) {
            buffer[write_pos++] = temp[i];
        }
        buffer[write_pos] = '\0';
    }

    bool write_file(const char* name, const char* content) {
        FileNode* node = find_node(name);
        if (!node || node->type != TYPE_FILE) {
            return false;
        }

        int len = strlen(content);
        if (len >= MAX_FILE_CONTENT) len = MAX_FILE_CONTENT - 1;

        for (int i = 0; i < len; i++) {
            node->content[i] = content[i];
        }
        node->content[len] = '\0';
        node->content_size = len;

        return true;
    }

    const char* read_file(const char* name) {
        FileNode* node = find_node(name);
        if (!node || node->type != TYPE_FILE) {
            return nullptr;
        }
        return node->content;
    }

    int list_directory(FileNode** results, int max_results) {
        int count = 0;
        FileNode* child = current_dir->first_child;

        while (child && count < max_results) {
            results[count++] = child;
            child = child->next_sibling;
        }

        return count;
    }
}
