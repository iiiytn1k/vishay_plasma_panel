/*
 * Created: 18.10.2015 11:38:01
 */ 

#define F_CPU 16000000UL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sfr_defs.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "framebuffer.h"

#define Chip_select PORTB0 
#define RX_BUFFER_SIZE 192 //Размер приемного буфера = максимальное количество отображаемых символов на дисплее


volatile uint8_t received_bytes_count=0; //Количество принятых символов
uint8_t uart_receive_data_flag=0;

char rx_buffer[RX_BUFFER_SIZE];

void clear_rx_buffer(void)
{
	for(uint8_t t=0;t<RX_BUFFER_SIZE;t++)
	{
		rx_buffer[t]=' ';
	}
}

void USART0_init(void)
{
	UCSR0A = 0;
	UCSR0B |= (1<<RXCIE0) | (1<<RXEN0) | (1<<TXEN0);  
	    
	UCSR0C |= (1 << UCSZ01) | (1<<UCSZ00);   //8data, 1 stop bit. 
	UBRR0 = 51; //19200 bps
}

uint8_t parse_buffer(void)
{
	struct {
		uint8_t x;
		uint8_t y;
		uint8_t character;
		int value;
		uint8_t length;
		char string[40];
	} arguments;
		
	/*Если пришел только один символ с кодом "127" (backspace), то стираем символ находящийся под курсором и сдвигаем курсор влево*/
	if(rx_buffer[0]==127 && received_bytes_count==1)
	{
		if(cursor_column!=0) 
		{
			cursor_column--;
		}
		
		else if (cursor_row!=0)
		{
			cursor_row--;
			cursor_column=31;
		}
		
		char_to_framebuffer(cursor_column,cursor_row, 32);
		received_bytes_count=0;
		return 0;
	}
	
	
	if(rx_buffer[0]=='<' && rx_buffer[1]=='c' && rx_buffer[2]=='l' && rx_buffer[3]=='s' && rx_buffer[4]=='>')
	{
		//<cls> - clear display - очистить дисплей
		clear_display(0);
		received_bytes_count=0;
		cursor_row=0;
		cursor_column=0;
		return 0;
	}
	
	if(rx_buffer[0]=='<' && rx_buffer[1]=='f' && rx_buffer[2]=='l' && rx_buffer[3]=='s' && rx_buffer[4]=='>')
	{
		//<fls> - clear and fill display - очистить и заполнить дисплей
		clear_display(1);
		received_bytes_count=0;
		cursor_row=0;
		cursor_column=0;
		return 0;
	}	
		
	if(rx_buffer[0]=='<' && rx_buffer[1]=='c' && rx_buffer[2]=='t' && rx_buffer[3]=='b' && rx_buffer[4]=='>' && rx_buffer[5]==';')
	{		
		//<ctb> - char to buffer (x;y;char) - вывести символ в знакоместо 
		//Char - ASCII код символа
		sscanf(&(rx_buffer[5]),";%hhu;%hhu;%hhu", &arguments.x, &arguments.y, &arguments.character);
		char_to_framebuffer(arguments.x, arguments.y, arguments.character);
		received_bytes_count=0;
		cursor_row=0;
		cursor_column=0;
		return 0;
	}
	
	if(rx_buffer[0]=='<' && rx_buffer[1]=='s' && rx_buffer[2]=='t' && rx_buffer[3]=='b' && rx_buffer[4]=='>' && rx_buffer[5]==';')
	{
		//<stb>; string to buffer (x;y;string) - вывести строку начиная с определенного знакоместа 
		//Работает криво. Нужно переписать.
		sscanf(&(rx_buffer[5]),";%hhu;%hhu;%s",&arguments.x, &arguments.y, &arguments.string);
		string_to_framebuffer(arguments.string, arguments.x, arguments.y, 0);
		received_bytes_count=0;
		return 0;
	}
	
	if(rx_buffer[0]=='<' && rx_buffer[1]=='v' && rx_buffer[2]=='t' && rx_buffer[3]=='b' && rx_buffer[4]=='>' && rx_buffer[5]==';')
	{
		//<vtb>; value to buffer (x;y;length;value) - вывести целочисленное значение. 
		//length - максимальное число цифр в числе. Выравнивание по правому краю.
		sscanf(&(rx_buffer[5]),";%hhu;%hhu;%hhu;%d",&arguments.x, &arguments.y, &arguments.length, &arguments.value);
		switch (arguments.length)
		{
			case 1:
			{
				sprintf(arguments.string, "%1d", arguments.value);
				break;
			}
			
			case 2:
			{
				sprintf(arguments.string, "%2d", arguments.value);
				break;
			}
			case 3:
			{
				sprintf(arguments.string, "%3d", arguments.value);
				break;
			}
			case 4:
			{
				sprintf(arguments.string, "%4d", arguments.value);
				break;
			}
			case 5:
			{
				sprintf(arguments.string, "%5d", arguments.value);
				break;
			}
			case 6:
			{
				sprintf(arguments.string, "%6d", arguments.value);
				break;
			}
			default:
			{
				break;
			}
		}
		string_to_framebuffer(arguments.string, arguments.x, arguments.y, arguments.length);
		received_bytes_count=0;
		return 0;
	}	
	
		
	if(rx_buffer[0]=='<' && rx_buffer[1]=='p' && rx_buffer[2]=='t' && rx_buffer[3]=='b' && rx_buffer[4]=='>' && rx_buffer[5]==';')
	{
		//<ptb>; pixel to buffer (x;y;on/off) 0-on; 1-off.
		//Зажечь/погасить пиксель на экране
		sscanf(&(rx_buffer[5]),";%hhu;%hhu;%hhu",&arguments.x ,&arguments.y, &arguments.character);
		pixel_to_framebuffer(arguments.x, arguments.y, arguments.character);
		received_bytes_count=0;
		return 0;
	}
	
	else 
	{
		return 1;
	}
		
}


void Timer1_init()
{
	TCCR1B &= ~(1<<CS12)|~(1<<CS11)|~(1<<CS10); //Таймер остановлен
	OCR1AH = 0x00;
	OCR1AL = 0x97;                           
	TIMSK1 |= (1<<OCIE1A);                    // Разрешить прерывание по совпадению A
}

void Timer3_init()
{
	TCCR3B &= ~(1<<CS32)|~(1<<CS31)|~(1<<CS30); //Таймер остановлен
	OCR3AH = 0x0f;
	OCR3AL = 0xa0;
	TIMSK3 |= (1<<OCIE3A);                    // Разрешить прерывание по совпадению A
}

ISR (TIMER1_COMPA_vect)
{
	TCCR1B &= ~(1<<CS12) | ~(1<<CS11) | ~(1<<CS10); //Таймер остановлен
	PORTA=0xFF;//Погасить
	PORTL=0xFF;//все строки
}

ISR (TIMER3_COMPA_vect)
{
	//Если таймер приема символов отсчитал
	PORTB &= ~(1<<5);
	TCCR3B = 0; //Останавливаем таймер
	
	uart_receive_data_flag = 1;
}

void Timer1_start(void) //Таймер отображения каждой строчки
{
	TCNT1H = 0xff;
	TCNT1L = 0x4a;
	TCCR1B |= (1<<CS11); //Запустить таймер, предделитель 8
		
}

void Timer3_start(void) //Таймер приема символа по UART
{
	TCCR3B |= (1<<WGM32);
	OCR3AH = 0x0f;
	OCR3AL = 0xa0;
	TCNT3H = 0x0;
	TCNT3L = 0x0;
	TCCR3B |= (1<<CS31); //Запустить таймер, предделитель 8
	PORTB |= (1<<5);
}

ISR(USART0_RX_vect)
{
	uint8_t received_data; 
	received_data=UDR0;
	rx_buffer[(received_bytes_count++)]=received_data;
	if (received_bytes_count == RX_BUFFER_SIZE) received_bytes_count=0; 
	Timer3_start(); //После каждого принятого символа запускаем/обнуляем таймер
	
}

void spi_write(uint8_t val)
{
	SPDR = val;
	while(!(SPSR & (1<<SPIF)));
}

void select_row(uint8_t row_symb, uint8_t row_char)
{
	switch(row_symb)
	{
		case 0:
		{
				switch(row_char)
				{
					case 0:
					{
						PORTL=3;
						Timer1_start();
						break;
					}
					
					case 1:
					{
						PORTA=23;
                        Timer1_start();
						break;	
					}
					
					case 2:
					{
						PORTL=4;
                        Timer1_start();
						break;						
					}
					
					case 3:
					{
						PORTA=22;
                        Timer1_start();
						break;						
					}
					
					case 4:
					{
						PORTL=5;
                        Timer1_start();
						break;						
					}
					
					case 5:
					{
						PORTA=21;
                        Timer1_start();
						break;						
					}
					
					case 6:
					{
						PORTL=6;
                        Timer1_start();
						break;						
					}
					
					default:
					break;
				}
		break;
		}
		
		case 1:
		{
			switch(row_char)
			{
				case 0:
				{
						PORTA=20;
                        Timer1_start();
						break;					
				}
				
				case 1:
				{
						PORTL=7;
                        Timer1_start();
						break;					
				}
				
				case 2:
				{
						PORTA=19;
                        Timer1_start();
						break;					
				}
				
				case 3:
				{
						PORTL=8;
                        Timer1_start();
						break;					
				}
				
				case 4:
				{
						PORTA=18;
                        Timer1_start();
						break;					
				}
				
				case 5:
				{
						PORTL=9;
                        Timer1_start();
						break;					
				}
				
				case 6:
				{
						PORTA=17;
                        Timer1_start();
						break;					
				}
				
				default:
				break;
			}
		break;
		}
		
		case 2:
		{
			switch(row_char)
			{
				case 0:
				{
						PORTL=10;
                        Timer1_start();
						break;					
				}
				
				case 1:
				{
						PORTA=16;
                        Timer1_start();
						break;					
				}
				
				case 2:
				{
						PORTL=11;
                        Timer1_start();
						break;					
				}
				
				case 3:
				{
						PORTA=15;
                        Timer1_start();
						break;					
				}
				
				case 4:
				{
						PORTL=12;
                        Timer1_start();
						break;					
				}
				
				case 5:
				{
						PORTA=14;
                        Timer1_start();
						break;					
				}
				
				case 6:
				{
						PORTL=13;
                        Timer1_start();
						break;					
				}
				
				default:
				break;
			}
		break;
		}
		
		case 3:
		{
			switch(row_char)
			{
				case 0:
				{
						PORTA=13;
                        Timer1_start();
						break;					
				}
				
				case 1:
				{
						PORTL=14;
                        Timer1_start();
						break;					
				}
				
				case 2:
				{
						PORTA=12;
                        Timer1_start();
						break;					
				}
				
				case 3:
				{
						PORTL=15;
                        Timer1_start();
						break;					
				}
				
				case 4:
				{
						PORTA=11;
                        Timer1_start();
						break;					
				}
				
				case 5:
				{
						PORTL=16;
                        Timer1_start();
						break;					
				}
				
				case 6:
				{
						PORTA=10;
                        Timer1_start();
						break;					
				}
				
				default:
				break;
			}	
		break;
		}
				
		case 4:
		{
			switch(row_char)
			{
				case 0:
				{
						PORTL=17;
                        Timer1_start();
						break;					
				}
				
				case 1:
				{
						PORTA=9;
                        Timer1_start();
						break;					
				}
				
				case 2:
				{
						PORTL=18;
                        Timer1_start();
						break;					
				}
				
				case 3:
				{
						PORTA=8;
                        Timer1_start();
						break;					
				}
				
				case 4:
				{
						PORTL=19;
                        Timer1_start();
						break;					
				}
				
				case 5:
				{
						PORTA=7;
                        Timer1_start();
						break;					
				}
				
				case 6:
				{
						PORTL=20;
                        Timer1_start();
						break;					
				}
				
				default:
				break;
			}
		break;
		}
		
		case 5:
		{
			switch(row_char)
			{
				case 0:
				{
						PORTA=6;
                        Timer1_start();
						break;					
				}
				
				case 1:
				{
						PORTL=21;
                        Timer1_start();
						break;					
				}
				
				case 2:
				{
						PORTA=5;
                        Timer1_start();
						break;					
				}
				
				case 3:
				{
						PORTL=22;
                        Timer1_start();
						break;					
				}
				case 4:
				{
						PORTA=4;
                        Timer1_start();
						break;					
				}
				
				case 5:
				{
						PORTL=23;
                        Timer1_start();
						break;					
				}
				
				case 6:
				{
						PORTA=3;
						Timer1_start();
						break;					
				}
			}
		break;
		}
				
		default:
		break;			
	}
}

int main(void)
{
	uint8_t data = 0;
	
	DDRB=(1<<DDB7) | (1<<DDB6) | (1<<DDB5) | (1<<DDB4) | (1<<DDB3) | (1<<DDB2) | (1<<DDB1) | (1<<DDB0);
	DDRA=(1<<DDA7) | (1<<DDA6) | (1<<DDA5) | (1<<DDA4) | (1<<DDA3) | (1<<DDA2) | (1<<DDA1) | (1<<DDA0);
	DDRL=(1<<DDL7) | (1<<DDL6) | (1<<DDL5) | (1<<DDL4) | (1<<DDL3) | (1<<DDL2) | (1<<DDL1) | (1<<DDL0);
	SPCR=(0<<SPIE) | (1<<SPE)  | (0<<DORD) | (1<<MSTR) | (1<<CPOL) | (1<<CPHA) | (1<<SPR0) |(0<<SPR1);
	SPSR |= (1<<SPI2X);
	
	USART0_init();
	Timer1_init();
	Timer3_init();
	sei();
	PORTA=0xFF;
	PORTL=0xFF;
	_delay_ms(20);
	PORTB |= 1<<Chip_select;
	
	clear_display(0);
	//string_to_framebuffer("Vishay APD-240M026A",1,5,0);
	string_to_framebuffer("С наступающим Новым Годом!",5,3,0);

    while(1)
    {
		for(uint8_t current_vertical_char=0;current_vertical_char<6;current_vertical_char++)//Текущий символ по вертикали
		{		
			for(uint8_t current_char_row=0;current_char_row<7;current_char_row++)//Текущая строка символа
			{
				for(uint8_t current_column=0;current_column<DISP_LENG;current_column=current_column+8)//Текущий столбец 
				{
					for(uint8_t i=0;i<8;i++)//Текущий бит байта передаваемого по SPI
					{
						if(frame[current_column+i][current_vertical_char]&(1<<current_char_row))data|=1<<(7-i);
					}
					spi_write(data);
					data=0;
				}
				
		    	PORTB |= 1<<Chip_select;
				PORTB &= ~1<<Chip_select;
				
				PORTA=0xFF;//Погасить
				PORTL=0xFF;//все строки
				select_row(current_vertical_char,current_char_row);				
			}
	    }
		
		if (uart_receive_data_flag)
		{
			cli();
			parse_buffer(); //Разбираем принятые данные на комманды
			if(!(received_bytes_count==0)) string_to_framebuffer(rx_buffer,cursor_row,cursor_column,received_bytes_count); //Если не пришла команда, то просто выводим принятые данные
			clear_rx_buffer(); //Очищаем приемный буфер
			received_bytes_count=0;
			uart_receive_data_flag=0;
			sei();
		}
	}
}
