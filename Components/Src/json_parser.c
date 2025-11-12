#include "json_parser.h"


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
    int r=sprintf(buffer,"POST %s HTTP/1.1\r\n" //address
            "Host: %s:%d\r\n" // ip and port like so: 192.168.1.20:21080
            "Content-Type: application/json; charset=UTF-8\r\n" // type of data
            "User-Agent: STM32 Logger \r\n" //client info: STM32 Logger 
            "Accept-Encoding: gzip\r\n"
            "Accept: application/json, */*\r\n"
            "Authorization: Bearer %s\r\n" //token
            "Content-Length: %d\r\n" //msg_length-1
            "Expect: 100-continue\r\n" // expected response
            "\r\n"
            "%s",
            address, ip, port, token, content_len, content);
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
    if(atoi(pos)!=200){
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
