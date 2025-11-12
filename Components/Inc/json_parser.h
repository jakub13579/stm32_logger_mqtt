#ifndef JSON_PARESER_H
#define JSON_PARSER_H

#include <stdio.h>
#include<string.h>
#include<stdlib.h>
typedef char* ipv4_addr_t;


int parse_json_int (char* json, const char* key, int* out_value);
int parse_json_float (char* json, const char* key, float* out_value);
int parse_json_bool(char* json, const char* key, _Bool* out_value);
int parse_json_string (char* json, const char* key, char* out_string, size_t max_len);
int create_post_request(char* buffer, int buffer_size,const char* address,const char* ip, int port,const char* token,const char* content);
int json_login_message(char* message,const char* username,const char*password, size_t max_len);
int create_bench_get_request(char* buffer, const char* address, ipv4_addr_t ip, int port,const char* token,int bench_id);
int extract_json_content(char* http_message, char* json_buffer, size_t max_len);
_Bool http_continue(char* http_message);

#endif
