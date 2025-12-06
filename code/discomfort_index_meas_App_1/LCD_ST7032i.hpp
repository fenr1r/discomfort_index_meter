/*
 * LCD_ST7032i.h
 *
 * Created: 2025/11/17 1:34:00
 *  Author: fenrir_
 */ 


#ifndef LCD_ST7032I_H_
#define LCD_ST7032I_H_

#define ST7032I_ADDR	0x7C
#define ST7032I_ADDR_WR	 (ST7032I_ADDR + 0x00)
#define ST7032I_ADDR_RD	 (ST7032I_ADDR + 0x01)

#define CTRL_BYTE_CMD_LASTBYTE		0b10000000	// Co = 1, Rs = 0
#define CTRL_BYTE_DDRAM_LASTBYTE	0b11000000	// Co = 1, Rs = 1
#define CTRL_BYTE_CMD				0b00000000	// Co = 0, Rs = 0
#define CTRL_BYTE_DDRAM				0b01000000	// Co = 0, Rs = 1

#define LCD_ST7032I_RAMADDR_ROW1	0x00
#define LCD_ST7032I_RAMADDR_ROW2	0x40

void lcd_init();
void lcd_putc(const uint8_t, const char);
void lcd_puts(const uint8_t, const char*, const bool);

#endif /* LCD_ST7032I_H_ */