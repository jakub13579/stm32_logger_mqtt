/*
 * mqtt_client.c
 *
 *  Created on: Nov 2, 2025
 *      Author: jakub
 */


#include "mqtt_client.h"
#include <lwip/sockets.h>
#include <arpa/inet.h>
#include "lwip/api.h"
#include <errno.h>

#ifndef EINPROGRESS
#define EINPROGRESS 115
#endif
extern int errno;


//static ip_addr_t serv_addr;
//static unsigned short api_port = 21080;

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


}

int create_socket_connection(int sock,const char* ip, unsigned short port, unsigned int timeout){

	struct sockaddr_in server_addr;
		sock = socket(AF_INET, SOCK_STREAM, 0);
		if (sock < 0) {
		    return -1; // Failed to create a socket
		}

		memset(&server_addr, 0, sizeof(server_addr));
		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		inet_pton(AF_INET, ip, &server_addr.sin_addr);


		//settings (should not be here)
		struct timeval tv;
		tv.tv_sec = 0;
		tv.tv_usec = 2000;
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		int flags = fcntl(sock, F_GETFL, 0);
		fcntl(sock, F_SETFL, flags | O_NONBLOCK);

		// Connect (non-blocking)
		connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr));
		if (errno == EINPROGRESS) {
		        // connection in progress
		        fd_set wfds;
		        FD_ZERO(&wfds);
		        FD_SET(sock, &wfds);
		        struct timeval timeout = {3, 0};  // 3s timeout

		        int ret = select(sock + 1, NULL, &wfds, NULL, &timeout);
		        if (ret > 0 && FD_ISSET(sock, &wfds)) {
		            // check if connection succeeded
		            int so_error = 0;
		            socklen_t len = sizeof(so_error);
		            getsockopt(sock, SOL_SOCKET, SO_ERROR, &so_error, &len);
		            if (so_error == 0) {
		            	return 0;
		            } else {
		                // connection failed
		                close(sock);
		                return -2;
		            }
		        } else {
		            // timeout or select error
		            close(sock);
		            return -3;
		        }
		}
	close(sock);
	return -1;
}
//HTTP Part

#include "json_parser.h"

void http_init(HttpClient_t* client,int sock,char* tx_buff,size_t tx_size, char* rx_buff, size_t rx_size,char* resp_jason, size_t resp_jason_size){
	client->socket = sock;
	client->send_buffer = tx_buff;
	client->send_buffer_size = tx_size;
	client->recv_buffer = rx_buff;
	client->recv_buffer_size = rx_size;
	client->response_json=resp_jason;
	client->response_json_size=resp_jason_size;
	client->error=0;
}
int http_post_request(HttpClient_t* client,int header_len,int content_len){
	int response_len;
	_Bool cont=0;
	int h=send(client->socket, client->send_buffer,header_len , 0);
	if (h==header_len){
		int	total_bytes_received=0;
		do{
			int bytes_received=recv(client->socket,client->recv_buffer,client->recv_buffer_size-1,0);
			if (bytes_received > 0) {
				if(!cont){//first wait for HTTP 100 continue
					if(http_continue(client->recv_buffer)){
						memset(client->recv_buffer,0,client->recv_buffer_size);
						int p=send(client->socket, client->send_buffer+header_len,content_len , 0);
						if(p==content_len)cont=1;
						else return -2;
					}
				}
				else{//then wait for HTTP 200 OK
					response_len=extract_json_content(client->recv_buffer,client->response_json,client->response_json_size);
					if(response_len >= 0)return 0;
					else continue;
				}

				// Check if we have space left in our main buffer
				if (total_bytes_received + bytes_received < 2048) {
					total_bytes_received += bytes_received;

				} else {
					// Error: Response is larger than our buffer
					// Handle this error (e.g., log it)
					return -3;
				}
				}
			else if (bytes_received < 0) {
				// An error occurred (bytes_received < 0)
				// Handle socket error (e.g., log it, break)
				if(errno==11){
					osDelay(10);
					continue;
				}
				else return -4;
			}
			else {
				// Connection was closed by the server.
				return -5;
			}
			osDelay(20);
		}while(1);
	}
	else return -1;
}
