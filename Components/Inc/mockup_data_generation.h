

#ifndef INC_MOCKUP_DATA
#define INC_MOCKUP_DATA

#define SINE_TABLE_SIZE 400
#define SEND_BUFFER_SIZE 3340
#define PI 3.14159265358979323846f

#define IMU_PACKET_N 20
#define ELEC_PACKET_N 400

typedef struct{
	int data_series_id;
	char bench_token[128];
	char timestamp[64];
	int acc_x[IMU_PACKET_N];
	int acc_y[IMU_PACKET_N];
	int acc_z[IMU_PACKET_N];
	int gyr_x[IMU_PACKET_N];
	int gyr_y[IMU_PACKET_N];
	int gyr_z[IMU_PACKET_N];
	int curr_u[ELEC_PACKET_N];
	int curr_v[ELEC_PACKET_N];
	int curr_w[ELEC_PACKET_N];
	int curr_dc[ELEC_PACKET_N];
	int volt_u[ELEC_PACKET_N];
	int volt_v[ELEC_PACKET_N];
	int volt_w[ELEC_PACKET_N];
	int volt_dc[ELEC_PACKET_N];
	int another_meas[IMU_PACKET_N];
}FullPacket_t;


int FullPacket_json_encode();

typedef enum{
	VOLTAGE_DC=0,
	VOLTAGE_A,
	VOLTAGE_B,
	VOLTAGE_C,
	CURRENT_DC,
	CURRENT_A,
	CURRENT_B,
	CURRENT_C,
	ACCE_X,
	ACCE_Y,
	ACCE_Z,
	GYRO_X,
	GYRO_Y,
	GYRO_Z,
	ANOTHER_MEAS
}mockup_sensor_t;

int init_mockup_data();


#endif /* INC_MQTT_CLIENT_H_ */
