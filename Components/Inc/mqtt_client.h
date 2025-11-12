/*
 * mqtt_client.h
 *
 *  Created on: Nov 3, 2025
 *      Author: jakub
 */

#ifndef INC_MQTT_CLIENT_H_
#define INC_MQTT_CLIENT_H_


#include "mqtt.h"
#include "mqtt_pal.h"
#include "lwip/netif.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include "lwip/opt.h"
#include <errno.h>



//#define MQTT_SEND_BUF_SIZE 512 //!! probably much more needed also might need two send buffers?
//#define MQTT_RECV_BUF_SIZE 512


int create_socket_connection(int sock,const char* ip,unsigned short port, unsigned int timeout);
void init_mqtt_client_thread();
int mqtt_data_transfer(struct mqtt_client* client);

//HTTP Part

typedef struct {
    int socket;
    char* send_buffer;
    size_t send_buffer_size;
    char* recv_buffer;
    size_t recv_buffer_size;
    char* response_json;
    size_t response_json_size;
    int error;
} HttpClient_t;

void http_init(HttpClient_t* client,int sock,char* tx_buff,size_t tx_size, char* rx_buff, size_t rx_size,char* resp_jason, size_t resp_jason_size);
int http_post_request(HttpClient_t* client,int header_len,int content_len);


#endif /* INC_MQTT_CLIENT_H_ */
