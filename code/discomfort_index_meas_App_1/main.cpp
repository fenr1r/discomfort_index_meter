/*
 * discomfort_index_meas_App_1.cpp
 *
 * Created: 2025/11/13 3:50:04
 * Author : fenrir_
 */ 

// Device Fuse bit
// WDT disabled
// OSC RC 9.6, DIV8

// Hardware connection:
//	SDA	PB4
//	SCL PB3
//	LED PB0

#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include <stdlib.h>	// itoa
#include <string.h>

// Internal RC 8.0MHz を 8分周（1MHz）。I2Cのウェイトはnop1回でOK
#define F_CPU 1000000
#include <util/delay.h>

#include "LCD_ST7032i.hpp"
#include "DHT20.hpp"

#define _USE_TIMER_ISR 1

#ifdef _USE_TIMER_ISR
	static uint16_t timer_cnt = 0;
#endif

// 1回ずつしか呼ばれないのでinline化
static void inline LED_init(){
	DDRB |= (1 << 0);
	PORTB |= (1 << 0);
}

static void inline LED_on(){
	PORTB |= (1 << 0);
}

static void inline LED_off(){
	PORTB &= ~(1 << 0);
}

static inline SensorValue observe_temperature_and_humidity(){
	return DHT20_reflesh();
}

// 割り算を使わないように、×10した値を返す（固定小数点）
// tempは小数第1位までを整数として表す
// humidityは整数部分のみ
static inline uint16_t calculate_discomfort_index(const uint16_t temp, const uint16_t humidity){
	// 整数演算とするため、一旦×100000する
	return (uint16_t)((81 * 100 * (uint32_t)temp + (1 * (uint32_t)humidity * (99 * (uint32_t) temp - 14300))) / 10000) + 463;
}

static inline void display_temperature_humidity_and_discomfort_index(const uint8_t temp, const uint8_t humidity, const uint16_t di){

	char temp_buf[6];
	itoa(temp, temp_buf, 10);
	
	// itoaは左詰め等の指定ができないので、strlenで調整
	// 温度が1桁台の場合のみ考慮
	// 本当はsprintfで整形したほうがいい。それか、自前itoa
	if (strlen(temp_buf) == 2){
		temp_buf[2] = temp_buf[1];
		temp_buf[1] = temp_buf[0];
		temp_buf[0] = '0';
	}
	
	temp_buf[3] = temp_buf[2];
	temp_buf[2] = '.';
	temp_buf[4] = 0b11011111;	// °の記号
	temp_buf[5] = '\0';			// lcd_putsは終端文字は表示しない
	
	lcd_puts(LCD_ST7032I_RAMADDR_ROW1, temp_buf, true);
	
	char humidity_buf[4];
	itoa(humidity, humidity_buf, 10);
	humidity_buf[2] = '%';
	humidity_buf[3] = '\0';
		
	lcd_puts(LCD_ST7032I_RAMADDR_ROW1, humidity_buf, false);
	
	char di_buf[5];
	itoa(di, di_buf, 10);
	
	di_buf[3] = di_buf[2];
	di_buf[2] = '.';
	di_buf[4] = '\0';
	
	lcd_puts(LCD_ST7032I_RAMADDR_ROW2, "DI:", true);
	lcd_puts(LCD_ST7032I_RAMADDR_ROW2, di_buf, false);
}

#ifdef _USE_TIMER_ISR

// 割り込みは、1MHz / 1024（タイマのプリスケーラ） / 256（TCNT0の最大カウント） ≒ 0.262sごとに生じる
ISR(TIM0_OVF_vect){
	// 他に割り込み来ないけど、一応disableしておく
	cli();
	
	timer_cnt++;
	
	// 5sごとに測定する（5 / 0.262 ≒ 19）
	if (timer_cnt > 19){
		timer_cnt = 0;
		
		// update
		LED_on();
		
		const SensorValue sensor_val = observe_temperature_and_humidity();
		
		const uint16_t di = calculate_discomfort_index(sensor_val.temperature, sensor_val.humidity);
		
		display_temperature_humidity_and_discomfort_index(sensor_val.temperature, sensor_val.humidity, di);
		
		LED_off();
	}
	
	sei();
}

static void init_timer(){
	timer_cnt = 0;
	
	TCCR0A	= 0x00;			// Normalモード、コンペア出力なし
	TCCR0B	= 0b00000101;	// 1024分周（1MHz / 1024 ≒ 1kHz）
	TIMSK	= (1 << TOIE0);	// Enable Overflow Interrupt
}
#endif

int main(void)
{
    // SCL, SDA、LEDポートはinitで上書きされる。が、とりあえず最初は全入力全プルアップ
	DDRB = 0x00;
	PORTB = 0x3F;
		
	ACSR &= ~(1 << ACD);	// アナログ・コンパレータを非アクティブ（電力消費軽減）
	
	PRR = (1 << PRTIM1) | (1 << PRUSI) | (1 << PRADC);	// Timer1, USI, ADCを非アクティブ
	
	// 各種初期化
	LED_init();
	lcd_init();
	DHT20_init();
	
	LED_off();
	
	// 本当は割り込みを使いたいが、使うと+200Bくらいされ、1kBを超えてしまうためdisable
#ifdef _USE_TIMER_ISR
	init_timer();
	sei();	
	
	// IdleよりPower-downのほうが省電力だが、どちらにしろLCDが1mAくらい、温度センサーが10mAくらい消費するため、Idleでよしとした。
	// Power-downはWDTかINT割り込みが必要だが、IdleならTimerで起きるので扱いやすいし
	set_sleep_mode(SLEEP_MODE_IDLE);
	
	while (1)
	{
		// Timer割り込みを使う場合、メインループではIdleモードに落とす
		sleep_mode();
	}
#else
	while (1)
	{
		// Timer割り込みしない場合、ひたすらポーリングで値更新	
		_delay_ms(1000);
			
		LED_on();
			
		const SensorValue sensor_val = observe_temperature_and_humidity();
			
		const uint16_t di = calculate_discomfort_index(sensor_val.temperature, sensor_val.humidity);
			
		display_temperature_humidity_and_discomfort_index(sensor_val.temperature, sensor_val.humidity, di);
			
		LED_off();
	}
#endif
	
	return 0;
}

