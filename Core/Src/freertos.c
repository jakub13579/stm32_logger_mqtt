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

#define MQTT_SEND_BUF_SIZE 512
#define MQTT_RECV_BUF_SIZE 512

extern struct netif gnetif;
osSemaphoreId lwipReadySemHandle;


static unsigned char mqtt_sendbuf[MQTT_SEND_BUF_SIZE] __attribute__((aligned(4)));
static unsigned char mqtt_recvbuf[MQTT_RECV_BUF_SIZE];

typedef enum {
    MQTT_STATE_INIT = 0,
    MQTT_STATE_NETCONN_CREATE,
    MQTT_STATE_NETCONN_CONNECT,
    MQTT_STATE_MQTT_INIT,
    MQTT_STATE_MQTT_CONNECT,
    MQTT_STATE_MQTT_LOOP,
    MQTT_STATE_ERROR,
	MQTT_STATE_MQTT_PUBLISH
} mqtt_state_t;

mqtt_state_t mqtt_state;
err_t error;
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
osThreadId tcpClientHandle;
osThreadId mqttClientTaskHandle;

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void const * argument);
void tcpClientStart(void const * argument);
void mqttStartClient(void const * argument);

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
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
	 osSemaphoreDef(lwipReadySem);
	    lwipReadySemHandle = osSemaphoreCreate(osSemaphore(lwipReadySem), 1);
	    osSemaphoreWait(lwipReadySemHandle, 0);

  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* definition and creation of defaultTask */
  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  defaultTaskHandle = osThreadCreate(osThread(defaultTask), NULL);

  /* definition and creation of tcpClient */
  ///!! check if uncommented
  //osThreadDef(tcpClient, tcpClientStart, osPriorityNormal, 0, 1024);
  //tcpClientHandle = osThreadCreate(osThread(tcpClient), NULL);

  /* definition and creation of mqttClientTask */
  osThreadDef(mqttClientTask, mqttStartClient, osPriorityBelowNormal, 0, 1024);
  mqttClientTaskHandle = osThreadCreate(osThread(mqttClientTask), NULL);

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
  extern osSemaphoreId lwipReadySemHandle;
  osSemaphoreRelease(lwipReadySemHandle);
  /* Infinite loop */
  for(;;)
  {
	HAL_GPIO_TogglePin(LD1_GPIO_Port, LD1_Pin);
    osDelay(1000);
  }
  /* USER CODE END StartDefaultTask */
}

/* USER CODE BEGIN Header_tcpClientStart */
/**
* @brief Function implementing the tcpClient thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_tcpClientStart */
void tcpClientStart(void const * argument)
{
  /* USER CODE BEGIN tcpClientStart */
	    struct netconn *conn;
	    err_t err, connect_err;
	    static ip_addr_t serv_addr;
	    static unsigned short dest_port;
	    char msg[40];
	    int msg_cnt = 0;

	    LWIP_UNUSED_ARG(argument);

	    IP4_ADDR(&serv_addr, 192, 168, 1, 10);
	    dest_port = 1883;

	    extern osSemaphoreId lwipReadySemHandle;
	    osSemaphoreWait(lwipReadySemHandle, osWaitForever);

	    while(1)
	    {
	        // Wait for network to be ready
	        while (!netif_is_link_up(&gnetif)) {
	        	osDelay(100);
	        }
	        //flash a diode to signal a new connection attempt
	        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,1);
	        osDelay(200);
	        HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,0);

	        // Create a new connection
	        conn = netconn_new(NETCONN_TCP);

	        if (conn != NULL)
	        {

	        	netconn_bind(conn, NULL, 7);

	        	connect_err = netconn_connect(conn, &serv_addr, dest_port);
	        	//netconn_err(conn)
	        	if(connect_err==ERR_OK)
	        	{
				for (;;)
					{
						HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
						msg_cnt++;

						int msg_len = sprintf(msg, "Message#%03d Hello from STM\r\n", msg_cnt);
						err = netconn_write(conn, msg, msg_len, NETCONN_COPY);

						if (err != ERR_OK)
							break; // lost connection or error

						osDelay(500);
					}
				}
	        	HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,1);
				osDelay(200);
				HAL_GPIO_WritePin(LD2_GPIO_Port, LD2_Pin,0);
	            //delete connection
	            netconn_close(conn);
	            netconn_delete(conn);
	        }

	        // Retry connection every 5 seconds if failed
	        osDelay(2000);

	    }


  /* USER CODE END tcpClientStart */
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
	mqtt_state=MQTT_STATE_INIT;
	mqttc_net_t sock;
	struct mqtt_client client;
	err_t err;
	ip_addr_t broker_ip;
	const unsigned short broker_port = 1883;
	int msg_cnt = 0;
	LWIP_UNUSED_ARG(argument);

	IP4_ADDR(&broker_ip, 192, 168, 1, 10);

	extern osSemaphoreId lwipReadySemHandle;
	osSemaphoreWait(lwipReadySemHandle, osWaitForever);

	while(1)
	{
		// Wait for network to be ready
		while (!netif_is_link_up(&gnetif)) {
			osDelay(100);
		}

		//flash a diode to signal a new connection attempt
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,1);
		osDelay(200);
		HAL_GPIO_WritePin(LD3_GPIO_Port, LD3_Pin,0);

		// Create a new connection
		mqtt_state=MQTT_STATE_NETCONN_CREATE;
		sock.conn = netconn_new(NETCONN_TCP);

		if (sock.conn != NULL)
		{	//netconn_bind(conn, NULL, 7);//redundant
			/* Set short recv timeout so MQTT sync loop doesn’t block */
			netconn_set_recvtimeout(sock.conn, 20);
			mqtt_state=MQTT_STATE_NETCONN_CONNECT;
			err = netconn_connect(sock.conn, &broker_ip, broker_port);
			error=err;
			if(err==ERR_OK){

				/* Initialize MQTT */
				mqtt_state=MQTT_STATE_MQTT_INIT;
				error=mqtt_init(&client, &sock, mqtt_sendbuf, sizeof(mqtt_sendbuf), mqtt_recvbuf, sizeof(mqtt_recvbuf), NULL);

				/* Connect to broker */
				mqtt_state=MQTT_STATE_MQTT_CONNECT;
				error=mqtt_connect(&client, "stm32client", NULL, NULL, 0, NULL, NULL, 0, 400);
				while (1)
				        {	//mqtt_state=MQTT_STATE_MQTT_LOOP;
				            //mqtt_sync(&client);
				            //printf("something");
				            HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
							msg_cnt++;
				            char msg[28];
				            int msg_len = sprintf(msg, "Message#%03d Hello from STM\r\n", msg_cnt);
				            //mqtt_state=MQTT_STATE_MQTT_PUBLISH;
				            mqtt_publish(&client, "stm32/test", msg, msg_len, MQTT_PUBLISH_QOS_0);
				            mqtt_sync(&client);

				            if(client.error != MQTT_OK){

				            	mqtt_state=MQTT_STATE_ERROR;
				            	if(client.error == MQTT_ERROR_SEND_BUFFER_IS_FULL)continue;
				            	break;
				            }
				            //osDelay(100);
				        }

			}
		}
		netconn_close(sock.conn);
		netconn_delete(sock.conn);
		osDelay(2000);
	}

	/* USER CODE BEGIN mqttStartClient */
  /* Infinite loop */
  for(;;)
  {
    osDelay(1);
  }
  /* USER CODE END mqttStartClient */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

