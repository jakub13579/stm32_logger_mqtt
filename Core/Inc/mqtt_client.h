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



#define MQTT_SEND_BUF_SIZE 512 //!! probably much more needed also might need two send buffers?
#define MQTT_RECV_BUF_SIZE 512

void init_mqtt_client_thread();
int mqtt_data_transfer(struct mqtt_client* client);



#endif /* INC_MQTT_CLIENT_H_ */
