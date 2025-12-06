/*
 * DHT20.h
 *
 * Created: 2025/11/17 12:36:33
 *  Author: fenrir_
 */ 


#ifndef DHT20_H_
#define DHT20_H_

typedef struct SensorValue_ {
	uint16_t temperature;
	uint16_t humidity;
} SensorValue;

void DHT20_init();
SensorValue DHT20_reflesh();

#endif /* DHT20_H_ */