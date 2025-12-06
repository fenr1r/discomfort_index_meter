/*
 * LCD_ST7032i.cpp
 *
 * Created: 2025/11/17 1:33:47
 *  Author: fenrir_
 */ 

#include <avr/io.h>

#define F_CPU 1000000
#include <util/delay.h>

#include "i2cmaster.h"	// 938Byte
#include "LCD_ST7032i.hpp"


static void LCD7032i_write_command(const uint8_t data){
	i2c_start(ST7032I_ADDR_WR);
	i2c_write(CTRL_BYTE_CMD_LASTBYTE);
	i2c_write(data);
	i2c_stop();
}

// 容量節約のためにinlineにしている。別にinlineにしなくていい
static void inline LCD7032i_write_str(const uint8_t addr, const char* str, const bool isWriteFromTop){

	i2c_start(ST7032I_ADDR_WR);

	// 行頭から書き込む場合は、先にDDRAMのアドレスを指定する
	if (isWriteFromTop) {
		i2c_write(CTRL_BYTE_CMD_LASTBYTE);
		i2c_write(0x80 | addr);
	
		_delay_us(100);	// このdelayが必要（ないと■になる）
	}

	// データ書き込む
	i2c_write(CTRL_BYTE_DDRAM);

	while (*str){
		// null終端文字は書き込まない（書き込むと空白として表示される）
		if (*str != '\0'){
			i2c_write(*str++);
		}
	}

	i2c_stop();
}

void lcd_puts(const uint8_t addr, const char* str, const bool isWriteFromTop){
	LCD7032i_write_str(addr, str, isWriteFromTop);
}

static void _delay_us_30(){
	_delay_us(30);
}

void lcd_init(){
	// init I2C interface
    i2c_init();                                

	// LCD Init sequence
	_delay_ms(40);	// > 40ms
	
	LCD7032i_write_command(0x38);
	_delay_us_30();	// > 26.3us
	
	LCD7032i_write_command(0x39);
	_delay_us_30();	// > 26.3us
	
	LCD7032i_write_command(0x14);
	_delay_us_30();	// > 26.3us
	
	// Note : 5V -> 0x73, 3.3V -> 0x70 （0x73ではちょっと画面が黒い）
	LCD7032i_write_command(0x70);
	_delay_us_30();	// > 26.3us
	
	// NOTE : 5V -> 0x52, 3.3V -> 0x56
	LCD7032i_write_command(0x56);	
	_delay_us_30();	// > 26.3us
	
	LCD7032i_write_command(0x6C);
	_delay_ms(200);	// > 200ms
	
	LCD7032i_write_command(0x38);
	_delay_us_30();	// > 26.3us
	
	LCD7032i_write_command(0x0C);
	_delay_us_30();	// > 26.3us
	
	LCD7032i_write_command(0x01);
	_delay_ms(2);	// > 1.08ms
}

void lcd_putc(const uint8_t addr, char c){
	// TODO : 実装
	// LCD7032i_write_str(addr, &c);
}

