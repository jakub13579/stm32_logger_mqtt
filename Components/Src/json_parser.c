#include "json_parser.h"
#include "mqtt_client.h"

int parse_json_int(char* json, const char* key,int* out_value){
    char* pos=strstr(json,key);
    if (!pos)
        return -1;
    
    pos=strchr(pos,':');
    if (!pos)
        return -2;
    pos++;

     *out_value=atoi(pos);
     return 0;
}

int parse_json_float(char* json, const char* key, float* out_value){
    char* pos=strstr(json,key);
    if (!pos)
        return -1;

    pos=strchr(pos,':');
    if (!pos)
        return -2;
    pos++;

    *out_value=atof(pos);
    return 0;
}

int parse_json_string(char* json, const char* key, char* out_string, size_t max_len){
    char* pos = strstr(json, key);
    if (!pos)
        return -1;

    pos = strchr(pos, ':');
    if (!pos)
        return -2;
    pos++;

    char* start = strchr(pos, '"');
    if (!start) 
        return -3;
    start++;

    char* end = strchr(start, '"');
    if (!end) 
        return -4;

    int len = end - start;
    int truncated=0;
    if (len >=max_len){
        truncated=(int)(len - (max_len-1));
        len =max_len-1;
    }

    strncpy(out_string, start, len);
    out_string[len] = '\0';
    
    return truncated;
}

int parse_json_bool(char* json, const char* key, _Bool* out_value){
    char* pos=strstr(json,key);
    if (!pos)
        return -1;
    pos=strchr(pos,':');
    if (!pos)
        return -2;
    pos++;
    if (*pos == ' ') pos++;
    
    if (strncmp(pos,"true",4)==0)
        *out_value=1;
    else if (strncmp(pos,"false",4)==0)
        *out_value=0;

    return 0;
}

int json_login_message(char* message,const char* username,const char*password, size_t max_len){
    return sprintf(message, "{\"username\":\"%s\",\"password\":\"%s\"}", username, password);
}

 int create_get_request(char* buffer, const char* address, ipv4_addr_t ip, int port,const char* token){
return sprintf(buffer, "GET %s HTTP/1.1\r\n"
            "Host: %s:%d\r\n HTTP/1.1" // ip and port like so: 192.168.1.20:21080
            "Content-Type: application/json; charset=UTF-8\r\n" // type of data
            "User-Agent: STM32 Logger \r\n" //client info: STM32 Logger 
            "Accept-Encoding: gzip\r\n"
            "Accept: application/json, */*\r\n"
            "Authorization: Bearer %s\r\n", //token
             address, ip, port, token);
}

int create_bench_get_request(char* buffer, const char* address, ipv4_addr_t ip, int port,const char* token,int bench_id){
    char get_line[64];
    snprintf(get_line, 64, "GET %s?bench_id=%d HTTP/1.1\r\n", address, bench_id);
    return  create_get_request(buffer, get_line, ip, port, token);
 }
 

int create_post_request(char* buffer, int buffer_size,const char* address,const char* ip, int port,const char* token,const char* content){
    size_t content_len=strlen(content);
    char authorisation[128]={""};
    if(token)snprintf(authorisation,128,"Authorization: Bearer %s\r\n",token);
    int r=snprintf(buffer,buffer_size,"POST /v1/%s HTTP/1.1\r\n" //address
            "Host: %s:%d\r\n" // ip and port like so: 192.168.1.20:21080
            "Content-Type: application/json; charset=UTF-8\r\n" // type of data
            "User-Agent: STM32 Logger \r\n" //client info: STM32 Logger 
            "Accept-Encoding: gzip\r\n"
            "Accept: application/json, */*\r\n"
            "%s"//token line
            "Content-Length: %d\r\n" //msg_length-1
            "Expect: 100-continue\r\n" // expected response
            "\r\n"
            "%s",
            address, ip, port, authorisation, content_len, content);
    if (r < 0) return -3;
       if (r >= buffer_size) {
           // truncated
           return -4;
       }
       return r;
}
_Bool http_continue(char* http_message){
    char* pos = strstr(http_message,"HTTP/");
    if(!pos){
        return 0;//not a valid HTTP message
    }
    pos=strstr(pos," ");
    if(atoi(pos)!=100){
        return 0;//not a valid HTTP message
    }
    return 1;
}

int extract_json_content(char* http_message, char* json_buffer, size_t max_len){
    char* pos = strstr(http_message,"HTTP/");
    if(!pos){
        return -1;//not a valid HTTP message
    }
    pos=strstr(pos," ");

    int http_code=atoi(pos);
    if(http_code < 200 || http_code >= 300){ //http code  2**
        return -1;//not a valid HTTP message
    }

    pos = strstr(http_message,"Content-Length");
    if(!pos){
        return -2; // problem checking content lenght
    }
    pos = strchr(pos, ':');
    if (!pos)
        return -2;//problem checking content lenght
    pos++;
    int content_lenght= atoi(pos);

    pos= strstr(pos,"\r\n\r\n");
    if (!pos)
        return -5; //not enough data, try again later
    pos+=4;

    if (strlen(pos)<content_lenght){
        return -5; //not enough data, try again later
    }


    if (content_lenght >= max_len){
        content_lenght = max_len - 1;
    }
    strncpy(json_buffer,pos,content_lenght);
    json_buffer[content_lenght]='\0';

    return content_lenght;
}

static const char* json_template[] = {
    "{\"name\":\"%s\"}",  						//0
    "{\"bench_idbench\":%d,\"version\":\"%s\"}", //1
    "{\"idbench_version\":%d}",					 //2
    "{\"name\":\"%s\",\"bench_idbench\":%d}",	 //3
    "{\"value\":\"%d\",\"status\":\"Active\",\"fault_idfault\":%d," //4
        "\"fault_bench_idbench\":%d,\"bench_idbench\":%d}",
    "{\"name\":\"%s\",\"bench_idbench\":%d}", 				//5
    "{\"idcondition\":%d,\"value\":\"%d\",\"status\":\"Active\"," //6
        "\"condition_idcondition\":%d,\"condition_bench_idbench\":%d,\"bench_idbench\":%d}",
    "{\"bench_version_idbench_version\":%d,\"selected_combinations\":[0],"//7
        "\"repetitions\":1,\"duration_seconds\":15,\"prediction_enabled\":1,"
        "\"prediction_protocol\":\"mqtt\",\"prediction_batch_type\":\"count\","
        "\"prediction_batch_config\":3,\"mqtt_topic\":\"%s\"}"
};


extern int bench_id;
extern int bench_version_id;
extern int condition_id;
extern int condition_value_id;
extern int fault_value[];
extern int fault_id;
extern int fault_value_id[];
extern int condition[];
extern char version_token[];
extern char mqtt_topic[];
extern const char bench_name[];
extern const char* bench_version_name[];
extern const char* condition_name[];
extern const char* fault_name[];
extern _Bool create_session_success;

int json_bench(char* out, size_t out_size, int flag) {
	return snprintf(out, out_size,json_template[0],bench_name);
}

int json_bench_version(char* out, size_t out_size, int flag) {
	if(bench_id < 0)return 0;
	return snprintf(out, out_size,json_template[1],bench_id,bench_version_name);
}

int json_bench_version_token(char* out, size_t out_size, int flag) {
	if(bench_version_id < 0)return 0;
	return snprintf(out, out_size,json_template[2],bench_version_id);
}
int json_fault(char* out, size_t out_size, int flag) {
	if(bench_id < 0)return 0;
	return snprintf(out, out_size,json_template[3],fault_name,bench_id);
}

int json_fault_value(char* out, size_t out_size, int flag) {
	if(bench_id < 0||fault_id < 0)return 0;
	return snprintf(out, out_size,json_template[4],fault_value[flag],fault_id,bench_id,bench_id);
}

int json_condition(char* out, size_t out_size, int flag) {
	if(bench_id < 0)return 0;
	return snprintf(out, out_size,json_template[5],condition_name, bench_id);
}

int json_condition_value(char* out, size_t out_size, int flag) {
	if(bench_id < 0||condition_id < 0)return 0;
	return snprintf(out, out_size,json_template[6], condition_id,condition[1],condition_id,bench_id,bench_id);
}

int json_create_session(char* out, size_t out_size, int flag) {
	if(bench_id < 0)return 0;
	return snprintf(out, out_size,json_template[7],bench_id ,mqtt_topic);
}

HttpRequestFunc steps[]={
		json_bench,
		json_bench_version,
		json_bench_version_token,
		json_fault,
		json_fault_value,
		json_condition,
		json_condition_value,
		json_create_session
};

//parsing steps
int parse_bench(HttpClient_t client) {
	return parse_json_int(client.response_json, "idbench", &bench_id);
}

int parse_bench_version(HttpClient_t client) {
	return parse_json_int(client.response_json, "\"idbench_version\"", &bench_version_id);
}

int parse_bench_version_token(HttpClient_t client) {
	return parse_json_string(client.response_json, "\"bench_token\"", version_token,128);
}
int parse_fault(HttpClient_t client) {
	return parse_json_int(client.response_json, "\"idfault\"", &fault_id);
}

int parse_fault_value(HttpClient_t client) {
	return parse_json_int(client.response_json, "\"idfault_value\"", fault_value_id);
}

int parse_condition(HttpClient_t client) {
	return parse_json_int(client.response_json, "\"idcondition\"", &condition_id);
}

int parse_condition_value(HttpClient_t client) {
	return parse_json_int(client.response_json, "\"idcondition_value\"", &condition_value_id);
}

int parse_create_session(HttpClient_t client) {
	return parse_json_bool(client.response_json, "\"success\"", &create_session_success);
}

HttpParseFunc parse_steps[]={
		parse_bench,
		parse_bench_version,
		parse_bench_version_token,
		parse_fault,
		parse_fault_value,
		parse_condition,
		parse_condition_value,
		parse_create_session
};
