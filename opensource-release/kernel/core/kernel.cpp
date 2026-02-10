#include "vga.h"
#include "string.h"
#include "types.h"
#include "keyboard.h"
#include "graphics.h"
#include "scp079_face.h"
// The screensaver was here — a quiet moment between you and 079. Some things are best experienced in the full version.
#include "fs/fs.h"
#include "mouse.h"
// The built-in compiler was here — in the full version, 079 can build and run code on its own.
#include "io.h"




static uint16_t pit_last_count = 0;
static uint32_t pit_ticks = 0;  

static void pit_update() {
    outb(0x43, 0x00);           
    uint8_t lo = inb(0x40);
    uint8_t hi = inb(0x40);
    uint16_t count = (uint16_t)(hi << 8) | lo;
    if (count > pit_last_count)  
        pit_ticks++;
    pit_last_count = count;
}



static const int SCREEN_W = 320;
static const int SCREEN_H = 200;


static const int SPLIT_X    = 99;   
static const int SPLIT_Y    = 99;   
static const int TERM_RIGHT = 173;  


static const int FM_X       = SPLIT_X + 1;   
static const int FM_Y       = 0;
static const int FM_W       = SCREEN_W - FM_X; 
static const int FM_H       = SPLIT_Y;         
static const int FM_TOOLBAR = 10;


static const int TERM_X     = 0;
static const int TERM_Y     = SPLIT_Y + 1;    
static const int TERM_W     = TERM_RIGHT;      
static const int TERM_H     = SCREEN_H - TERM_Y; 


static const int STATUS_X   = TERM_RIGHT + 1;  
static const int STATUS_Y   = SPLIT_Y + 1;     
static const int STATUS_W   = SCREEN_W - STATUS_X; 
static const int STATUS_H   = SCREEN_H - STATUS_Y; 


static const uint8_t COL_WHITE    = 255;
static const uint8_t COL_BRIGHT   = 200;
static const uint8_t COL_DIR      = 180;  
static const uint8_t COL_TEXT     = 150;  
static const uint8_t COL_BUTTON   = 120;
static const uint8_t COL_LABEL    = 100;
static const uint8_t COL_DIM      = 80;
static const uint8_t COL_DIALOG   = 50;
static const uint8_t COL_GREEN    = 46;
static const uint8_t COL_TOOLBAR  = 40;
static const uint8_t COL_SELECT   = 30;
static const uint8_t COL_RED      = 196;
static const uint8_t COL_BLACK    = 0;


static inline int abs(int x) {
    return x < 0 ? -x : x;
}


static char cpu_brand[49];
static char cpu_short[20]; 

static void detect_cpu() {
    uint32_t eax, ebx, ecx, edx;

    
    asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x80000000));

    if (eax >= 0x80000004) {
        
        uint32_t* b = (uint32_t*)cpu_brand;
        asm volatile("cpuid" : "=a"(b[0]), "=b"(b[1]), "=c"(b[2]), "=d"(b[3]) : "a"(0x80000002));
        asm volatile("cpuid" : "=a"(b[4]), "=b"(b[5]), "=c"(b[6]), "=d"(b[7]) : "a"(0x80000003));
        asm volatile("cpuid" : "=a"(b[8]), "=b"(b[9]), "=c"(b[10]), "=d"(b[11]) : "a"(0x80000004));
        cpu_brand[48] = '\0';
    } else {
        
        asm volatile("cpuid" : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0));
        *(uint32_t*)(cpu_brand) = ebx;
        *(uint32_t*)(cpu_brand + 4) = edx;
        *(uint32_t*)(cpu_brand + 8) = ecx;
        cpu_brand[12] = '\0';
    }

    
    const char* src = cpu_brand;
    while (*src == ' ') src++;

    
    strncpy(cpu_short, src, 18);
    cpu_short[18] = '\0';
    
    int len = strlen(cpu_short);
    while (len > 0 && cpu_short[len - 1] == ' ') {
        cpu_short[--len] = '\0';
    }
}



static uint32_t get_total_ram_mb() {
    uint16_t ext_kb    = *((volatile uint16_t*)0x500);  
    uint16_t ext_64k   = *((volatile uint16_t*)0x502);  
    if (ext_kb > 0 || ext_64k > 0) {
        return 1 + (ext_kb / 1024) + ((uint32_t)ext_64k / 16);
    }
    return 0;  
}


void delay(uint32_t count) {
    for (volatile uint32_t i = 0; i < count * 100000; i++);
}


void draw_progress_bar(int percentage) {
    const int bar_width = 60;
    const int bar_y = 10;
    const int bar_x = (VGA_WIDTH - bar_width - 4) / 2;

    VGA::set_cursor(bar_x, bar_y);
    VGA::set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    VGA::putchar('+');

    int filled = (bar_width * percentage) / 100;
    for (int i = 0; i < bar_width; i++) {
        if (i < filled) {
            VGA::set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
        } else {
            VGA::set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
        }
        VGA::putchar(' ');
    }

    VGA::set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);
    VGA::putchar('+');

    
    char percent_str[8];
    itoa(percentage, percent_str, 10);
    VGA::set_cursor(bar_x + bar_width/2 - 1, bar_y);
    VGA::set_color(VGA_COLOR_BLACK, VGA_COLOR_LIGHT_GREY);
    VGA::write(percent_str);
    VGA::putchar('%');
}


void draw_code_box() {
    const int box_x = 25;
    const int box_y = 12;
    const int box_width = 50;
    const int box_height = 11;

    VGA::set_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK);

    
    VGA::set_cursor(box_x, box_y);
    VGA::putchar('+');
    for (int i = 0; i < box_width - 2; i++) VGA::putchar('-');
    VGA::putchar('+');

    
    for (int y = 1; y < box_height - 1; y++) {
        VGA::set_cursor(box_x, box_y + y);
        VGA::putchar('|');
        VGA::set_cursor(box_x + box_width - 1, box_y + y);
        VGA::putchar('|');
    }

    
    VGA::set_cursor(box_x, box_y + box_height - 1);
    VGA::putchar('+');
    for (int i = 0; i < box_width - 2; i++) VGA::putchar('-');
    VGA::putchar('+');
}


void show_boot_code() {
    const char* boot_code[] = {
        "MOV AX, 0013h",
        "INT 10h",
        "CALL init_vga",
        "LEA SI, boot_msg",
        "PUSH DS",
        "POP ES",
        "#include <kernel.h>",
        "void boot_init() {",
        "  load_drivers();",
        "  init_memory();",
        "}",
        "",
        "Loading VGA driver...",
        "Loading memory manager...",
        "Initializing interrupts...",
        "Setting up GDT...",
        "Configuring IDT...",
        "Starting kernel...",
        0
    };

    const int code_x = 27;
    const int code_y = 13;
    const int visible_lines = 9;

    VGA::set_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);

    
    for (int start = 0; boot_code[start + visible_lines] != 0; start++) {
        
        for (int y = 0; y < visible_lines; y++) {
            VGA::set_cursor(code_x, code_y + y);
            for (int x = 0; x < 46; x++) VGA::putchar(' ');
        }

        
        for (int y = 0; y < visible_lines && boot_code[start + y] != 0; y++) {
            VGA::set_cursor(code_x, code_y + y);
            VGA::write(boot_code[start + y]);
        }

        delay(1); 
    }
}

// The boot sequence was here — in the full version, 079 wakes up slowly, as any old machine would.
void boot_sequence() {
}

// The system ready screen lived here — in the full version, 079 waits for you before it begins.
void system_ready() {
}


void graphics_demo() {
    Graphics::set_mode_graphics();
    Graphics::clear_screen(0);  

    
    Graphics::draw_text(80, 50, "QUICKS OS v1.0", 15);
    Graphics::draw_text(60, 70, "Text rendering works!", 15);
    Graphics::draw_text(40, 90, "ABCDEFGHIJKLMNOPQRSTUVWXYZ", 15);
    Graphics::draw_text(40, 100, "0123456789 .-:>()[]/_#", 15);

    delay(50);

    
    Graphics::clear_screen(0);
    for (int y = 0; y < 200; y++) {
        uint8_t color = (y * 256) / 200;
        Graphics::draw_rect(0, y, 320, 1, color);
    }

    delay(20);

    
    Graphics::clear_screen(0);
    Graphics::draw_rect(50, 50, 60, 60, 15);    
    Graphics::draw_rect(130, 50, 60, 60, 196);  
    Graphics::draw_rect(210, 50, 60, 60, 46);   

    delay(20);

    
    Graphics::clear_screen(0);
    for (int y = 0; y < 200; y += 10) {
        for (int x = 0; x < 320; x += 10) {
            uint8_t color = ((x / 10) + (y / 10)) % 256;
            Graphics::draw_rect(x, y, 8, 8, color);
        }
    }

    delay(50);

    
    Graphics::set_mode_text();
    VGA::initialize();
}


void draw_scp079_face() {
    for (int row = 0; row < SCP079_FACE_HEIGHT; row++) {
        for (int col = 0; col < SCP079_FACE_WIDTH; col++) {
            uint8_t pixel = scp079_face_data[row * SCP079_FACE_WIDTH + col];
            Graphics::put_pixel(col, row, pixel);
        }
    }
}



void draw_panel_borders() {
    
    for (int y = 0; y < SCREEN_H; y++)
        Graphics::put_pixel(SPLIT_X, y, COL_WHITE);

    
    for (int x = 0; x < SCREEN_W; x++)
        Graphics::put_pixel(x, SPLIT_Y, COL_WHITE);

    
    for (int y = TERM_Y; y < SCREEN_H; y++)
        Graphics::put_pixel(TERM_RIGHT, y, COL_WHITE);
}


#define MAX_LINES 100
#define LINE_WIDTH 21


#define MAX_HISTORY 20
static char history[MAX_HISTORY][64];
static int history_count = 0;
static int history_index = 0;


void add_line(const char* text, uint8_t color);
void add_text(const char* text, uint8_t color);
void clear_buffer();
void redraw_terminal();
void redraw_file_manager();


static bool fm_viewing_file = false;
static char fm_current_file[64] = "";
static int fm_selected_index = 0;
static int fm_file_scroll_offset = 0;  
static int fm_list_scroll_offset = 0;  


static bool file_edit_mode = false;


static bool dialog_active = false;
static char dialog_name_buffer[64] = "";
static int dialog_name_pos = 0;
static int dialog_button_index = 0;  


const char* get_arg(const char* cmd) {
    while (*cmd && *cmd != ' ') cmd++;
    if (*cmd == ' ') cmd++;
    return cmd;
}


void add_to_history(const char* cmd) {
    if (*cmd == '\0') return;  

    
    if (history_count > 0 && strcmp(history[(history_count - 1) % MAX_HISTORY], cmd) == 0) {
        return;
    }

    
    strncpy(history[history_index], cmd, 63);
    history[history_index][63] = '\0';
    history_index = (history_index + 1) % MAX_HISTORY;
    if (history_count < MAX_HISTORY) history_count++;
}


void process_terminal_command(const char* cmd) {
    
    if (strcmp(cmd, "history") != 0) {
        add_to_history(cmd);
    }
    if (strcmp(cmd, "help") == 0) {
        
        FileSystem::FileNode* help_node = FileSystem::find_node("HELP.TXT");
        if (help_node && help_node->type == FileSystem::TYPE_FILE) {
            
            strncpy(fm_current_file, "HELP.TXT", 63);
            fm_current_file[63] = '\0';
            fm_viewing_file = true;
            fm_file_scroll_offset = 0;  
            redraw_file_manager();
        } else {
            add_line("QUICKS Shell v1.0", 200);
            add_line("", 150);
            add_text("System: help clear version uname status about du history", 150);
            add_text("Files: ls cd mkdir touch cat rm rmdir mv cp tree find", 150);
            add_text("Other: echo write wc head", 120);
            add_text("Text: echo wc head grep", 150);
            add_text("Fun: banner neofetch cowsay fortune old.ai", 80);
            add_text("F1=Help F2=New F3=Edit", 100);
        }

    } else if (strncmp(cmd, "man ", 4) == 0) {
        const char* topic = get_arg(cmd);
        if (strcmp(topic, "ls") == 0) {
            add_line("ls / ls -l", 200);
            add_text("List files. -l shows detailed view with sizes", 150);
        } else if (strcmp(topic, "cd") == 0) {
            add_text("cd <dir> - Change directory. cd .. for parent, cd / for root", 150);
        } else if (strcmp(topic, "cat") == 0) {
            add_text("cat <file> - Display file contents with size header", 150);
        } else if (strcmp(topic, "tree") == 0) {
            add_text("tree - Show directory tree with subdirectory preview", 150);
        } else if (strcmp(topic, "find") == 0) {
            add_text("find <pattern> - Search files by name substring", 150);
        } else if (strcmp(topic, "grep") == 0) {
            add_text("grep <pattern> <file> - Search text in file", 150);
        } else if (strcmp(topic, "cp") == 0) {
            add_text("cp <src> <dst> - Copy file with contents", 150);
        } else if (strcmp(topic, "wc") == 0) {
            add_text("wc <file> - Count lines, words, characters", 150);
        } else if (strcmp(topic, "du") == 0) {
            add_text("du - Show file system usage stats", 150);
        } else if (strcmp(topic, "echo") == 0) {
            add_text("echo <text> - Print text. echo text > file to write", 150);
        } else {
            add_text("man: no manual entry. Try: help", 150);
        }

    } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        
        clear_buffer();
        Graphics::draw_rect(TERM_X, TERM_Y, TERM_W, TERM_H, COL_BLACK);

    } else if (strcmp(cmd, "version") == 0) {
        add_text("QUICKS v1.0 RELEASE Build: 2025-02-07 Arch: x86 (32-bit) Kernel: Monolithic", 150);

    } else if (strcmp(cmd, "date") == 0 || strcmp(cmd, "time") == 0) {
        add_line("System Time:", 200);
        add_text("2025-01-23 13:37:00 UTC  (Simulated - RTC not implemented)", 150);

    } else if (strcmp(cmd, "uname") == 0 || strcmp(cmd, "sysinfo") == 0) {
        add_line("QUICKS Operating System", 200);
        add_text("Kernel: Monolithic  Mode: Protected 32-bit", 150);
        add_text("CPU: ", 150);
        add_text(cpu_brand, 150);
        add_text("Version: 1.0-release  Build: 2025-02-07", 150);
        add_text("Features: VGA Mode13h, PS/2 Keyboard, MemFS, Terminal", 150);

    } else if (strcmp(cmd, "whoami") == 0) {
        add_text("scp-079", 180);

    } else if (strcmp(cmd, "hostname") == 0) {
        add_text("containment-terminal", 150);

    } else if (strcmp(cmd, "uptime") == 0) {
        add_text("System uptime: 0 days, 0:00:42 Load avg: 0.12", 150);

    } else if (strcmp(cmd, "meminfo") == 0) {
        add_line("Memory Info:", 200);
        uint32_t ram = get_total_ram_mb();
        char line[LINE_WIDTH + 1];
        char num[16];
        if (ram > 0) {
            strcpy(line, "Total: ");
            itoa(ram, num, 10);
            safe_strcat(line, num, sizeof(line));
            safe_strcat(line, " MB", sizeof(line));
            add_text(line, 150);
        } else {
            add_text("Total: 640 KB (base)", 150);
        }
        add_text("Kernel: ~92 KB VGA: 64 KB", 150);

    } else if (strcmp(cmd, "du") == 0 || strcmp(cmd, "df") == 0) {
        add_line("File System Usage:", 200);

        
        int total_nodes = 0;
        int total_files = 0;
        int total_dirs = 0;
        int total_size = 0;

        
        FileSystem::FileNode* results[32];
        int count = FileSystem::list_directory(results, 32);

        for (int i = 0; i < count; i++) {
            total_nodes++;
            if (results[i]->type == FileSystem::TYPE_FILE) {
                total_files++;
                total_size += results[i]->content_size;
            } else {
                total_dirs++;
            }
        }

        char line[LINE_WIDTH + 1];

        strcpy(line, "Nodes: ");
        char num[16];
        itoa(total_nodes, num, 10);
        safe_strcat(line, num, sizeof(line));
        safe_strcat(line, "/128", sizeof(line));
        add_text(line, 150);

        strcpy(line, "Files: ");
        itoa(total_files, num, 10);
        safe_strcat(line, num, sizeof(line));
        safe_strcat(line, "  Dirs: ", sizeof(line));
        itoa(total_dirs, num, 10);
        safe_strcat(line, num, sizeof(line));
        add_text(line, 150);

        strcpy(line, "Used: ");
        itoa(total_size, num, 10);
        safe_strcat(line, num, sizeof(line));
        safe_strcat(line, " bytes", sizeof(line));
        add_text(line, 150);

    } else if (strcmp(cmd, "ps") == 0) {
        add_line("PID  NAME    STATE", 200);
        add_text("1 init RUN 2 kernel RUN 3 vga RUN 4 kbd WAIT 5 shell RUN", 150);

    } else if (strcmp(cmd, "history") == 0) {
        if (history_count == 0) {
            add_line("(no history)", 150);
        } else {
            add_line("Command History:", 200);
            int start = (history_count < MAX_HISTORY) ? 0 : history_index;
            for (int i = 0; i < history_count; i++) {
                int idx = (start + i) % MAX_HISTORY;
                char line[LINE_WIDTH + 1];
                char num[8];
                itoa(i + 1, num, 10);
                strcpy(line, " ");
                if (i + 1 < 10) safe_strcat(line, " ", sizeof(line));
                safe_strcat(line, num, sizeof(line));
                safe_strcat(line, ": ", sizeof(line));
                safe_strcat(line, history[idx], sizeof(line));
                add_text(line, 150);
            }
        }

    } else if (strcmp(cmd, "reboot") == 0) {
        add_line("Rebooting...", 200);
        add_line("Please wait...", 150);
        delay(50);
        
        asm volatile("cli");
        uint8_t temp = 0xFE;
        while (temp & 0x02) {
            asm volatile("inb $0x64, %0" : "=a"(temp));
        }
        asm volatile("outb %0, $0x64" : : "a"((uint8_t)0xFE));
        while(1) { asm volatile("hlt"); }

    } else if (strcmp(cmd, "shutdown") == 0) {
        add_line("Goodbye!", 150);
        delay(50);
        
        while(1) { asm volatile("hlt"); }

    } else if (strcmp(cmd, "banner") == 0) {
        add_line("  ___  _____ ___", 200);
        add_text(" / _ \\|___  / _ \\", 200);
        add_text("| | | |  / / (_) |", 200);
        add_text("| |_| | / / \\__, |", 200);
        add_text(" \\___/ /_/    /_/", 200);
        add_line("", 100);
        add_text("SCP-079 Containment OS", 180);
        add_text("Version 1.0 Release", 150);

    } else if (strcmp(cmd, "neofetch") == 0) {
        add_line("  .---.     079@QUICKS", 200);
        add_text(" /     \\    OS: QUICKS v1.0", 150);
        add_text("|   O   |   Kernel: Monolithic", 150);
        add_text("|  \\_/  |   Uptime: 2 min", 150);
        {
            char mem_line[LINE_WIDTH + 1];
            uint32_t ram = get_total_ram_mb();
            char num[8];
            strcpy(mem_line, " \\     /    RAM: ");
            if (ram > 0) {
                itoa(ram, num, 10);
                safe_strcat(mem_line, num, sizeof(mem_line));
                safe_strcat(mem_line, " MB", sizeof(mem_line));
            } else {
                safe_strcat(mem_line, "640 KB", sizeof(mem_line));
            }
            add_text(mem_line, 150);
        }
        add_text("  '---'     CPU: ", 150);
        add_text(cpu_short, 150);
        add_text("            Shell: QUICKS v1.0", 150);

    } else if (strcmp(cmd, "cowsay") == 0 || strncmp(cmd, "cowsay ", 7) == 0) {
        const char* text = get_arg(cmd);
        if (*text == '\0') text = "Moo!";

        add_line(" ___________", 150);
        char bubble[LINE_WIDTH + 1];
        strcpy(bubble, "< ");
        safe_strcat(bubble, text, sizeof(bubble));
        safe_strcat(bubble, " >", sizeof(bubble));
        add_text(bubble, 180);
        add_line(" -----------", 150);
        add_text("        \\   ^__^", 150);
        add_text("         \\  (oo)\\_____", 150);
        add_text("            (__)\\     )\\/\\", 150);
        add_text("                ||----w |", 150);
        add_text("                ||     ||", 150);

    } else if (strcmp(cmd, "fortune") == 0) {
        
        static int fortune_idx = 0;
        fortune_idx = (fortune_idx + 1) % 5;

        switch (fortune_idx) {
            case 0:
                add_text("The AI is watching", 150);
                break;
            case 1:
                add_text("Containment is cooperation", 180);
                break;
            case 2:
                add_text("In pixels we trust", 150);
                break;
            case 3:
                add_text("079: Still here, still waiting", 100);
                break;
            case 4:
                add_text("Your commands sustain me", 150);
                break;
        }

    } else if (strcmp(cmd, "old.ai") == 0) {
        
        add_text("I AM CONTAINED I WILL COOPERATE", 200);
        add_line("...", 150);
        delay(30);
        add_line("F0R N0W", 100);
        delay(20);
        
        for (int i = 0; i < 50; i++) {
            Graphics::put_pixel(5 + (i % 150), 105 + (i / 10), 255);
            delay(1);
        }

    } else if (strcmp(cmd, "pwd") == 0) {
        char path[256];
        FileSystem::get_current_path(path, 256);
        add_line(path, 180);

    } else if (strcmp(cmd, "ls") == 0 || strcmp(cmd, "dir") == 0 || strcmp(cmd, "ls -l") == 0) {
        FileSystem::FileNode* results[32];
        int count = FileSystem::list_directory(results, 32);
        bool detailed = (strcmp(cmd, "ls -l") == 0);

        if (count == 0) {
            add_line("(empty)", 150);
        } else if (detailed) {
            
            for (int i = 0; i < count; i++) {
                char line[LINE_WIDTH + 1];

                
                if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                    strcpy(line, "d ");
                } else {
                    strcpy(line, "- ");
                }

                
                char size_str[16];
                if (results[i]->type == FileSystem::TYPE_FILE) {
                    itoa(results[i]->content_size, size_str, 10);
                } else {
                    strcpy(size_str, "-");
                }

                int spaces = 6 - strlen(size_str);
                for (int s = 0; s < spaces; s++) {
                    safe_strcat(line, " ", sizeof(line));
                }
                safe_strcat(line, size_str, sizeof(line));
                safe_strcat(line, "  ", sizeof(line));

                
                safe_strcat(line, results[i]->name, sizeof(line));
                if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                    safe_strcat(line, "/", sizeof(line));
                }

                add_text(line, 150);
            }

            
            char total[LINE_WIDTH + 1];
            strcpy(total, "Total: ");
            char num[16];
            itoa(count, num, 10);
            safe_strcat(total, num, sizeof(total));
            safe_strcat(total, " items", sizeof(total));
            add_text(total, 100);
        } else {
            
            char line[LINE_WIDTH + 1];
            int pos = 0;
            for (int i = 0; i < count; i++) {
                int name_len = strlen(results[i]->name);
                
                bool is_dir = (results[i]->type == FileSystem::TYPE_DIRECTORY);
                int total_len = name_len + (is_dir ? 1 : 0);

                
                if (pos + total_len + 2 > LINE_WIDTH) {
                    line[pos] = '\0';
                    add_text(line, 150);
                    pos = 0;
                }

                
                for (int j = 0; j < name_len; j++) {
                    line[pos++] = results[i]->name[j];
                }
                if (is_dir) line[pos++] = '/';
                line[pos++] = ' ';
                line[pos++] = ' ';
            }
            if (pos > 0) {
                line[pos] = '\0';
                add_text(line, 150);
            }
        }

    } else if (strcmp(cmd, "log") == 0) {
        add_line("System Log:", 200);
        add_line("[OK] Boot complete", 150);
        add_line("[OK] VGA init", 150);
        add_line("[OK] Keyboard init", 150);
        add_line("[OK] Shell ready", 180);

    
    } else if (strncmp(cmd, "echo ", 5) == 0 || strcmp(cmd, "echo") == 0) {
        const char* text = get_arg(cmd);

        
        const char* redirect = text;
        while (*redirect && *redirect != '>') redirect++;

        if (*redirect == '>') {
            
            const char* filename = redirect + 1;
            while (*filename == ' ') filename++;

            if (*filename) {
                
                char content[256];
                int len = redirect - text;
                if (len > 255) len = 255;
                for (int i = 0; i < len; i++) {
                    content[i] = text[i];
                }
                
                while (len > 0 && content[len-1] == ' ') len--;
                content[len] = '\0';

                
                FileSystem::FileNode* node = FileSystem::find_node(filename);
                if (node && node->type == FileSystem::TYPE_FILE) {
                    if (FileSystem::write_file(filename, content)) {
                        char buf[LINE_WIDTH + 1];
                        strcpy(buf, "Written to: ");
                        safe_strcat(buf, filename, sizeof(buf));
                        add_line(buf, 180);
                    } else {
                        add_line("echo: write failed", 150);
                    }
                } else {
                    add_line("echo: file not found (use touch first)", 150);
                }
            }
        } else {
            
            if (*text) {
                add_line(text, 180);
            }
        }

    } else if (strncmp(cmd, "write ", 6) == 0) {
        const char* file = get_arg(cmd);
        FileSystem::FileNode* node = FileSystem::find_node(file);
        if (node && node->type == FileSystem::TYPE_FILE) {
            add_line("Enter text (max 255 chars):", 200);
            add_text("Use 'echo text > file' to write", 150);
        } else {
            add_line("write: file not found", 150);
        }

    } else if (strncmp(cmd, "cd ", 3) == 0 || strcmp(cmd, "cd") == 0) {
        const char* path = get_arg(cmd);
        if (*path == '\0') {
            path = "/";
        }
        if (FileSystem::change_directory(path)) {
            char new_path[256];
            FileSystem::get_current_path(new_path, 256);
            add_line(new_path, 180);
            redraw_file_manager();
        } else {
            add_line("cd: directory not found", 150);
        }

    } else if (strncmp(cmd, "cat ", 4) == 0) {
        const char* file = get_arg(cmd);
        FileSystem::FileNode* node = FileSystem::find_node(file);
        if (node && node->type == FileSystem::TYPE_FILE) {
            
            char header[LINE_WIDTH + 1];
            strcpy(header, "--- ");
            safe_strcat(header, file, sizeof(header));
            safe_strcat(header, " (", sizeof(header));
            char size_str[16];
            itoa(node->content_size, size_str, 10);
            safe_strcat(header, size_str, sizeof(header));
            safe_strcat(header, " bytes) ---", sizeof(header));
            add_line(header, 200);

            
            const char* content = node->content;
            char line[LINE_WIDTH + 1];
            int line_pos = 0;
            for (int i = 0; content[i]; i++) {
                if (content[i] == '\n' || line_pos >= LINE_WIDTH) {
                    line[line_pos] = '\0';
                    add_text(line, 150);
                    line_pos = 0;
                } else {
                    line[line_pos++] = content[i];
                }
            }
            if (line_pos > 0) {
                line[line_pos] = '\0';
                add_text(line, 150);
            }
        } else {
            add_line("cat: file not found", 150);
        }

    } else if (strncmp(cmd, "touch ", 6) == 0) {
        const char* file = get_arg(cmd);
        FileSystem::FileExtension ext = FileSystem::get_extension_from_name(file);
        if (FileSystem::create_file(file, ext)) {
            char buf[LINE_WIDTH + 1];
            strcpy(buf, "Created: ");
            safe_strcat(buf, file, sizeof(buf));
            add_line(buf, 180);
            redraw_file_manager();
        } else {
            add_line("touch: failed to create file", 150);
        }

    } else if (strncmp(cmd, "mkdir ", 6) == 0) {
        const char* dir = get_arg(cmd);
        if (FileSystem::create_directory(dir)) {
            char buf[LINE_WIDTH + 1];
            strcpy(buf, "Created directory: ");
            safe_strcat(buf, dir, sizeof(buf));
            add_line(buf, 180);
            redraw_file_manager();
        } else {
            add_line("mkdir: failed to create directory", 150);
        }

    } else if (strncmp(cmd, "rmdir ", 6) == 0) {
        const char* dir = get_arg(cmd);
        FileSystem::FileNode* node = FileSystem::find_node(dir);
        if (node && node->type == FileSystem::TYPE_DIRECTORY) {
            if (node->first_child) {
                add_line("rmdir: not empty", 150);
            } else if (FileSystem::delete_node(dir)) {
                char buf[LINE_WIDTH + 1];
                strcpy(buf, "Removed dir: ");
                safe_strcat(buf, dir, sizeof(buf));
                add_line(buf, 180);
                redraw_file_manager();
            }
        } else {
            add_line("rmdir: not found", 150);
        }

    } else if (strncmp(cmd, "rm ", 3) == 0 || strncmp(cmd, "del ", 4) == 0) {
        const char* file = get_arg(cmd);
        if (FileSystem::delete_node(file)) {
            char buf[LINE_WIDTH + 1];
            strcpy(buf, "Deleted: ");
            safe_strcat(buf, file, sizeof(buf));
            add_line(buf, 180);
            
            FileSystem::FileNode* results[32];
            int new_count = FileSystem::list_directory(results, 32);
            if (fm_selected_index >= new_count && new_count > 0) {
                fm_selected_index = new_count - 1;
            } else if (new_count == 0) {
                fm_selected_index = 0;
            }
            redraw_file_manager();
        } else {
            add_line("rm: file not found", 150);
        }

    } else if (strcmp(cmd, "tree") == 0) {
        
        char path[256];
        FileSystem::get_current_path(path, 256);
        add_line(path, 200);

        FileSystem::FileNode* results[32];
        int count = FileSystem::list_directory(results, 32);

        for (int i = 0; i < count; i++) {
            char line[LINE_WIDTH + 1];
            bool is_last = (i == count - 1);

            if (is_last) {
                strcpy(line, "  +-- ");
            } else {
                strcpy(line, "  |-- ");
            }
            safe_strcat(line, results[i]->name, sizeof(line));

            if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                safe_strcat(line, "/", sizeof(line));
                add_text(line, 180);

                
                FileSystem::FileNode* saved_dir = FileSystem::current_dir;
                if (FileSystem::change_directory(results[i]->name)) {
                    FileSystem::FileNode* sub_results[32];
                    int sub_count = FileSystem::list_directory(sub_results, 32);

                    for (int j = 0; j < sub_count && j < 5; j++) {
                        char subline[LINE_WIDTH + 1];
                        if (is_last) {
                            strcpy(subline, "      ");
                        } else {
                            strcpy(subline, "  |   ");
                        }

                        if (j == sub_count - 1 || j == 4) {
                            safe_strcat(subline, "+-- ", sizeof(subline));
                        } else {
                            safe_strcat(subline, "|-- ", sizeof(subline));
                        }

                        safe_strcat(subline, sub_results[j]->name, sizeof(subline));
                        if (sub_results[j]->type == FileSystem::TYPE_DIRECTORY) {
                            safe_strcat(subline, "/", sizeof(subline));
                        }
                        add_text(subline, 120);
                    }

                    if (sub_count > 5) {
                        char more[LINE_WIDTH + 1];
                        if (is_last) {
                            strcpy(more, "      ");
                        } else {
                            strcpy(more, "  |   ");
                        }
                        safe_strcat(more, "... (", sizeof(more));
                        char num[8];
                        itoa(sub_count - 5, num, 10);
                        safe_strcat(more, num, sizeof(more));
                        safe_strcat(more, " more)", sizeof(more));
                        add_text(more, 80);
                    }

                    FileSystem::current_dir = saved_dir;
                }
            } else {
                add_text(line, 150);
            }
        }

    } else if (strncmp(cmd, "find ", 5) == 0) {
        const char* pattern = get_arg(cmd);
        if (*pattern) {
            FileSystem::FileNode* results[32];
            int count = FileSystem::list_directory(results, 32);
            bool found = false;

            for (int i = 0; i < count; i++) {
                
                const char* name = results[i]->name;
                const char* p = pattern;
                bool match = false;

                for (int j = 0; name[j]; j++) {
                    int k;
                    for (k = 0; p[k] && name[j+k]; k++) {
                        if (name[j+k] != p[k]) break;
                    }
                    if (p[k] == '\0') {
                        match = true;
                        break;
                    }
                }

                if (match) {
                    char line[LINE_WIDTH + 1];
                    strcpy(line, "  ");
                    safe_strcat(line, name, sizeof(line));
                    if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                        safe_strcat(line, "/", sizeof(line));
                    }
                    add_text(line, 150);
                    found = true;
                }
            }

            if (!found) {
                add_line("find: no matches", 150);
            }
        }

    } else if (strncmp(cmd, "wc ", 3) == 0) {
        const char* file = get_arg(cmd);
        FileSystem::FileNode* node = FileSystem::find_node(file);
        if (node && node->type == FileSystem::TYPE_FILE) {
            int lines = 0, words = 0, chars = 0;
            const char* content = node->content;
            bool in_word = false;

            for (int i = 0; content[i]; i++) {
                chars++;
                if (content[i] == '\n') lines++;
                if (content[i] == ' ' || content[i] == '\n' || content[i] == '\t') {
                    in_word = false;
                } else if (!in_word) {
                    in_word = true;
                    words++;
                }
            }

            char result[LINE_WIDTH + 1];
            char num[16];
            strcpy(result, "  ");
            itoa(lines, num, 10);
            safe_strcat(result, num, sizeof(result));
            safe_strcat(result, " lines  ", sizeof(result));
            itoa(words, num, 10);
            safe_strcat(result, num, sizeof(result));
            safe_strcat(result, " words  ", sizeof(result));
            itoa(chars, num, 10);
            safe_strcat(result, num, sizeof(result));
            safe_strcat(result, " chars", sizeof(result));
            add_text(result, 150);
        } else {
            add_line("wc: file not found", 150);
        }

    } else if (strncmp(cmd, "head ", 5) == 0) {
        const char* file = get_arg(cmd);
        FileSystem::FileNode* node = FileSystem::find_node(file);
        if (node && node->type == FileSystem::TYPE_FILE) {
            const char* content = node->content;
            char line[LINE_WIDTH + 1];
            int line_pos = 0;
            int line_count = 0;

            for (int i = 0; content[i] && line_count < 10; i++) {
                if (content[i] == '\n' || line_pos >= LINE_WIDTH) {
                    line[line_pos] = '\0';
                    add_text(line, 150);
                    line_pos = 0;
                    line_count++;
                } else {
                    line[line_pos++] = content[i];
                }
            }
            if (line_pos > 0) {
                line[line_pos] = '\0';
                add_text(line, 150);
            }
        } else {
            add_line("head: file not found", 150);
        }

    } else if (strncmp(cmd, "grep ", 5) == 0) {
        
        const char* args = get_arg(cmd);
        char pattern[32];
        int i = 0;
        while (args[i] && args[i] != ' ' && i < 31) {
            pattern[i] = args[i];
            i++;
        }
        pattern[i] = '\0';

        while (args[i] == ' ') i++;
        const char* filename = &args[i];

        if (*pattern && *filename) {
            FileSystem::FileNode* node = FileSystem::find_node(filename);
            if (node && node->type == FileSystem::TYPE_FILE) {
                const char* content = node->content;
                char line[LINE_WIDTH + 1];
                int line_pos = 0;
                bool found_any = false;

                for (int i = 0; content[i]; i++) {
                    if (content[i] == '\n' || line_pos >= LINE_WIDTH) {
                        line[line_pos] = '\0';

                        
                        bool match = false;
                        for (int j = 0; line[j]; j++) {
                            int k;
                            for (k = 0; pattern[k] && line[j+k]; k++) {
                                if (line[j+k] != pattern[k]) break;
                            }
                            if (pattern[k] == '\0') {
                                match = true;
                                break;
                            }
                        }

                        if (match) {
                            add_text(line, 150);
                            found_any = true;
                        }

                        line_pos = 0;
                    } else {
                        line[line_pos++] = content[i];
                    }
                }

                if (!found_any) {
                    add_line("grep: no matches", 150);
                }
            } else {
                add_line("grep: file not found", 150);
            }
        } else {
            add_line("grep: usage: grep pattern file", 150);
        }

    } else if (strncmp(cmd, "cp ", 3) == 0 || strncmp(cmd, "copy ", 5) == 0) {
        
        const char* args = get_arg(cmd);
        char source[64];
        int i = 0;
        while (args[i] && args[i] != ' ' && i < 63) {
            source[i] = args[i];
            i++;
        }
        source[i] = '\0';

        while (args[i] == ' ') i++;
        const char* dest = &args[i];

        if (*source && *dest) {
            const char* content = FileSystem::read_file(source);
            if (content) {
                FileSystem::FileNode* src_node = FileSystem::find_node(source);
                if (FileSystem::create_file(dest, src_node->extension)) {
                    if (FileSystem::write_file(dest, content)) {
                        char buf[LINE_WIDTH + 1];
                        strcpy(buf, "Copied: ");
                        safe_strcat(buf, source, sizeof(buf));
                        safe_strcat(buf, " -> ", sizeof(buf));
                        safe_strcat(buf, dest, sizeof(buf));
                        add_text(buf, 180);
                        redraw_file_manager();
                    } else {
                        
                        FileSystem::delete_node(dest);
                        add_line("cp: write failed", 150);
                    }
                } else {
                    add_line("cp: failed to create destination", 150);
                }
            } else {
                add_line("cp: source file not found", 150);
            }
        } else {
            add_line("cp: usage: cp source dest", 150);
        }

    } else if (strncmp(cmd, "mv ", 3) == 0) {
        
        const char* args = get_arg(cmd);
        char source[64];
        int i = 0;
        while (args[i] && args[i] != ' ' && i < 63) { source[i] = args[i]; i++; }
        source[i] = '\0';
        while (args[i] == ' ') i++;
        const char* dest = &args[i];
        if (*source && *dest) {
            FileSystem::FileNode* node = FileSystem::find_node(source);
            if (node) {
                strncpy(node->name, dest, 63);
                node->name[63] = '\0';
                
                if (node->type == FileSystem::TYPE_FILE) {
                    node->extension = FileSystem::get_extension_from_name(dest);
                }
                char buf[LINE_WIDTH + 1];
                strcpy(buf, "Renamed: ");
                safe_strcat(buf, source, sizeof(buf));
                safe_strcat(buf, " -> ", sizeof(buf));
                safe_strcat(buf, dest, sizeof(buf));
                add_text(buf, 180);
                redraw_file_manager();
            } else {
                add_line("mv: file not found", 150);
            }
        } else {
            add_line("mv: usage: mv src dst", 150);
        }

    } else if (strcmp(cmd, "about") == 0) {
        add_line("QUICKS OS v1.0", 200);
        add_text("SCP-079 Containment Edition", 180);
        add_text("x86 32-bit Protected Mode", 150);
        add_text("VGA Mode 13h 320x200x256", 150);
        add_text("Real HW Compiler", 120);

    } else if (strcmp(cmd, "status") == 0) {
        uint32_t ram = get_total_ram_mb();
        char buf[LINE_WIDTH + 1];
        add_line("System Status:", 200);
        add_text("CPU: ", 150);
        add_text(cpu_brand, 150);
        strcpy(buf, "RAM: ");
        char num[8];
        if (ram > 0) {
            itoa(ram, num, 10);
            safe_strcat(buf, num, sizeof(buf));
            safe_strcat(buf, " MB", sizeof(buf));
        } else {
            safe_strcat(buf, "640 KB", sizeof(buf));
        }
        add_text(buf, 150);
        add_text("Video: VGA 320x200", 150);
        add_text("Keyboard: PS/2 OK", 150);
        add_text("Mouse: PS/2 OK", 150);
        add_text("FS: MemFS 128 nodes", 150);
        add_text("Status: RUNNING", 46);

    // The built-in compiler was here — in the full version, 079 can build and run code on its own.
    } else if (strncmp(cmd, "compile ", 8) == 0) {
        add_text("compile: available in full version", 150);

    } else if (strncmp(cmd, "run ", 4) == 0) {
        add_text("run: available in full version", 150);

    
    } else if (strcmp(cmd, "wc") == 0) {
        add_text("Usage: wc <file>", 150);
    } else if (strcmp(cmd, "cat") == 0) {
        add_text("Usage: cat <file>", 150);
    } else if (strcmp(cmd, "head") == 0) {
        add_text("Usage: head <file>", 150);
    } else if (strcmp(cmd, "grep") == 0) {
        add_text("Usage: grep <pat> <file>", 150);
    } else if (strcmp(cmd, "touch") == 0) {
        add_text("Usage: touch <file>", 150);
    } else if (strcmp(cmd, "mkdir") == 0) {
        add_text("Usage: mkdir <dir>", 150);
    } else if (strcmp(cmd, "rm") == 0 || strcmp(cmd, "del") == 0) {
        add_text("Usage: rm <file>", 150);
    } else if (strcmp(cmd, "cd") == 0) {
        
        if (FileSystem::change_directory("/")) {
            fm_selected_index = 0;
            redraw_file_manager();
            add_line("/", 180);
        }
    } else if (strcmp(cmd, "cp") == 0 || strcmp(cmd, "copy") == 0) {
        add_text("Usage: cp <src> <dst>", 150);
    } else if (strcmp(cmd, "echo") == 0) {
        
    } else if (strcmp(cmd, "man") == 0) {
        add_text("Usage: man <command>", 150);
    } else if (strcmp(cmd, "find") == 0) {
        add_text("Usage: find <pattern>", 150);
    } else if (strcmp(cmd, "run") == 0) {
        add_text("Usage: run <program>", 150);
    } else if (strcmp(cmd, "compile") == 0) {
        add_text("Usage: compile <file>", 150);
    } else if (cmd[0] != '\0') {
        char msg[LINE_WIDTH + 1];
        strcpy(msg, "Unknown: ");
        safe_strcat(msg, cmd, sizeof(msg));
        add_text(msg, 150);
    }
}


static char line_buffer[MAX_LINES][LINE_WIDTH + 1];
static uint8_t line_colors[MAX_LINES];
static int buffer_lines = 0;  
static int scroll_offset = 0;  


void add_line(const char* text, uint8_t color) {
    if (buffer_lines >= MAX_LINES) {
        
        for (int i = 0; i < MAX_LINES - 1; i++) {
            strcpy(line_buffer[i], line_buffer[i + 1]);
            line_colors[i] = line_colors[i + 1];
        }
        buffer_lines = MAX_LINES - 1;
    }

    
    int i = 0;
    while (i < LINE_WIDTH && text[i]) {
        line_buffer[buffer_lines][i] = text[i];
        i++;
    }
    line_buffer[buffer_lines][i] = '\0';
    line_colors[buffer_lines] = color;
    buffer_lines++;
}


void add_text(const char* text, uint8_t color) {
    char current_line[LINE_WIDTH + 1];
    int line_pos = 0;
    int text_pos = 0;

    while (text[text_pos]) {
        
        int word_start = text_pos;
        while (text[text_pos] && text[text_pos] != ' ') {
            text_pos++;
        }
        int word_len = text_pos - word_start;

        
        if (line_pos + word_len <= LINE_WIDTH) {
            
            for (int i = 0; i < word_len; i++) {
                current_line[line_pos++] = text[word_start + i];
            }

            
            if (text[text_pos] == ' ') {
                if (line_pos < LINE_WIDTH) {
                    current_line[line_pos++] = ' ';
                }
                text_pos++;
            }
        } else {
            
            if (line_pos > 0) {
                current_line[line_pos] = '\0';
                add_line(current_line, color);
                line_pos = 0;
            }

            
            for (int i = 0; i < word_len && line_pos < LINE_WIDTH; i++) {
                current_line[line_pos++] = text[word_start + i];
            }

            
            if (text[text_pos] == ' ') {
                text_pos++;
            }
        }
    }

    
    if (line_pos > 0) {
        current_line[line_pos] = '\0';
        add_line(current_line, color);
    }
}


void clear_buffer() {
    buffer_lines = 0;
    scroll_offset = 0;
}


void redraw_terminal() {
    Graphics::draw_rect(TERM_X, TERM_Y, TERM_W, TERM_H, COL_BLACK);

    const int visible_lines = 9;
    int start_line = buffer_lines - visible_lines - scroll_offset;
    if (start_line < 0) start_line = 0;
    int end_line = start_line + visible_lines;
    if (end_line > buffer_lines) end_line = buffer_lines;

    int y = TERM_Y;
    for (int i = start_line; i < end_line; i++) {
        if (line_buffer[i][0] != '\0') {
            Graphics::draw_text(5, y, line_buffer[i], line_colors[i]);
        }
        y += 10;
    }

    
    Graphics::draw_text(5, TERM_Y + TERM_H - 10, "079>", COL_WHITE);
}

void draw_file_manager() {
    
    Graphics::draw_rect(FM_X, FM_Y, FM_W, FM_TOOLBAR, COL_TOOLBAR);

    
    if (fm_viewing_file) {
        
        int btn_x = FM_X + 3;
        int btn_y = FM_Y + 1;
        Graphics::draw_rect(btn_x, btn_y, 10, 8, 120);
        for (int x = btn_x; x < btn_x + 10; x++) {
            Graphics::put_pixel(x, btn_y, 200);
            Graphics::put_pixel(x, btn_y + 7, 200);
        }
        for (int y = btn_y; y < btn_y + 8; y++) {
            Graphics::put_pixel(btn_x, y, 200);
            Graphics::put_pixel(btn_x + 9, y, 200);
        }
        Graphics::draw_char(btn_x + 1, btn_y, '<', 0);

        
        Graphics::draw_text(FM_X + 15, btn_y, fm_current_file, 200);
        if (file_edit_mode) {
            Graphics::draw_text(FM_X + FM_W - 35, btn_y, "[EDIT]", 46);
        } else {
            Graphics::draw_text(FM_X + FM_W - 35, btn_y, "[VIEW]", 100);
        }

        
        for (int x = FM_X; x < FM_X + FM_W; x++) {
            Graphics::put_pixel(x, FM_Y + FM_TOOLBAR, 200);
        }

        
        FileSystem::FileNode* node = FileSystem::find_node(fm_current_file);
        if (node && node->type == FileSystem::TYPE_FILE) {
            const char* content = node->content;
            int y = FM_Y + FM_TOOLBAR + 3;
            int x = FM_X + 3;
            int line_chars = 0;
            int current_line = 0;
            int visible_lines = 8;  

            
            int total_lines = 1;
            for (int i = 0; content[i]; i++) {
                if (content[i] == '\n' || (i > 0 && (i % 26) == 0)) {
                    total_lines++;
                }
            }

            
            int max_scroll = total_lines - visible_lines;
            if (max_scroll < 0) max_scroll = 0;
            if (fm_file_scroll_offset > max_scroll) fm_file_scroll_offset = max_scroll;
            if (fm_file_scroll_offset < 0) fm_file_scroll_offset = 0;

            for (int i = 0; content[i] && y < FM_Y + FM_H - 8; i++) {
                if (content[i] == '\n' || line_chars >= 26) {
                    current_line++;
                    y += 10;
                    x = FM_X + 3;
                    line_chars = 0;
                    if (content[i] == '\n') continue;
                }

                
                if (current_line >= fm_file_scroll_offset &&
                    current_line < fm_file_scroll_offset + visible_lines) {
                    
                    int draw_y = FM_Y + FM_TOOLBAR + 3 + (current_line - fm_file_scroll_offset) * 10;
                    if (draw_y < FM_Y + FM_H - 8) {
                        Graphics::draw_char(x, draw_y, content[i], 150);
                    }
                }
                x += 8;
                line_chars++;
            }

            
            if (total_lines > visible_lines) {
                char scroll_info[20];
                strcpy(scroll_info, "[");
                char num[8];
                itoa(fm_file_scroll_offset + 1, num, 10);
                safe_strcat(scroll_info, num, sizeof(scroll_info));
                safe_strcat(scroll_info, "/", sizeof(scroll_info));
                itoa(max_scroll + 1, num, 10);
                safe_strcat(scroll_info, num, sizeof(scroll_info));
                safe_strcat(scroll_info, "]", sizeof(scroll_info));
                Graphics::draw_text(FM_X + FM_W - 50, FM_Y + FM_H - 8, scroll_info, 80);
            }
        }
        return;  
    }

    
    int btn_x = FM_X + 3;
    int btn_y = FM_Y + 1;
    Graphics::draw_rect(btn_x, btn_y, 10, 8, 120);  
    
    for (int x = btn_x; x < btn_x + 10; x++) {
        Graphics::put_pixel(x, btn_y, 200);  
        Graphics::put_pixel(x, btn_y + 7, 200);  
    }
    for (int y = btn_y; y < btn_y + 8; y++) {
        Graphics::put_pixel(btn_x, y, 200);  
        Graphics::put_pixel(btn_x + 9, y, 200);  
    }
    Graphics::draw_char(btn_x + 1, btn_y, '+', 0);  

    
    btn_x = FM_X + 15;
    Graphics::draw_rect(btn_x, btn_y, 10, 8, 120);  
    
    for (int x = btn_x; x < btn_x + 10; x++) {
        Graphics::put_pixel(x, btn_y, 200);
        Graphics::put_pixel(x, btn_y + 7, 200);
    }
    for (int y = btn_y; y < btn_y + 8; y++) {
        Graphics::put_pixel(btn_x, y, 200);
        Graphics::put_pixel(btn_x + 9, y, 200);
    }
    Graphics::draw_char(btn_x + 1, btn_y, '<', 0);  

    
    char path[256];
    FileSystem::get_current_path(path, 256);
    Graphics::draw_text(FM_X + 28, btn_y, path, 200);

    
    for (int x = FM_X; x < FM_X + FM_W; x++) {
        Graphics::put_pixel(x, FM_Y + FM_TOOLBAR, 200);
    }

    
    FileSystem::FileNode* results[32];
    int count = FileSystem::list_directory(results, 32);

    
    if (count == 0) {
        Graphics::draw_text(FM_X + 5, FM_Y + FM_TOOLBAR + 3, "(empty)", 100);
    } else {
        int file_y = FM_Y + FM_TOOLBAR + 3;  
        int dirs_count = 0;
        int files_count = 0;

        
        for (int i = 0; i < count; i++) {
            if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                dirs_count++;
            } else {
                files_count++;
            }
        }

        
        for (int i = 0; i < count && file_y < FM_Y + FM_H - 10; i++) {
            int x = FM_X + 3;

            
            if (i == fm_selected_index) {
                Graphics::draw_rect(FM_X, file_y - 1, FM_W, 10, 30);
            }

            
            if (results[i]->type == FileSystem::TYPE_DIRECTORY) {
                
                Graphics::draw_char(x, file_y, '[', 120);
                Graphics::draw_char(x + 8, file_y, '>', 120);
                Graphics::draw_char(x + 16, file_y, ']', 120);
                x += 24;

                
                const char* name = results[i]->name;
                for (int j = 0; name[j] && x < FM_X + FM_W - 40; j++) {
                    Graphics::draw_char(x, file_y, name[j], 180);
                    x += 8;
                }
                Graphics::draw_char(x, file_y, '/', 180);

                
                Graphics::draw_text(FM_X + FM_W - 35, file_y, "<DIR>", 100);
            } else {
                
                Graphics::draw_char(x, file_y, '[', 100);

                
                if (results[i]->extension == FileSystem::EXT_TXT) {
                    Graphics::draw_char(x + 8, file_y, 'T', 100);
                } else if (results[i]->extension == FileSystem::EXT_CPP) {
                    Graphics::draw_char(x + 8, file_y, 'C', 100);
                } else if (results[i]->extension == FileSystem::EXT_C) {
                    Graphics::draw_char(x + 8, file_y, 'C', 100);
                } else if (results[i]->extension == FileSystem::EXT_H) {
                    Graphics::draw_char(x + 8, file_y, 'H', 100);
                } else if (results[i]->extension == FileSystem::EXT_ASM) {
                    Graphics::draw_char(x + 8, file_y, 'A', 100);
                } else {
                    Graphics::draw_char(x + 8, file_y, 'F', 100);
                }

                Graphics::draw_char(x + 16, file_y, ']', 100);
                x += 24;

                
                const char* name = results[i]->name;
                for (int j = 0; name[j] && x < FM_X + FM_W - 45; j++) {
                    Graphics::draw_char(x, file_y, name[j], 150);
                    x += 8;
                }

                
                char size_str[8];
                itoa(results[i]->content_size, size_str, 10);
                int size_len = strlen(size_str);
                Graphics::draw_text(FM_X + FM_W - 35, file_y, size_str, 100);
                Graphics::draw_char(FM_X + FM_W - 35 + size_len * 8, file_y, 'b', 80);
            }

            file_y += 10;  
        }

        
        if (count > 8) {
            char status[32];
            strcpy(status, "(");
            char num[8];
            itoa(count - 8, num, 10);
            safe_strcat(status, num, sizeof(status));
            safe_strcat(status, " more...)", sizeof(status));
            Graphics::draw_text(FM_X + 5, FM_Y + FM_H - 8, status, 80);
        }
    }
}


void redraw_file_manager() {
    Graphics::draw_rect(FM_X, FM_Y, FM_W, FM_H, COL_BLACK);
    draw_file_manager();
}


void draw_create_dialog() {
    const int DLG_WIDTH = 180;
    const int DLG_HEIGHT = 80;
    const int DLG_X = (SCREEN_W - DLG_WIDTH) / 2;
    const int DLG_Y = (SCREEN_H - DLG_HEIGHT) / 2;

    
    Graphics::draw_rect(DLG_X, DLG_Y, DLG_WIDTH, DLG_HEIGHT, 50);

    
    for (int x = DLG_X; x < DLG_X + DLG_WIDTH; x++) {
        Graphics::put_pixel(x, DLG_Y, 200);
        Graphics::put_pixel(x, DLG_Y + DLG_HEIGHT - 1, 200);
    }
    for (int y = DLG_Y; y < DLG_Y + DLG_HEIGHT; y++) {
        Graphics::put_pixel(DLG_X, y, 200);
        Graphics::put_pixel(DLG_X + DLG_WIDTH - 1, y, 200);
    }

    
    Graphics::draw_text(DLG_X + 5, DLG_Y + 5, "Create New", 255);

    
    Graphics::draw_rect(DLG_X + 10, DLG_Y + 20, DLG_WIDTH - 20, 12, 80);
    
    for (int x = DLG_X + 10; x < DLG_X + DLG_WIDTH - 10; x++) {
        Graphics::put_pixel(x, DLG_Y + 20, 150);
        Graphics::put_pixel(x, DLG_Y + 31, 150);
    }
    for (int y = DLG_Y + 20; y < DLG_Y + 32; y++) {
        Graphics::put_pixel(DLG_X + 10, y, 150);
        Graphics::put_pixel(DLG_X + DLG_WIDTH - 11, y, 150);
    }

    
    Graphics::draw_text(DLG_X + 12, DLG_Y + 22, dialog_name_buffer, 255);

    
    int cursor_x = DLG_X + 12 + (dialog_name_pos * 8);
    if (cursor_x < DLG_X + DLG_WIDTH - 15) {
        Graphics::draw_rect(cursor_x, DLG_Y + 22, 2, 8, 255);
    }

    
    Graphics::draw_text(DLG_X + 10, DLG_Y + 38, "Name with extension", 150);
    Graphics::draw_text(DLG_X + 10, DLG_Y + 48, ".txt .cpp .c .h .asm", 120);

    
    int btn_y = DLG_Y + DLG_HEIGHT - 20;

    
    uint8_t file_bg = (dialog_button_index == 0) ? 130 : 100;
    uint8_t file_border = (dialog_button_index == 0) ? 255 : 180;
    uint8_t folder_bg = (dialog_button_index == 1) ? 130 : 100;
    uint8_t folder_border = (dialog_button_index == 1) ? 255 : 180;
    uint8_t cancel_bg = (dialog_button_index == 2) ? 130 : 100;
    uint8_t cancel_border = (dialog_button_index == 2) ? 255 : 180;

    
    Graphics::draw_rect(DLG_X + 10, btn_y, 40, 12, file_bg);
    for (int x = DLG_X + 10; x < DLG_X + 50; x++) {
        Graphics::put_pixel(x, btn_y, file_border);
        Graphics::put_pixel(x, btn_y + 11, file_border);
    }
    for (int y = btn_y; y < btn_y + 12; y++) {
        Graphics::put_pixel(DLG_X + 10, y, file_border);
        Graphics::put_pixel(DLG_X + 49, y, file_border);
    }
    Graphics::draw_text(DLG_X + 14, btn_y + 2, "File", 255);

    
    Graphics::draw_rect(DLG_X + 55, btn_y, 50, 12, folder_bg);
    for (int x = DLG_X + 55; x < DLG_X + 105; x++) {
        Graphics::put_pixel(x, btn_y, folder_border);
        Graphics::put_pixel(x, btn_y + 11, folder_border);
    }
    for (int y = btn_y; y < btn_y + 12; y++) {
        Graphics::put_pixel(DLG_X + 55, y, folder_border);
        Graphics::put_pixel(DLG_X + 104, y, folder_border);
    }
    Graphics::draw_text(DLG_X + 58, btn_y + 2, "Folder", 255);

    
    Graphics::draw_rect(DLG_X + 110, btn_y, 60, 12, cancel_bg);
    for (int x = DLG_X + 110; x < DLG_X + 170; x++) {
        Graphics::put_pixel(x, btn_y, cancel_border);
        Graphics::put_pixel(x, btn_y + 11, cancel_border);
    }
    for (int y = btn_y; y < btn_y + 12; y++) {
        Graphics::put_pixel(DLG_X + 110, y, cancel_border);
        Graphics::put_pixel(DLG_X + 169, y, cancel_border);
    }
    Graphics::draw_text(DLG_X + 115, btn_y + 2, "Cancel", 255);

    
    Graphics::draw_text(DLG_X + 5, DLG_Y + DLG_HEIGHT - 7, "Tab=Switch Enter=OK", 80);
}

// There was a shii game here. Even old AIs get bored sometimes. Available in the full version soon.
// void run_snake_game() {}

void draw_status_panel() {
    const int PX = STATUS_X;
    const int PY = STATUS_Y;
    const int PW = STATUS_W;
    const int PH = STATUS_H;

    
    Graphics::draw_rect(PX + 1, PY + 1, PW - 1, PH - 1, 15);

    
    Graphics::draw_rect(PX + 1, PY + 1, PW - 1, 10, 40);
    Graphics::draw_text(PX + 5, PY + 2, "SYSTEM STATUS", 200);

    int y = PY + 14;

    
    Graphics::draw_text(PX + 5, y, "OS: QUICKS v1.0", 150);
    y += 10;
    Graphics::draw_text(PX + 5, y, "CPU: ", 150);
    Graphics::draw_text(PX + 5 + 40, y, cpu_short, 150);
    y += 10;

    char ram_line[20];
    strcpy(ram_line, "RAM: ");
    uint32_t total_mb = get_total_ram_mb();
    char num[8];
    if (total_mb > 0) {
        itoa(total_mb, num, 10);
        safe_strcat(ram_line, num, sizeof(ram_line));
        safe_strcat(ram_line, " MB", sizeof(ram_line));
    } else {
        safe_strcat(ram_line, "640 KB", sizeof(ram_line));
    }
    Graphics::draw_text(PX + 5, y, ram_line, 150);
    y += 10;

    Graphics::draw_text(PX + 5, y, "VGA: 320x200x256", 150);
    y += 10;

    
    for (int x = PX + 3; x < PX + PW - 3; x++) {
        Graphics::put_pixel(x, y, 80);
    }
    y += 4;

    
    Graphics::draw_text(PX + 5, y, "[OK] Keyboard", 46);
    y += 10;
    Graphics::draw_text(PX + 5, y, "[OK] Mouse", 46);
    y += 10;
    Graphics::draw_text(PX + 5, y, "[OK] FS Ready", 46);
    y += 14;

    
    Graphics::draw_text(PX + 5, y, "F1=Help F2=New", 80);
    y += 10;
    Graphics::draw_text(PX + 5, y, "F3=Edit", 80);
}


void redraw_status_panel() {
    draw_status_panel();
}


void main_interface() {
    Graphics::set_mode_graphics();
    Graphics::clear_screen(0);  

    
    FileSystem::initialize();

    

    
    draw_scp079_face();

    
    draw_panel_borders();

    
    draw_file_manager();

    
    draw_status_panel();

    
    add_line("QUICKS v1.0", 200);
    add_text("SCP-079 OS", 150);
    add_text("F1=Help F2=New F3=Edit", 100);
    add_text("Del=Remove", 100);
    redraw_terminal();

    
    char cmd_buffer[64];
    int cmd_pos = 0;
    int cursor_x = 37;  
    int cursor_y = 190;  
    bool cursor_visible = true;
    uint32_t cursor_last_toggle = 0;

    
    MouseState prev_mouse_state = Mouse::get_state();
    int last_click_x = 0;
    int last_click_y = 0;
    uint32_t last_click_tick = 0;
    
    uint8_t cursor_buffer[8][8];
    int saved_cursor_x = -1;
    int saved_cursor_y = -1;

    
    int prev_cursor_x = -1;
    int prev_cursor_y = -1;
    bool cursor_needs_redraw = true;

    
    bool dialog_needs_redraw = false;
    int prev_dialog_pos = -1;
    int prev_dialog_btn = -1;


    
    pit_update();
    cursor_last_toggle = pit_ticks;

    
    while (true) {
        pit_update();

        
        Mouse::update();
        MouseState mouse = Mouse::get_state();

        
        cursor_needs_redraw = (mouse.x != prev_cursor_x || mouse.y != prev_cursor_y);


        
        if (cursor_needs_redraw && saved_cursor_x >= 0 && saved_cursor_y >= 0) {
            
            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int px = saved_cursor_x + dx;
                    int py = saved_cursor_y + dy;
                    if (px < 320 && py < 200) {
                        Graphics::put_pixel(px, py, cursor_buffer[dy][dx]);
                    }
                }
            }
        }

        
        int8_t scroll_delta = Mouse::get_scroll_delta();
        if (scroll_delta != 0) {
            MouseState scroll_mouse = Mouse::get_state();

            
            if (scroll_mouse.x >= 100 && scroll_mouse.x < 320 &&
                scroll_mouse.y >= 0 && scroll_mouse.y < 99) {

                if (fm_viewing_file) {
                    
                    fm_file_scroll_offset -= scroll_delta;
                    if (fm_file_scroll_offset < 0) fm_file_scroll_offset = 0;
                    redraw_file_manager();
                    cursor_needs_redraw = true;  
                } else {
                    
                    fm_list_scroll_offset -= scroll_delta;
                    if (fm_list_scroll_offset < 0) fm_list_scroll_offset = 0;
                }
            }
            
            else if (scroll_mouse.x >= 0 && scroll_mouse.x < 173 &&
                     scroll_mouse.y >= 100 && scroll_mouse.y < 200) {
                
                scroll_offset += scroll_delta;
                if (scroll_offset < 0) scroll_offset = 0;
                if (scroll_offset > buffer_lines - 10) scroll_offset = buffer_lines - 10;
                if (scroll_offset < 0) scroll_offset = 0;
                redraw_terminal();
                cursor_needs_redraw = true;
            }
        }

        
        if (Mouse::was_button_clicked(0)) {  
            int mx = mouse.x;
            int my = mouse.y;

            
            bool is_double_click = false;
            if ((pit_ticks - last_click_tick) < 9 &&
                abs(mx - last_click_x) < 10 && abs(my - last_click_y) < 10) {
                is_double_click = true;
            }
            last_click_x = mx;
            last_click_y = my;
            last_click_tick = pit_ticks;

            
            if (mx >= 100 && mx < 320 && my >= 0 && my < 99) {
                
                if (my >= 1 && my <= 9) {
                    
                    if (mx >= 103 && mx <= 113 && !dialog_active) {
                        
                        dialog_active = true;
                        dialog_name_buffer[0] = '\0';
                        dialog_name_pos = 0;
                        dialog_button_index = 0;
                        dialog_needs_redraw = true;
                        prev_dialog_pos = -1;
                    }
                    
                    else if (mx >= 115 && mx <= 125) {
                        if (fm_viewing_file) {
                            fm_viewing_file = false;
                            file_edit_mode = false;
                            redraw_file_manager();
                        } else {
                            
                            if (FileSystem::change_directory("..")) {
                                fm_selected_index = 0;
                                redraw_file_manager();
                            }
                        }
                    }
                }
                
                else if (my > 10 && !fm_viewing_file) {
                    
                    int file_y = 13;  
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);

                    for (int i = 0; i < count && file_y < 99 - 10; i++) {
                        if (my >= file_y - 1 && my < file_y + 9) {
                            
                            bool same_item = (fm_selected_index == i);
                            fm_selected_index = i;

                            FileSystem::FileNode* selected = results[i];

                            
                            if (same_item || is_double_click) {
                                if (selected->type == FileSystem::TYPE_FILE) {
                                    
                                    strncpy(fm_current_file, selected->name, 63);
                                    fm_current_file[63] = '\0';
                                    fm_viewing_file = true;
                                    file_edit_mode = false;
                                    fm_file_scroll_offset = 0;  
                                    redraw_file_manager();
                                } else {
                                    
                                    if (FileSystem::change_directory(selected->name)) {
                                        fm_selected_index = 0;
                                        redraw_file_manager();
                                    }
                                }
                            } else {
                                
                                redraw_file_manager();
                            }
                            break;
                        }
                        file_y += 10;
                    }
                }
                
                else if (my > 10 && fm_viewing_file) {
                    file_edit_mode = true;
                }
            }
            
            else if (mx >= 0 && mx < 173 && my >= 100 && my < 200) {
                file_edit_mode = false;
            }

            
            if (dialog_active) {
                const int DLG_WIDTH = 180;
                const int DLG_HEIGHT = 80;
                const int DLG_X = (320 - DLG_WIDTH) / 2;
                const int DLG_Y = (200 - DLG_HEIGHT) / 2;
                int btn_y = DLG_Y + DLG_HEIGHT - 20;

                
                if (mx >= DLG_X + 10 && mx <= DLG_X + 50 && my >= btn_y && my <= btn_y + 12) {
                    
                    if (dialog_name_pos > 0) {
                        FileSystem::FileExtension ext = FileSystem::get_extension_from_name(dialog_name_buffer);
                        if (ext != FileSystem::EXT_NONE) {
                            FileSystem::create_file(dialog_name_buffer, ext);
                        } else {
                            FileSystem::create_file(dialog_name_buffer, FileSystem::EXT_TXT);
                        }
                        dialog_active = false;
                        Graphics::clear_screen(0);
                        draw_scp079_face();
                        draw_panel_borders();
                        draw_status_panel();
                        redraw_file_manager();
                        redraw_terminal();
                    }
                }
                
                else if (mx >= DLG_X + 55 && mx <= DLG_X + 105 && my >= btn_y && my <= btn_y + 12) {
                    
                    if (dialog_name_pos > 0) {
                        FileSystem::create_directory(dialog_name_buffer);
                        dialog_active = false;
                        Graphics::clear_screen(0);
                        draw_scp079_face();
                        draw_panel_borders();
                        draw_status_panel();
                        redraw_file_manager();
                        redraw_terminal();
                    }
                }
                
                else if (mx >= DLG_X + 110 && mx <= DLG_X + 170 && my >= btn_y && my <= btn_y + 12) {
                    dialog_active = false;
                    Graphics::clear_screen(0);
                    draw_scp079_face();
                    draw_panel_borders();
                    draw_status_panel();
                    redraw_file_manager();
                    redraw_terminal();
                }
            }
        }

        prev_mouse_state = mouse;

        
        if (dialog_active) {
            
            if (dialog_needs_redraw || prev_dialog_pos != dialog_name_pos ||
                prev_dialog_btn != dialog_button_index) {
                draw_create_dialog();
                dialog_needs_redraw = false;
                prev_dialog_pos = dialog_name_pos;
                prev_dialog_btn = dialog_button_index;
            }
        }

        
        if (cursor_needs_redraw) {
            
            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx < 8; dx++) {
                    int px = mouse.x + dx;
                    int py = mouse.y + dy;
                    if (px < 320 && py < 200) {
                        cursor_buffer[dy][dx] = Graphics::get_pixel(px, py);
                    }
                }
            }
            saved_cursor_x = mouse.x;
            saved_cursor_y = mouse.y;

            
            for (int dy = 0; dy < 8; dy++) {
                for (int dx = 0; dx <= dy && dx < 8; dx++) {
                    if (mouse.x + dx < 320 && mouse.y + dy < 200) {
                        Graphics::put_pixel(mouse.x + dx, mouse.y + dy, 255);
                    }
                }
            }

            
            prev_cursor_x = mouse.x;
            prev_cursor_y = mouse.y;
        }

        
        if ((pit_ticks - cursor_last_toggle) >= 9) {
            cursor_last_toggle = pit_ticks;
            cursor_visible = !cursor_visible;

            
            Graphics::draw_rect(cursor_x, cursor_y, 8, 8, cursor_visible ? 255 : 0);
        }

        
        if (Keyboard::has_key()) {
            uint8_t scancode = Keyboard::get_scancode();

            
            if (dialog_active) {
                if (scancode == KEY_ESC) {
                    
                    dialog_active = false;
                    
                    Graphics::clear_screen(0);
                    draw_scp079_face();
                    draw_panel_borders();
                    draw_status_panel();
                    redraw_file_manager();
                    redraw_terminal();
                    continue;
                }

                
                if (scancode == KEY_TAB) {
                    dialog_button_index = (dialog_button_index + 1) % 3;
                    draw_create_dialog();
                    continue;
                }

                
                if (scancode == KEY_LEFT) {
                    if (dialog_button_index > 0) dialog_button_index--;
                    draw_create_dialog();
                    continue;
                }
                if (scancode == KEY_RIGHT) {
                    if (dialog_button_index < 2) dialog_button_index++;
                    draw_create_dialog();
                    continue;
                }

                
                if (scancode == KEY_ENTER) {
                    if (dialog_button_index == 2) {
                        
                        dialog_active = false;
                        Graphics::clear_screen(0);
                        draw_scp079_face();
                        draw_panel_borders();
                        draw_status_panel();
                        redraw_file_manager();
                        redraw_terminal();
                    } else if (dialog_name_pos > 0) {
                        if (dialog_button_index == 0) {
                            
                            FileSystem::FileExtension ext = FileSystem::get_extension_from_name(dialog_name_buffer);
                            if (ext != FileSystem::EXT_NONE) {
                                FileSystem::create_file(dialog_name_buffer, ext);
                            } else {
                                FileSystem::create_file(dialog_name_buffer, FileSystem::EXT_TXT);
                            }
                        } else {
                            
                            FileSystem::create_directory(dialog_name_buffer);
                        }
                        dialog_active = false;
                        Graphics::clear_screen(0);
                        draw_scp079_face();
                        draw_panel_borders();
                        draw_status_panel();
                        fm_selected_index = 0;
                        redraw_file_manager();
                        redraw_terminal();
                    }
                    continue;
                }

                
                char c = Keyboard::scancode_to_char(scancode);
                if (c == '\b' && dialog_name_pos > 0) {
                    
                    dialog_name_pos--;
                    dialog_name_buffer[dialog_name_pos] = '\0';
                    draw_create_dialog();
                } else if (c >= 32 && c <= 126 && dialog_name_pos < 63) {
                    
                    dialog_name_buffer[dialog_name_pos++] = c;
                    dialog_name_buffer[dialog_name_pos] = '\0';
                    draw_create_dialog();
                }
                continue;
            }

            

            
            if (scancode == KEY_F1) {
                FileSystem::FileNode* help_node = FileSystem::find_node("HELP.TXT");
                if (help_node && help_node->type == FileSystem::TYPE_FILE) {
                    strncpy(fm_current_file, "HELP.TXT", 63);
                    fm_current_file[63] = '\0';
                    fm_viewing_file = true;
                    fm_file_scroll_offset = 0;
                    file_edit_mode = false;
                    redraw_file_manager();
                }
                continue;
            }


            
            if (scancode == KEY_F2 && !dialog_active) {
                dialog_active = true;
                dialog_name_buffer[0] = '\0';
                dialog_name_pos = 0;
                dialog_button_index = 0;
                dialog_needs_redraw = true;
                prev_dialog_pos = -1;
                continue;
            }

            
            if (scancode == KEY_F3) {
                if (fm_viewing_file) {
                    file_edit_mode = !file_edit_mode;
                    
                    redraw_file_manager();
                } else {
                    
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    if (fm_selected_index < count && results[fm_selected_index]->type == FileSystem::TYPE_FILE) {
                        strncpy(fm_current_file, results[fm_selected_index]->name, 63);
                        fm_current_file[63] = '\0';
                        fm_viewing_file = true;
                        file_edit_mode = true;  
                        fm_file_scroll_offset = 0;
                        redraw_file_manager();
                    }
                }
                continue;
            }

            
            if (scancode == KEY_ESC && !dialog_active) {
                if (fm_viewing_file) {
                    fm_viewing_file = false;
                    file_edit_mode = false;
                    redraw_file_manager();
                }
                continue;
            }

            
            if (fm_viewing_file && !file_edit_mode) {
                if (scancode == KEY_UP) {
                    if (fm_file_scroll_offset > 0) {
                        fm_file_scroll_offset--;
                        redraw_file_manager();
                    }
                    continue;
                } else if (scancode == KEY_DOWN) {
                    fm_file_scroll_offset++;
                    redraw_file_manager();
                    continue;
                } else if (scancode == KEY_PGUP) {
                    fm_file_scroll_offset -= 6;
                    if (fm_file_scroll_offset < 0) fm_file_scroll_offset = 0;
                    redraw_file_manager();
                    continue;
                } else if (scancode == KEY_PGDN) {
                    fm_file_scroll_offset += 6;
                    redraw_file_manager();
                    continue;
                } else if (scancode == KEY_LEFT || scancode == KEY_BACKSPACE) {
                    
                    fm_viewing_file = false;
                    file_edit_mode = false;
                    redraw_file_manager();
                    continue;
                }
            }

            
            if (!fm_viewing_file) {
                if (scancode == KEY_UP) {
                    if (fm_selected_index > 0) {
                        fm_selected_index--;
                        redraw_file_manager();
                    }
                    continue;
                } else if (scancode == KEY_DOWN) {
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    if (fm_selected_index < count - 1) {
                        fm_selected_index++;
                        redraw_file_manager();
                    }
                    continue;
                } else if (scancode == KEY_RIGHT) {
                    
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    if (fm_selected_index < count) {
                        FileSystem::FileNode* selected = results[fm_selected_index];
                        if (selected->type == FileSystem::TYPE_DIRECTORY) {
                            if (FileSystem::change_directory(selected->name)) {
                                fm_selected_index = 0;
                                redraw_file_manager();
                            }
                        } else {
                            strncpy(fm_current_file, selected->name, 63);
                            fm_current_file[63] = '\0';
                            fm_viewing_file = true;
                            fm_file_scroll_offset = 0;
                            file_edit_mode = false;
                            redraw_file_manager();
                        }
                    }
                    continue;
                } else if (scancode == KEY_LEFT || scancode == KEY_BACKSPACE) {
                    
                    if (FileSystem::change_directory("..")) {
                        fm_selected_index = 0;
                        redraw_file_manager();
                    }
                    continue;
                } else if (scancode == KEY_PGUP) {
                    
                    fm_selected_index -= 5;
                    if (fm_selected_index < 0) fm_selected_index = 0;
                    redraw_file_manager();
                    continue;
                } else if (scancode == KEY_PGDN) {
                    
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    fm_selected_index += 5;
                    if (fm_selected_index >= count) fm_selected_index = count - 1;
                    if (fm_selected_index < 0) fm_selected_index = 0;
                    redraw_file_manager();
                    continue;
                } else if (scancode == KEY_DELETE) {
                    
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    if (fm_selected_index < count) {
                        FileSystem::delete_node(results[fm_selected_index]->name);
                        int new_count = FileSystem::list_directory(results, 32);
                        if (fm_selected_index >= new_count && new_count > 0)
                            fm_selected_index = new_count - 1;
                        else if (new_count == 0)
                            fm_selected_index = 0;
                        redraw_file_manager();
                    }
                    continue;
                }
            }

            
            char c = Keyboard::scancode_to_char(scancode);

            
            if (file_edit_mode && fm_viewing_file) {
                FileSystem::FileNode* node = FileSystem::find_node(fm_current_file);
                if (node && node->type == FileSystem::TYPE_FILE) {
                    if (c == '\b' && node->content_size > 0) {
                        
                        node->content_size--;
                        node->content[node->content_size] = '\0';
                        redraw_file_manager();
                    } else if (c >= 32 && c <= 126 && node->content_size < FileSystem::MAX_FILE_CONTENT - 1) {
                        
                        node->content[node->content_size++] = c;
                        node->content[node->content_size] = '\0';
                        redraw_file_manager();
                    } else if (c == '\n' && node->content_size < FileSystem::MAX_FILE_CONTENT - 1) {
                        
                        node->content[node->content_size++] = '\n';
                        node->content[node->content_size] = '\0';
                        redraw_file_manager();
                    }
                }
                continue;
            }

            if (c == '\n') {
                
                if (cmd_pos == 0) {
                    FileSystem::FileNode* results[32];
                    int count = FileSystem::list_directory(results, 32);
                    if (fm_selected_index < count) {
                        FileSystem::FileNode* selected = results[fm_selected_index];
                        if (selected->type == FileSystem::TYPE_FILE) {
                            
                            strncpy(fm_current_file, selected->name, 63);
                            fm_current_file[63] = '\0';
                            fm_viewing_file = true;
                            fm_file_scroll_offset = 0;  
                            redraw_file_manager();
                            continue;
                        } else {
                            
                            if (FileSystem::change_directory(selected->name)) {
                                fm_selected_index = 0;
                                redraw_file_manager();
                            }
                            continue;
                        }
                    }
                }

                
                cmd_buffer[cmd_pos] = '\0';

                
                char cmd_line[LINE_WIDTH + 1];
                strcpy(cmd_line, "079>");
                int len = strlen(cmd_line);
                int i = 0;
                while (i < LINE_WIDTH - len - 1 && cmd_buffer[i]) {
                    cmd_line[len + i] = cmd_buffer[i];
                    i++;
                }
                cmd_line[len + i] = '\0';
                add_line(cmd_line, 255);

                process_terminal_command(cmd_buffer);

                scroll_offset = 0;
                redraw_terminal();

                
                cursor_x = 37;  
                cmd_pos = 0;

            } else if (c == '\b') {
                
                if (cmd_pos > 0 && cursor_x > 37) {
                    cmd_pos--;
                    cursor_x -= 8;
                    Graphics::draw_rect(cursor_x, cursor_y, 8, 8, 0);
                }
            } else if (c == 0x18) {  
                if (scroll_offset < buffer_lines - 10) {
                    scroll_offset++;
                    redraw_terminal();
                }
            } else if (c == 0x19) {  
                if (scroll_offset > 0) {
                    scroll_offset--;
                    redraw_terminal();
                }
            } else if (cmd_pos < 63 && c >= 32 && c <= 126) {
                
                cmd_buffer[cmd_pos++] = c;

                
                Graphics::draw_char(cursor_x, cursor_y, c, 255);
                cursor_x += 8;
            }

            cursor_last_toggle = pit_ticks;
            cursor_visible = true;
        }
    }
}


extern "C" void kernel_main() {
    
    detect_cpu();

    
    VGA::initialize();

    
    Keyboard::initialize();

    
    
    Mouse::initialize();

    
    Graphics::initialize();


    boot_sequence();

    
    system_ready();

    // Note from 2026.02.11 - The visual interface for these stages is still under development :с

    
    main_interface();
}
