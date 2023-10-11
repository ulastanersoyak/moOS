#ifndef SCREEN_H
#define SCREEN_H

#define video_address 0xb8000
#define max_rows 25
#define max_cols 80

#define b_w 0x0f
#define r_w 0xf4

#define reg_screen_ctrl 0x3d4
#define reg_screen_data 0x3d5

void clear_screen();
void kprint_at(char *msg, int row, int col);
void kprint(char *msg);
#endif // !SCREEN_H
