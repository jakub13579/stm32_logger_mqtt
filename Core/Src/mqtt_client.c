/*
 * mqtt_client.c
 *
 *  Created on: Nov 2, 2025
 *      Author: jakub
 */


#include "mqtt_client.h"


static unsigned char mqtt_sendbuf[MQTT_SEND_BUF_SIZE] __attribute__((aligned(4)));
static unsigned char mqtt_recvbuf[MQTT_RECV_BUF_SIZE];


struct netconn *conn;
mqttc_net_t sock;
static ip_addr_t serv_addr;

static unsigned short dest_port = 1883;
struct mqtt_client client;

int tree_setup(){



return 0;
}




int mqtt_data_transfer(struct mqtt_client* client){
//TODO
return 0;
}

int get_config(){
//TODO
	return 0;
}
void init_mqtt_client_thread(){


	while(1){
		//0. waiting for Semaphore from LWIP setup function
		extern osSemaphoreId lwipReadySemHandle;
		osSemaphoreWait(lwipReadySemHandle, osWaitForever);
		//1. create tcp connection
		conn = netconn_new(NETCONN_TCP);
		IP_ADDR4(&serv_addr, 192, 168, 1, 10);
		if (conn != NULL &&
			netconn_connect(conn, &serv_addr, dest_port)==ERR_OK){
			//1.5?? get test configuration?
		//2. Tree setup
		//TODO tree setup based on matlab
			//2.1. login on the website
			tree_setup();

		//TODO get config from the server
		char* mqtt_username="";
	    char* mqtt_password="";
		char* mqtt_topic="";
		char* mqtt_client_id="";
		//3. Create test session
		//TODO test session based on matlab
		//4. get data_series
		//TODO data series based on matlab
		//5. MQTT client start
			mqtt_init(&client, &sock, mqtt_sendbuf, sizeof(mqtt_sendbuf), mqtt_recvbuf, sizeof(mqtt_recvbuf), NULL);
			mqtt_connect(&client, mqtt_client_id, NULL, NULL, 0, mqtt_username, mqtt_password, 0, 400);
		//6. enter infinite looop with  mqtt_data_transfer()

			while(1){
				mqtt_sync(&client);
				mqtt_data_transfer(&client);
				osDelay(10);
			}
		}
		netconn_close(conn);
		netconn_delete(conn);
	}

}

