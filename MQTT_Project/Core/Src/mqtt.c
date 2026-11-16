#include "mqtt.h"

//连接报文
bool MQTT_CONNECT(char *client_id)
{
	
	uint8_t  buf[BUF_SIZE] = {0};  	//缓冲区
	uint16_t cnt = 0;								//计数器
	
	uint16_t client_len = strlen(client_id);
	
	//剩余长度 = 可变(10byte) + 有效载荷(UTF-8格式的客户端ID)
	uint16_t remainder_len = 10 + strlen(client_id) + 2;
	
	
	//1.构造固定报头
	buf[cnt++] = 0x10;		//报文类型
	
	do										//剩余长度
	{
		uint8_t encodedByte = remainder_len % 128;
		
		remainder_len = remainder_len / 128;
		
		// if there are more data to encode, set the top bit of this byte
		if ( remainder_len > 0 )
		{
			encodedByte = encodedByte | 128;
		}
		buf[cnt++] = encodedByte;
		
	}while ( remainder_len > 0 );
	
	//2.构造可变报头
	buf[cnt++] = 0;				//协议名称长度MSB
	buf[cnt++] = 4;				//协议名称长度LSB
	buf[cnt++] = 'M';			//协议名称
	buf[cnt++] = 'Q';			
	buf[cnt++] = 'T';			
	buf[cnt++] = 'T';			
	buf[cnt++] = 4;				//协议级别
	buf[cnt++] = 0x02;		//连接标志
	buf[cnt++] = 0;				//心跳保持时间MSB
	buf[cnt++] = 120;			//心跳保持时间LSB
	
	//3.构造有效载荷
	buf[cnt++] = MQTT_UTF8_MSB(client_len);			//客户端标识符长度MSB
	buf[cnt++] = MQTT_UTF8_LSB(client_len);			//客户端标识符长度LSB
	strcpy((char *)&buf[cnt],client_id);
	cnt += client_len;
	
	//4.发送报文
	USART2_ClearBuffer();
	HAL_UART_Transmit(&huart2,buf,cnt,100);
	
	//5.等待服务器的确认连接报文
	int i = 500;
	while(i--)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		
		
		//  报文类型               剩余长度
		if(u2_rxbuf[0] == 0x20 && u2_rxbuf[1] == 0x02)
		{
			//判断返回码
			switch(u2_rxbuf[3])
			{
				case 0x00:printf("连接已被服务端接受\r\n");return true;
				case 0x01:printf("服务端不支持客户端请求的MQTT协议级别\r\n");return false;
				case 0x02:printf("客户端标识符是正确的UTF-8编码，但服务端不允许使用\r\n");return false;
				case 0x03:printf("网络连接已建立，但MQTT服务不可用\r\n");return false;
				case 0x04:printf("用户名或密码的数据格式无效\r\n");return false;
				case 0x05:printf("客户端未被授权连接到此服务器\r\n");return false;
			}
			
		}
	}
	
	printf("连接超时，未收到服务器的确认连接报文\r\n");
	return false;
}

//订阅报文  巴法云只支持Qos0和Qos1
bool MQTT_SUBSCRIBE(char *topic,uint8_t Qos)
{
	
	//检查参数有效性
	if(topic == NULL && (Qos != 0 || Qos != 1))
	{
		printf("参数无效\r\n");
		return false;
	}
	
	uint8_t  buf[BUF_SIZE] = {0};  	//缓冲区
	uint16_t cnt = 0;								//计数器
	
	uint16_t topic_len = strlen(topic);
	
	//剩余长度 = 可变(2byte) + 有效载荷(UTF-8格式的主题名+服务质量)
	uint16_t remainder_len = 2 + strlen(topic) + 2 + 1;
	
	
	//1.构造固定报头
	buf[cnt++] = 0x82;		//报文类型
	
	do										//剩余长度
	{
		uint8_t encodedByte = remainder_len % 128;
		
		remainder_len = remainder_len / 128;
		
		// if there are more data to encode, set the top bit of this byte
		if ( remainder_len > 0 )
		{
			encodedByte = encodedByte | 128;
		}
		buf[cnt++] = encodedByte;
		
	}while ( remainder_len > 0 );
	
	//2.构造可变报头
	buf[cnt++] = 0;								//报文标识符长度MSB
	buf[cnt++] = topic_len;				//报文标识符长度LSB

	
	//3.构造有效载荷
	buf[cnt++] = MQTT_UTF8_MSB(topic_len);			//主题名称长度MSB
	buf[cnt++] = MQTT_UTF8_LSB(topic_len);			//主题名称长度LSB
	
	strcpy((char *)&buf[cnt],topic);
	
	cnt += topic_len;
	
	buf[cnt++] = Qos;							//服务质量
	
	//4.发送报文
	USART2_ClearBuffer();
	HAL_UART_Transmit(&huart2,buf,cnt,100);
	
	//5.等待服务器的确认连接报文
	int i = 500;
	while(i--)
	{
		vTaskDelay(pdMS_TO_TICKS(10));
		
		//  报文类型               剩余长度
		if(u2_rxbuf[0] == 0x90 && u2_rxbuf[1] == 0x03)
		{
			//判断返回码
			switch(u2_rxbuf[4])
			{
				case 0x00:printf("%s订阅成功，服务质量最大QoS0\r\n", topic);return true;
				case 0x01:printf("订阅成功，服务质量最大QoS1\r\n");return true;
				case 0x02:printf("订阅成功，服务质量最大QoS2\r\n");return true;
				case 0x80:printf("订阅失败\r\n");return false;
			}
			
		}
	}
	
	printf("连接超时，未收到服务器的订阅应答报文\r\n");
	return false;
}

//发布消息报文
void MQTT_Publish(char *topic,char *msg)
{

	uint8_t  buf[BUF_SIZE + 16U] = {0};  	//缓冲区
	uint16_t cnt = 0;								//计数器
	
	uint16_t topic_len = strlen(topic);
	
	//剩余长度 = 可变(UTF-8格式的主题名) + 有效载荷(消息内容)
	uint16_t remainder_len = 2 + strlen(topic) + strlen(msg);
	
	//1.构造固定报头
	buf[cnt++] = 0x30;		//报文类型
	do//剩余长度
	{
		uint8_t encodedByte = remainder_len % 128;
		
		remainder_len = remainder_len / 128;
		
		// if there are more data to encode, set the top bit of this byte
		if ( remainder_len > 0 )
		{
			encodedByte = encodedByte | 128;
		}
		buf[cnt++] = encodedByte;
		
	}while ( remainder_len > 0 );
	
	//2.构造可变报头
	buf[cnt++] = MQTT_UTF8_MSB(topic_len);			//主题名称长度MSB
	buf[cnt++] = MQTT_UTF8_LSB(topic_len);			//主题名称长度LSB
	
	strcpy((char *)&buf[cnt],topic);
	
	cnt += topic_len;
	
	//3.构造有效载荷
	strcpy((char *)&buf[cnt],msg);
	cnt += strlen(msg);
	
	//4.发送报文
	// USART2_ClearBuffer();
	HAL_UART_Transmit(&huart2,buf,cnt,100);
}