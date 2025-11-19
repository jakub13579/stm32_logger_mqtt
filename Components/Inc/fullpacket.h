#ifndef FULLPACKET_H
#define FULLPACKET_H

#include "string.h"
#include <stdio.h>
#include <stdint.h>

#define IMU_PACKET_N 20
#define ELEC_PACKET_N 400
#define DATA_ARRAY_SIZE 3340

typedef struct {
	long int elec[8][ELEC_PACKET_N];
	long int imu[6][IMU_PACKET_N];
	long int another_meas[IMU_PACKET_N];
}FullPacket_t ; //__attribute__ ((packed))

void generate_dataArray(int32_t* packet, long int offset);
void generate_dataStruct(FullPacket_t* packet, long int offset);
int fullpacket_json_encode(char* json_buff, size_t json_buff_size, FullPacket_t packet);
int create_base64_packet(unsigned char* json_buff, size_t buff_size,int dataseries_id,char* bench_token,uint32_t timestamp,FullPacket_t* packet);
int create_base64_packet_arr(char* json_buff, size_t buff_size,int dataseries_id,char* bench_token,char* timestamp,int32_t* arr);

#endif 
