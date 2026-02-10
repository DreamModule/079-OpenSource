#include "gui.h"
#include "graphics.h"
#include "string.h"

bool GUI::is_point_in_rect(int px, int py, int rx, int ry, int rw, int rh) {
    return (px >= rx && px < rx + rw && py >= ry && py < ry + rh);
}

void GUI::draw_button(const Button* btn, uint8_t color) {
    
    Graphics::draw_rect(btn->x, btn->y, btn->width, btn->height, color);

    
    if (btn->pressed) {
        for (int y = 1; y < btn->height - 1; y++) {
            for (int x = 1; x < btn->width - 1; x++) {
                Graphics::put_pixel(btn->x + x, btn->y + y, 8); 
            }
        }
    }

    
    int text_x = btn->x + (btn->width / 2) - ((strlen(btn->text) * 8) / 2);
    int text_y = btn->y + (btn->height / 2) - 4;
    Graphics::draw_text(text_x, text_y, btn->text, color);
}

bool GUI::is_button_clicked(const Button* btn, const MouseState* mouse) {
    return is_point_in_rect(mouse->x, mouse->y, btn->x, btn->y, btn->width, btn->height);
}

void GUI::draw_dialog(const Dialog* dlg, uint8_t bg_color, uint8_t border_color) {
    if (!dlg->visible) return;

    
    Graphics::draw_rect(dlg->x, dlg->y, dlg->width, dlg->height, bg_color);

    
    for (int x = dlg->x; x < dlg->x + dlg->width; x++) {
        Graphics::put_pixel(x, dlg->y, border_color);
        Graphics::put_pixel(x, dlg->y + dlg->height - 1, border_color);
    }
    for (int y = dlg->y; y < dlg->y + dlg->height; y++) {
        Graphics::put_pixel(dlg->x, y, border_color);
        Graphics::put_pixel(dlg->x + dlg->width - 1, y, border_color);
    }

    
    if (dlg->title) {
        Graphics::draw_text(dlg->x + 4, dlg->y + 4, dlg->title, border_color);
    }
}

void GUI::draw_dialog_text(const Dialog* dlg, int y_offset, const char* text, uint8_t color) {
    if (!dlg->visible) return;
    Graphics::draw_text(dlg->x + 4, dlg->y + y_offset, text, color);
}

void GUI::draw_file_entry(int x, int y, const char* name, bool selected, bool is_dir) {
    
    if (selected) {
        Graphics::draw_rect(x, y, 150, 8, 7); 
    }

    
    if (is_dir) {
        draw_folder_icon(x, y, 15);
    } else {
        draw_file_icon(x, y, 15);
    }

    
    Graphics::draw_text(x + 10, y, name, 15);
}

int GUI::get_clicked_file_index(int x, int y, int width, int entry_height, const MouseState* mouse) {
    if (mouse->x < x || mouse->x >= x + width) {
        return -1;
    }

    int relative_y = mouse->y - y;
    if (relative_y < 0) {
        return -1;
    }

    return relative_y / entry_height;
}

void GUI::draw_plus_icon(int x, int y, uint8_t color) {
    
    
    for (int i = 1; i < 6; i++) {
        Graphics::put_pixel(x + 3, y + i, color);
    }
    
    for (int i = 1; i < 6; i++) {
        Graphics::put_pixel(x + i, y + 3, color);
    }
}

void GUI::draw_trash_icon(int x, int y, uint8_t color) {
    
    
    for (int i = 1; i < 6; i++) {
        Graphics::put_pixel(x + i, y + 1, color);
    }
    
    Graphics::put_pixel(x + 2, y + 2, color);
    Graphics::put_pixel(x + 4, y + 2, color);
    for (int j = 3; j < 6; j++) {
        Graphics::put_pixel(x + 2, y + j, color);
        Graphics::put_pixel(x + 4, y + j, color);
    }
    
    for (int i = 2; i < 5; i++) {
        Graphics::put_pixel(x + i, y + 6, color);
    }
}

void GUI::draw_back_arrow(int x, int y, uint8_t color) {
    
    
    Graphics::put_pixel(x + 1, y + 3, color);
    Graphics::put_pixel(x + 2, y + 2, color);
    Graphics::put_pixel(x + 2, y + 4, color);
    
    for (int i = 2; i < 6; i++) {
        Graphics::put_pixel(x + i, y + 3, color);
    }
}

void GUI::draw_folder_icon(int x, int y, uint8_t color) {
    
    
    Graphics::put_pixel(x, y, color);
    Graphics::put_pixel(x + 1, y, color);
    Graphics::put_pixel(x + 2, y, color);
    
    for (int j = 1; j < 6; j++) {
        for (int i = 0; i < 7; i++) {
            Graphics::put_pixel(x + i, y + j, color);
        }
    }
}

void GUI::draw_file_icon(int x, int y, uint8_t color) {
    
    for (int j = 0; j < 8; j++) {
        Graphics::put_pixel(x, y + j, color);
        Graphics::put_pixel(x + 5, y + j, color);
    }
    for (int i = 0; i < 6; i++) {
        Graphics::put_pixel(x + i, y, color);
        Graphics::put_pixel(x + i, y + 7, color);
    }
}
