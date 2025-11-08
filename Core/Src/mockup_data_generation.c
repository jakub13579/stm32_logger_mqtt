/*
 * mockup_data_generation.c
 *
 *  Created on: Nov 4, 2025
 *      Author: jakub
 */
#include "mockup_data_generation.h"
#include <stdint.h>
#include "math.h"

static uint32_t sine_table[SINE_TABLE_SIZE];
int32_t FullPacketA[SEND_BUFFER_SIZE];
int32_t FullPacketB[SEND_BUFFER_SIZE];

void generate_test_data(){
	//TODO change to int or change table to float
	for (int i = 0; i < SINE_TABLE_SIZE; i++) {
	    float angle_rad = 2.0f * PI * ((float)i / SINE_TABLE_SIZE);
	    float sine_val = sinf(angle_rad);

	    // Scale to 32-bit signed integer range
	    sine_table[i] = (int32_t)(sine_val * 2147483647.0f); // INT32_MAX
	}
}


int32_t mockup_data_generator(mockup_sensor_t sensor, int step){
switch (sensor){
case VOLTAGE_DC:
	return 400+step;
case VOLTAGE_A:
	return sine_table[(step% SINE_TABLE_SIZE)];
case VOLTAGE_B:
	return sine_table[(step% SINE_TABLE_SIZE)];
case VOLTAGE_C:
	return sine_table[(step% SINE_TABLE_SIZE)];
case CURRENT_DC:
	return 20+step;
case CURRENT_A:
	return sine_table[(step% SINE_TABLE_SIZE)];
case CURRENT_B:
	return sine_table[(step% SINE_TABLE_SIZE)];
case CURRENT_C:
	return sine_table[(step% SINE_TABLE_SIZE)];
case ACCE_X:
	return 1+step;
case ACCE_Y:
	return 2+step;
case ACCE_Z:
	return 3+step;
case GYRO_X:
	return 4+step;
case GYRO_Y:
	return 5+step;
case GYRO_Z:
	return 6+step;
case ANOTHER_MEAS:
	return 20-step;
default:
	return 0;
}
}

void populate_send_buffer(int32_t* buffer, int phase_offset){
	uint32_t offset = 0;
	for(int sensor=0;sensor<15;sensor++){
		int samples = (sensor < 8) ? 400 : 20; // 20 ms period
        for (int t = 0; t < samples; t++) {
            buffer[offset++] = mockup_data_generator(sensor, (t + phase_offset) % SINE_TABLE_SIZE);
        }
	}
}

int init_mockup_data(){
	generate_test_data();
	populate_send_buffer(FullPacketA,0);
	populate_send_buffer(FullPacketB,50);
	return 0;
}

