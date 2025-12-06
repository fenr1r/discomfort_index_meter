/*
 * DHT20.cpp
 *
 * Created: 2025/11/17 12:36:24
 *  Author: fenrir_
 */ 

#include <avr/io.h>
#include <stdlib.h>	// itoa

#define F_CPU 1000000
#include <util/delay.h>

#include "i2cmaster.h"
#include "DHT20.hpp"

#define CODE_SHORT 1
//#define _DEBUG 1

#ifdef _DEBUG

#include "LCD_ST7032i.hpp"
#include <stdio.h>	//sprintf

#endif


void DHT20_init(){

#ifndef CODE_SHORT
	// LCD側で200ms先に待つので、省略可能
	_delay_ms(100); 
#endif
	
	uint8_t status_word = 0;
	
	i2c_start(0x71);
	status_word = i2c_read(0);
	i2c_stop();
	
#ifndef CODE_SHORT
	if (status_word != 0x18) {
		// TODO : init 0x1B, 0x1C, 0x1E
		// しかし、公式のサンプルコード見に行かないと処理が分からない
		// 幸い、今のところはずっと初期化後0x18（=24）が返ってきている
	}
#endif

#ifdef _DEBUG
	char buf[5];
	itoa(status_word, buf, 10);
	lcd_puts(LCD_ST7032I_RAMADDR_ROW1, buf, true);
#endif
}

// 温度がマイナスの場合は面倒だから考慮しない
SensorValue DHT20_reflesh()
{
	
#ifndef CODE_SHORT
	// main側で10ms待つので、省略可能
	_delay_ms(10);
#endif
	
	// 計測開始コマンドをWrite
	i2c_start(0x70);
	i2c_write(0xAC);
	i2c_write(0x33);
	i2c_write(0x00);
	i2c_stop();
	
	// 80ms待つ（省略不能）
	_delay_ms(80);
	
	// 計測結果読み取り
	
	SensorValue ret;

	uint8_t state;
	uint8_t data[5];
	
	i2c_start(0x71);
	state = i2c_read(1);
	data[0] = i2c_read(1);
	data[1] = i2c_read(1);
	data[2] = i2c_read(1);
	data[3] = i2c_read(1);
	data[4] = i2c_read(1);
	i2c_read(0);	// CRC 使わない
	i2c_stop();
	
#ifndef CODE_SHORT
	uint32_t _temp = 0;	// 本当はsigned だが、0℃は下回らない前提
	uint32_t _humidity = 0;
	
	_humidity = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | (uint32_t)((data[2] & 0xF0) >> 4);
	_temp = ((uint32_t)(data[2] & 0x0F) << 16) | ((uint32_t)data[3] << 8) | (uint32_t)(data[4]);
	
	// 10進、固定小数点（小数第1位まで含んで整数として扱う）の値に直す
	_humidity = ((_humidity * 10 * 100) >> 20);	// 2^20 * 1000 = 1,048,576,000 < 2^20 * 2^10 = 2^30 < 2^32 なのでuint32_tに収まる
	_temp = ((_temp * 10 * 200) >> 20) - 50 * 10;	// リッチな書き方
	
	// 範囲外チェック
	if (_humidity > 1000) {
		_humidity = 999;
	}
	
	if (_temp > 1500) {
		_temp = 1499;
	}
	// else if (humidity2 < 0) {
	//	humidity2 = 0;
	//}
	
	// 最後に幅合わせ
	ret.humidity	= (uint16_t)_humidity;	
	ret.temperature	= (uint16_t)_temp;
#else
	// 測定される湿度をAとする
	// Aの上位n-bitのみを使用すると、測定は1 / 2^nの分解能を持つ
	// 例：	n = 20	0.009%
	//		n = 12	0.02%
	//		n = 10	0.10%
	//		n = 9	0.20%
	//		n = 8	0.40%
	//		n = 6	1.56%
	// 実際のところ、このセンサーの湿度の誤差は最小でも+/- 3%なので、n = 6程度の精度があればよいことになる。
	// ただし、表示上の1の位すらも雑というのはなんだか気に食わないので、n = 9で使う。
	// 
	// %単位にしたときの整数だけ使いたいなら、×100。
	// その場合、(A * 100) / 2^nの順で計算する。先に/2^nすると小数になり計算結果は切り捨てられすべて0となる。
	// このとき、計算過程における最大値はフルスケール時の2^n * 100なので、それに応じたbit幅が必要。
	// 2^6 < 100 < 2^7なので、(n + 7) bitあればいい。
	// 例：	n = 10	-> 102400 < 2^17 = 131072 よって uint16_tではダメでuint32_t
	// 例：	n = 9	-> 51200  < 2^16 = 65536  よって uint16_tでOK
	// 
	// %単位にしたときに小数第1位まで使いたいなら、(A * 1000) / 2^nする。しかし実用的ではない
	// 
	// 温度は、精度+/-0.5℃で、換算式が×100から×200になるので（更に、小数表示のためには×2000）、uint32_tを使うのが無難
	// 
	// 結局、温度のためにuint32_t使うなら湿度もuint32_tでもよいような…
	
	uint32_t _temp = 0;
	uint16_t _humidity = 0;
	
	_humidity = ((uint32_t)data[0] << 1) | (uint32_t)((data[2] & 0x80) >> 7);
	_temp = ((uint32_t)(data[2] & 0x0F) << 16) | ((uint32_t)data[3] << 8) | (uint32_t)(data[4]);
	
	// 10進の値に直す
	_humidity = ((_humidity * 100) >> 9);	// 10進、整数
	_temp = ((_temp * 2000) >> 20) - 500; // 10進、固定小数点（小数第1位まで含んで整数として扱う）
	
	// 範囲外チェック
	if (_humidity > 100) {
		_humidity = 99;
	}
	
	if (_temp > 1500) {
		_temp = 1499;
	}
	
	// 最後に幅合わせ
	ret.humidity	= _humidity;
	ret.temperature	= (uint16_t)_temp;
#endif
	
#ifdef _DEBUG
	char buf[8];
	sprintf(buf, "%x %u", state, _humidity);
	// sprintf(buf, "%x", temp);
	
	lcd_puts(LCD_ST7032I_RAMADDR_ROW1, buf, true);
	
	//sprintf(buf, "%x %x", data[1], data[2]);
	sprintf(buf, "%u", _temp);
	lcd_puts(LCD_ST7032I_RAMADDR_ROW2, buf, true);
#endif

	return ret;
}