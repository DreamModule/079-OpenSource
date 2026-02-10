#ifndef FILE_MANAGER_H
#define FILE_MANAGER_H

#include "types.h"

class FileManager {
public:
    static void initialize();
    static void draw();
    static void handle_input(); 
    static void open_file(const char* filename); 
    static void set_position(int x, int y, int width, int height);

private:
    static void show_create_dialog();
    static void handle_create_dialog();
    static void refresh_file_list();

    static int panel_x, panel_y, panel_w, panel_h;
    static int selected_file_index;
    static bool dialog_visible;
    static int dialog_selection; 
    static char new_filename[16];
};

#endif
