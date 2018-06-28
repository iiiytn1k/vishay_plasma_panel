#ifndef     Frame_h
#define     Frame_h

#define DISP_LENG 160 //Длина дисплея в точках
#define DISP_CHARS 32 //Длина дисплея в символах
extern unsigned char frame [DISP_LENG][6]; //Основной фреймбуфер дисплея
extern uint8_t cursor_row, cursor_column;	
	
void clear_display(uint8_t fill);
void scroll_display(void);
void string_to_framebuffer(char *string_, uint8_t row_, uint8_t column_, uint8_t length_);
void char_to_framebuffer(uint8_t column, uint8_t row, uint8_t character);
void pixel_to_framebuffer(uint8_t x, uint8_t y, uint8_t draw);
void line_to_framebuffer(int x1, int y1, int x2, int y2);
void rectangle_to_framebuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2);
void filled_rectangle_to_framebuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill);

#endif //Header