#include "fullpacket.h"
#include <stdint.h>

void generate_imu(long int* imu_array,long int offset){
    for(int i=0; i<IMU_PACKET_N; i++){
       imu_array[i]=i+offset;
    }
}
void generate_elec(long int* elec_array,long int offset){
    for(int i=0; i<ELEC_PACKET_N; i++){
       elec_array[i]=i+offset;
    }
}

void generate_dataStruct(FullPacket_t* packet,long int offset){

	      for(int i=0; i<8; i++){
	        generate_elec(packet->elec[i],1000*i+offset);
	      }
	      for(int i=0; i<6; i++){
	        generate_imu(packet->imu[i],100*i+offset);
	     }
	      generate_imu(packet->another_meas,2222+offset);
}

void generate_dataArray(int32_t* data,long int offset){
	      for(int i=0; i<8; i++){
	        generate_elec(data+(i*ELEC_PACKET_N+offset),offset);
	      }
	      for(int i=0; i<7; i++){
	      	        generate_imu(data+(3200+i*IMU_PACKET_N),offset);
	      	     }
}

const char mqtt_publish_message[]="{\"iddata_series\":\"%d\","
								  "\"bench_token\":\"%s\","
								  "\"timestamp\":\"%s\","
								  "\"data\":{";
static const char base64[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

int base64_encode(const uint8_t *src, size_t len, char *dst)
{
    const uint8_t *end = src + len - (len % 3);
    size_t j = 0;
    uint32_t triple;

    while (src < end) {
        triple = ((uint32_t)src[0] << 16) |
                 ((uint32_t)src[1] << 8)  |
                 ((uint32_t)src[2]);

        dst[j++] = base64[(triple >> 18) & 0x3F];
        dst[j++] = base64[(triple >> 12) & 0x3F];
        dst[j++] = base64[(triple >> 6)  & 0x3F];
        dst[j++] = base64[triple & 0x3F];

        src += 3;
    }

    size_t r = len % 3;

    if (r == 1) {
        triple = ((uint32_t)src[0] << 16);

        dst[j++] = base64[(triple >> 18) & 0x3F];
        dst[j++] = base64[(triple >> 12) & 0x3F];
        dst[j++] = '=';
        dst[j++] = '=';
    }
    else if (r == 2) {
        triple = ((uint32_t)src[0] << 16) |
                 ((uint32_t)src[1] << 8);

        dst[j++] = base64[(triple >> 18) & 0x3F];
        dst[j++] = base64[(triple >> 12) & 0x3F];
        dst[j++] = base64[(triple >> 6)  & 0x3F];
        dst[j++] = '=';
    }

    dst[j] = 0;
    return j;
}

int create_base64_packet(unsigned char* json_buff, size_t buff_size,
						int dataseries_id,char* bench_token,
						uint32_t timestamp,FullPacket_t* packet) {

    // 1. Calculate binary size
    // Assuming your struct only contains the arrays now, or use a pointer to the arrays
    // Let's assume 'packet' is the struct containing the int16_t arrays.
    size_t binary_size = sizeof(FullPacket_t);

    // 2. Start JSON
    int len = snprintf((char*)json_buff, buff_size,
             "{\"id\":%d,\"token\":\"%s\",\"ts\":\"%ld\",\"data_base64\":\"",
             dataseries_id,bench_token,timestamp);

    // 3. Append Base64 Data
    // We pass the address of the struct as the binary input
    len += base64_encode((uint8_t*)packet, binary_size, json_buff+len);

    // 4. Close JSON
    json_buff[len++] = '"';
    json_buff[len++] = '}';
    json_buff[len] = 0;
    len += 2;

    return len;
}

int create_base64_packet_arr(char* json_buff, size_t buff_size,
		int dataseries_id,char* bench_token,char* timestamp,int32_t* arr){

    // 1. Calculate binary size
    // Assuming your struct only contains the arrays now, or use a pointer to the arrays
    // Let's assume 'packet' is the struct containing the int16_t arrays.
    size_t binary_size = DATA_ARRAY_SIZE*sizeof(int32_t);

    // 2. Start JSON
    int len = snprintf(json_buff, buff_size,
             "{\"id\":%d,\"token\":\"%s\",\"ts\":\"%s\",\"data_base64\":\"",
             dataseries_id,bench_token,timestamp);

    // 3. Append Base64 Data
    // We pass the address of the struct as the binary input
    len += base64_encode((uint8_t*)arr, binary_size, json_buff+len);

    // 4. Close JSON
    json_buff[len++] = '"';
    json_buff[len++] = '}';
    json_buff[len] = 0;
    len += 2;

    return len;
}




