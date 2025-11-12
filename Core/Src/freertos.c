/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lwip/opt.h"
#include "lwip/api.h"
#include "lwip/sys.h"
#include <string.h>
#include <stdio.h>
#include "mqtt.h"
#include "mqtt_pal.h"
#include "lwip/netif.h"
#include "usart.h"
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include "mqtt_client.h"
#include "json_parser.h"

#define MQTT_SEND_BUF_SIZE 2048
#define MQTT_RECV_BUF_SIZE 512

extern struct netif gnetif;
osSemaphoreId lwipReadySemHandle;
osSemaphoreId httpSetupReadySemHandle;
mqtt_pal_mutex_t mqttClientMutex;


static unsigned char mqtt_sendbuf[MQTT_SEND_BUF_SIZE] __attribute__((aligned(4)));
static unsigned char mqtt_recvbuf[MQTT_RECV_BUF_SIZE];


#define HTTP_SEND_BUF_SIZE 1024
#define HTTP_RECV_BUF_SIZE 1024

static char http_sendbuf[HTTP_SEND_BUF_SIZE];
static char http_recvbuf[HTTP_RECV_BUF_SIZE];


typedef enum {
    MQTT_STATE_INIT = 0,
    MQTT_STATE_NETCONN_CREATE,
    MQTT_STATE_NETCONN_CONNECT,
    MQTT_STATE_MQTT_INIT,
    MQTT_STATE_MQTT_CONNECT,
	MQTT_STATE_CONNECTING,
    MQTT_STATE_MQTT_LOOP,
    MQTT_STATE_ERROR,
	MQTT_STATE_MQTT_PUBLISH,
	MQTT_STATE_SYNC

} mqtt_state_t;

mqtt_state_t mqtt_state;
struct mqtt_client client;
err_t error;
_Bool start_process=0;
int msg_cnt = 0;
char error_msg[128];
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

/* USER CODE END Variables */
osThreadId defaultTaskHandle;
uint32_t defaultTaskBuffer[ 512 ];
osStaticThreadDef_t defaultTaskControlBlock;
osThreadId mqttClientTaskHandle;
uint32_t mqttClientTaskBuffer[ 2048 ];
osStaticThreadDef_t mqttClientTaskControlBlock;
osThreadId dataCollectionHandle;
uint32_t dataCollectionBuffer[ 2048 ];
osStaticThreadDef_t dataCollectionControlBlock;
osThreadId RTOS_UART_TASKHandle;
uint32_t RTOS_UART_TASKBuffer[ 512 ];
osStaticThreadDef_t RTOS_UART_TASKControlBlock;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void mqttStartClient(void const * argument);
void StartDataCollection(void const * argument);
void StartTask04(void const * argument);

extern void MX_LWIP_Init(void);
void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = &xIdleStack[0];
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
	osMutexDef(mqttClientMutex);
	mqttClientMutex = osMutexCreate(osMutex(mqttClientMutex));
	client.mutex = mqttClientMutex; // assign to client
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
	osSemaphoreDef(lwipReadySem);
	lwipReadySemHandle = osSemaphoreCreate(osSemaphore(lwipReadySem), 1);
	osSemaphoreWait(lwipReadySemHandle, 0);

	osSemaphoreDef(httpSetupReadySem);
	httpSetupReadySemHandle = osSemaphoreCreate(osSemaphore(httpSetupReadySem), 1);
	osSemaphoreWait(httpSetupReadySemHandle, 0);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadStaticDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 512, defaultTaskBuffer, &defaultTaskControlBlock);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of mqttClientTask */
  osThreadStaticDef(mqttClientTask, mqttStartClient, osPriorityNormal, 0, 2048, mqttClientTaskBuffer, &mqttClientTaskControlBlock);
  mqttClientTaskHandle = osThreadCreate(osThread(mqttClientTask), NULL);

  /* definition and creation of dataCollection */
  osThreadStaticDef(dataCollection, StartDataCollection, osPriorityRealtime, 0, 2048, dataCollectionBuffer, &dataCollectionControlBlock);
  dataCollectionHandle = osThreadCreate(osThread(dataCollection), NULL);

  /* definition and creation of RTOS_UART_TASK */
  //osThreadStaticDef(RTOS_UART_TASK, StartTask04, osPriorityBelowNormal, 0, 512, RTOS_UART_TASKBuffer, &RTOS_UART_TASKControlBlock);
  //RTOS_UART_TASKHandle = osThreadCreate(osThread(RTOS_UART_TASK), NULL);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void const * argument)
{
  /* init code for LWIP */
  MX_LWIP_Init();
  /* USER CODE BEGIN StartDefaultTask */
  osSemaphoreRelease(lwipReadySemHandle);

  osSemaphoreWait(httpSetupReadySemHandle, osWaitForever);
  /* Infinite loop */
  for(;;)
  {
	osSemaphoreWait(httpSetupReadySemHandle, osWaitForever);
	uint32_t time_start=HAL_GetTick();
	uint32_t time_ms=0;
	msg_cnt=0;
	start_process=0;
	if(HAL_GPIO_ReadPin(USER_Btn_GPIO_Port, USER_Btn_Pin)==GPIO_PIN_SET)start_process=1;


	while(client.error==MQTT_OK && start_process && msg_cnt<=10000){
		char msg[64];
		time_ms=HAL_GetTick()-time_start;
		msg_cnt++;
		int msg_len = sprintf(msg, "Message#%d Hello from STM\r\n ,Time:%lu\r\n", msg_cnt,time_ms);

		mqtt_publish(&client, "stm32/test", msg, msg_len, MQTT_PUBLISH_QOS_0);
		osDelay(20);
	}

    osDelay(100);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_mqttStartClient */
/**
* @brief Function implementing the mqttClientTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_mqttStartClient */
void mqttStartClient(void const * argument)
{
  /* USER CODE BEGIN mqttStartClient */
	mqtt_state=MQTT_STATE_INIT;
	const unsigned short broker_port = 1883;
	LWIP_UNUSED_ARG(argument);

	osSemaphoreWait(lwipReadySemHandle, osWaitForever);

	while(1)
	{


		// Wait for network to be ready
		while (!netif_is_link_up(&gnetif)) {
			osDelay(100);
		}

		//flash a diode to signal a new connection attempt
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,1);
		osDelay(100);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,0);
		mqtt_state=MQTT_STATE_NETCONN_CONNECT;


		// Create a new connection;
		int sock_fd;
		if(create_socket_connection(sock_fd, "192.168.1.10", broker_port, 3)!=0)continue;

		/* Initialize MQTT */
		mqtt_state=MQTT_STATE_MQTT_INIT;
		error=mqtt_init(&client, &sock_fd, mqtt_sendbuf, sizeof(mqtt_sendbuf), mqtt_recvbuf, sizeof(mqtt_recvbuf), NULL);

		/* Connect to broker */
		mqtt_state=MQTT_STATE_MQTT_CONNECT;
		error=mqtt_connect(&client, "stm32client", NULL, NULL, 0, NULL, NULL, 0, 400);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,1);
		while (client.error == MQTT_OK)
			{
				mqtt_sync(&client);
				mqtt_state=MQTT_STATE_MQTT_PUBLISH;
				osDelay(10);
			}
		mqtt_disconnect(&client);
		close(sock_fd);
		HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,0);
	}


  /* Infinite loop */
  for(;;)
  {
    osDelay(100);
  }
  /* USER CODE END mqttStartClient */
}

/* USER CODE BEGIN Header_StartDataCollection */
/**
* @brief Function implementing the dataCollection thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDataCollection */
__weak void StartDataCollection(void const * argument)
{
  /* USER CODE BEGIN StartDataCollection */
  /* Infinite loop */
	const short int api_port=21080;
	const char url_login[]="/v1/user/login";
	char target_ip[]="192.168.1.10";
	char response_json[512];
	int sock_http=0;
	_Bool success;
	char msg[128];
	char access_token[128];

	osSemaphoreWait(lwipReadySemHandle, osWaitForever);
	while(1){
		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin,1);
		osDelay(100);
		HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin,0);

		if(create_socket_connection(sock_http,target_ip, api_port, 3)<0)continue;
		break;
  }


	char login_json[128];

	HttpClient_t http_client;

	http_init(&http_client, sock_http, http_sendbuf, HTTP_SEND_BUF_SIZE, http_recvbuf , HTTP_RECV_BUF_SIZE, response_json, 512);

	int content_len=json_login_message(login_json,  "admin", "admin12345", sizeof(login_json));

	int msg_len=create_post_request(http_sendbuf, 2048, url_login, target_ip, api_port,"", login_json);
	int header_len=msg_len-content_len;



	HAL_GPIO_WritePin(LD1_GPIO_Port, LD1_Pin,1);
	int i=0;
	http_client.error=http_post_request(&http_client, header_len, content_len);
		if(http_client.error==0){
			parse_json_string(response_json,"\"message\"",msg,sizeof(msg));
			parse_json_string(response_json, "\"access_token\"" ,access_token, sizeof(access_token));
			i=parse_json_bool(response_json, "\"success\"" ,&success);

		}
		for(;;)osDelay(1000);
	close(sock_http);

  /* USER CODE END StartDataCollection */
}

/* USER CODE BEGIN Header_StartTask04 */
/**
* @brief Function implementing the RTOS_UART_TASK thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartTask04 */
__weak void StartTask04(void const * argument)
{
  /* USER CODE BEGIN StartTask04 */
  /* Infinite loop */
	 uint8_t buffIn[64];
	    const uint8_t errorMsg[]   = "ERROR: command not recognized\r\n";
	    const uint8_t successMsg[] = "SUCCESS: process will start\r\n";

	    size_t idx = 0;
	    HAL_StatusTypeDef status;
	    const uint32_t perByteTimeout = 200;    // ms to wait for each byte
	    const uint32_t overallTimeout = 2000;   // ms max to wait for full command

	    for (;;)
	    {
	        idx = 0;
	        uint32_t t_start = HAL_GetTick();
	        _Bool gotCommand = 0;

	        while ((HAL_GetTick() - t_start) < overallTimeout && idx < (sizeof(buffIn) - 1))
	        {
	            uint8_t byte;
	            status = HAL_UART_Receive(&huart3, &byte, 1, perByteTimeout);

	            if (status == HAL_OK)
	            {
	                // Ignore CR (carriage return)
	                if (byte == '\r') {
	                    continue;
	                }

	                // If newline -> end of command
	                if (byte == '\n') {
	                    gotCommand = 1;
	                    break;
	                }
	                // store and continue
	                buffIn[idx++] = byte;
	                // reset overall timeout so slow typing within a command is allowed
	                t_start = HAL_GetTick();
	            }
	            else
	            {
	                // no byte received within perByteTimeout, loop and check overall timeout
	            }
	        }

	        // If we have at least one character or gotCommand via newline, process buffer
	        if (idx > 0 || gotCommand)
	        {
	            buffIn[idx] = '\0'; // null-terminate

	            // Optional: trim trailing whitespace
	            while (idx > 0 && (buffIn[idx-1] == ' ' || buffIn[idx-1] == '\t')) {
	                idx--;
	                buffIn[idx] = '\0';
	            }

	            if (strcmp((char*)buffIn, "start") == 0)
	            {
	                HAL_UART_Transmit(&huart3, (uint8_t*)successMsg, strlen((char*)successMsg), HAL_MAX_DELAY);
	                HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
	            }
	            else
	            {
	                HAL_UART_Transmit(&huart3, (uint8_t*)errorMsg, strlen((char*)errorMsg), HAL_MAX_DELAY);
	                HAL_GPIO_TogglePin(LD3_GPIO_Port, LD3_Pin);
	            }
	        }

	  osDelay(10);
  }
  /* USER CODE END StartTask04 */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

