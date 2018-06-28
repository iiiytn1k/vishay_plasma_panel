#include <stdlib.h>
#include <string.h>
#include <avr/pgmspace.h>
#include "framebuffer.h"
#include "my_font_5x7.h"

 uint8_t frame [DISP_LENG][6];
 uint8_t cursor_row=0, cursor_column=0;

void clear_display(uint8_t fill)
{
	uint8_t i,j;
	
	for(i=0;i<DISP_LENG;i++)
	{
		for(j=0;j<6;j++)
		{
			fill ? (frame[i][j]=0xFF) : (frame[i][j]=0);
		}
	}
}

void scroll_display(void)
{
	uint8_t i,j;
	for(i=0;i<DISP_LENG;i++)
	{
		for(j=0;j<5;j++)
		{
			frame[i][j]=frame[i][j+1];
		}
		frame[i][5]=0x00;
	}
}

void char_to_framebuffer(uint8_t column, uint8_t row, uint8_t character)
{
	if((character<32)||(character>255)) return;
	if((column>DISP_CHARS-1)||(row>5)) return;
	
	for(uint8_t i=0;i<5;i++)
	frame[(column*5)+i][row]=pgm_read_byte(&(font[character-32][i]));
}

void string_to_framebuffer(char *string, uint8_t row_, uint8_t column_, uint8_t length_)
{
	if(row_>5) row_=0;
	if(column_>DISP_CHARS-1) column_=0;
	
	if(length_==0) length_=strlen(string);
	for(int i=0;i<length_;i++)
	{
		if(string[i]==0x0D) column_=0;
		if(string[i]==0x0A)
		{
			row_++;
			if(row_>5)
			{
				scroll_display();
				row_=5;
			}
		}
		
		if(!(string[i]==0x0D)) // \r
		{
			if(column_>DISP_CHARS-1)
			{
				column_=0;
				row_++;
				if(row_>5)
				{
					scroll_display();
					row_=5;
				}
			}
			
			if(!(string[i]==0x0A)) // \n
			{
				char_to_framebuffer(column_,row_, 32);
				char_to_framebuffer(column_,row_, string[i]);
				column_++;
				if(column_>DISP_CHARS-1)
				{
					column_=0;
					row_++;
					if(row_>5)
					{
						scroll_display();
						row_=5;
					}
				}
			}
		}
	}
	cursor_row=row_;
	cursor_column=column_;
}

void pixel_to_framebuffer(uint8_t x, uint8_t y, uint8_t draw)
{
	if(x>159||y>41) return;
	if(draw==0)
	{
		if(y<7)frame[x][0]|=1<<y;
		if(y>=7&&y<14)frame[x][1]|=1<<(y-7);
		if(y>=14&&y<21)frame[x][2]|=1<<(y-14);
		if(y>=21&&y<28)frame[x][3]|=1<<(y-21);
		if(y>=28&&y<35)frame[x][4]|=1<<(y-28);
		if(y>=35&&y<42)frame[x][5]|=1<<(y-35);
	}
	if(draw!=0)
	{
		if(y<7)frame[x][0]&=~(1<<y);
		if(y>=7&&y<14)frame[x][1]&=~(1<<(y-7));
		if(y>=14&&y<21)frame[x][2]&=~(1<<(y-14));
		if(y>=21&&y<28)frame[x][3]&=~(1<<(y-21));
		if(y>=28&&y<35)frame[x][4]&=~(1<<(y-28));
		if(y>=35&&y<42)frame[x][5]&=~(1<<(y-35));
	}
}

void line_to_framebuffer(int x1, int y1, int x2, int y2)
{
	const int deltaX = abs(x2 - x1);
	const int deltaY = abs(y2 - y1);
	const int signX = x1 < x2 ? 1 : -1;
	const int signY = y1 < y2 ? 1 : -1;
	
	int error = deltaX - deltaY;
	
	pixel_to_framebuffer(x2, y2,0);
	while(x1 != x2 || y1 != y2) {
		pixel_to_framebuffer(x1, y1,0);
		const int error2 = error * 2;
		
		if(error2 > -deltaY) {
			error -= deltaY;
			x1 += signX;
		}
		if(error2 < deltaX) {
			error += deltaX;
			y1 += signY;
		}
	}
}

void rectangle_to_framebuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
		line_to_framebuffer(x1,y1,x2,y1);
		line_to_framebuffer(x1,y2,x2,y2);
		line_to_framebuffer(x1,y1,x1,y2);
		line_to_framebuffer(x2,y1,x2,y2);
}

void filled_rectangle_to_framebuffer(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t fill)
{
	int x,y;
	
	for(y=y1; y<=y2; y++)
	{
		for(x=x1; x<=x2; x++)
		{

			fill ? pixel_to_framebuffer(x,y,1) : pixel_to_framebuffer(x,y,0);
	    }
	}	
}