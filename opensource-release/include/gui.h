#ifndef GUI_H
#define GUI_H

#include "types.h"
#include "mouse.h"


struct Button {
    int16_t x, y;
    int16_t width, height;
    const char* text;
    bool pressed;
};


struct Dialog {
    int16_t x, y;
    int16_t width, height;
    const char* title;
    bool visible;
};


struct FileEntry {
    char name[16];
    bool is_directory;
    bool selected;
};

class GUI {
public:
    
    static void draw_button(const Button* btn, uint8_t color);
    static bool is_button_clicked(const Button* btn, const MouseState* mouse);

    
    static void draw_dialog(const Dialog* dlg, uint8_t bg_color, uint8_t border_color);
    static void draw_dialog_text(const Dialog* dlg, int y_offset, const char* text, uint8_t color);

    
    static void draw_file_entry(int x, int y, const char* name, bool selected, bool is_dir);
    static int get_clicked_file_index(int x, int y, int width, int entry_height, const MouseState* mouse);

    
    static void draw_plus_icon(int x, int y, uint8_t color);
    static void draw_trash_icon(int x, int y, uint8_t color);
    static void draw_back_arrow(int x, int y, uint8_t color);
    static void draw_folder_icon(int x, int y, uint8_t color);
    static void draw_file_icon(int x, int y, uint8_t color);

    
    static bool is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh);
};

#endif
