
#include "sys.h"
#include "usart.h"
#include "myiic.h"

#define TEM_SUM 8	//�¶ȴ�����������

u8 IfDebug = 0;

void USART_puts(uint8_t *s,uint8_t len)
{
    int i;
    for(i=0; i<len; i++)
    {
        putchar(s[i]);
    }
}

void delay_us(u32 time)
{    
	u32 i,j;
  
	for(j=0; j<time; j++)
	{
	   for(i=0;i<9;i++);
	}
   	
}

void delay_ms(u32 time)
{
	u32 i,j;
  
	for(j=0; j<time; j++)
	{
	   for(i=0;i<8000;i++);
	}
}

void i_start(void) 
{ 
	SDA_OUT();     //sda�����
	IIC_SDA=1;	  	  
	IIC_SCL=1;
	delay_us(4);
	IIC_SDA=0;//START:when CLK is high,DATA change form high to low 
	delay_us(4);
	IIC_SCL=0;//ǯסI2C���ߣ�׼�����ͻ�������� 
} 
void i_stop(void) 
{ 
	SDA_OUT();//sda�����
	IIC_SCL=0;
	IIC_SDA=0;//STOP:when CLK is high DATA change form low to high
 	delay_us(4);
	IIC_SCL=1; 
	IIC_SDA=1;//����I2C���߽����ź�
	delay_us(4);		 
} 
void i_init(void) 
{  
	SDA_OUT();//sda�����	
	IIC_SCL=0; 
	i_stop(); 
} 

u8 i_clock(void) 
{ 
	u8 sample; 
	delay_us(2); 	
	IIC_SCL=1; 
	delay_us(2);
	
	SDA_IN();//SDA����Ϊ����
	sample=READ_SDA; 
	delay_us(2); 
	IIC_SCL=0; 
	delay_us(2); 
	return(sample); 
}

void i_ack(void) 
{ 
	SDA_OUT();//sda�����	
	IIC_SDA=0; 
	i_clock(); 
	IIC_SDA=1; 
} 

u8 i_send(unsigned char i_data) 
{ 
  unsigned char i; 
	SDA_OUT();//sda�����
	IIC_SCL=0;//����ʱ�ӿ�ʼ���ݴ���	
	for(i=0;i<8;i++) 
	{ 
		if((i_data&0x80)>>7)
			IIC_SDA=1;
		else
			IIC_SDA=0;
		i_data<<=1; 
		i_clock(); 
	} 
	IIC_SDA=1; 
	if(i_clock())	//1����Ӧ��
		return 0;
	else			//0����Ӧ��
		return 1; 
}

unsigned char i_receive(void) 
{ 
  unsigned char i_data=0; 
  unsigned char i; 
  for(i=0;i<8;i++) 
    { 
      i_data*=2; 
      if(i_clock()) i_data++; 
    } 
  return(i_data); 
}

u8 start_temperature_T(char TemIdx) 
{ 
	i_start(); 
	if(i_send(0x90+ (TemIdx<<1))) 
	{ 
		if(i_send(0xee)) 
		{ 
			i_stop(); 
			delay_us(2);
			return(1); 
		} 
		else 
		{ 
			i_stop(); 
			delay_us(2);
			return(0); 
		} 
	} 
	else 
	{ 
		i_stop(); 
		delay_us(2); 
		return(0); 
	} 
} 

u8 read_temperature_T(char TemIdx, unsigned char *p) 
{ 
	i_start(); 
	if(i_send(0x90+ (TemIdx<<1))) 
	{ 
		if(i_send(0xaa)) 
		{ 
			i_start(); 
			if(i_send(0x91+ (TemIdx<<1))) 
			{ 
				*(p+1)=i_receive(); 
				i_ack(); 
				*p=i_receive(); 
				i_stop(); 
				delay_us(2); 
				return 1; 
			} 
			else 
			{ 
				i_stop(); 
				delay_us(2);
				return 0; 
			} 
		} 
		else 
		{ 
			i_stop(); 
			delay_us(2);
			return 0; 
		} 
	} 
	else 
	{ 
		i_stop(); 
		delay_us(2); 
		return 0; 
	} 
} 

void pagTemDataAndSend(u8 ID , double Tem)
{
	u8 temPag[]={0xAA , 0x55 , 0x00 , 0xDD , 0x00 , 0x08 , 0x00 , 0x00 , 0x00 , 0x00 , 0x56 , 0x78 , 0x9A , 0xBC , 0xDE , 0xF0 , 0x00 , 0x00 , 0x0D , 0x0A};
		
	temPag[3] = 0xD0 + ID;
	*(short *)(&temPag[8]) = (short)(Tem*100);
	USART_puts(temPag,sizeof(temPag));

	//printf("tem=%d\n",*(short *)(&temPag[8]));
}
	
 int main(void)
 {	 
	unsigned char temperdata[2]; 
	unsigned char result;
	double Temple[8];
	u8 TemIdx = 0;
	char ReturnValue = 0;
	u8 TemValid = 0; //��Ч���¶�ֵ����
	 
	 
	 /*72M */      
	SystemInit();
	IIC_Init();
	// i_init();
	uart_init(115200);	 	//���ڳ�ʼ��Ϊ115200

	
	while(1)
	{
		//��������DS1624
		for(TemIdx = 0;TemIdx < TEM_SUM;TemIdx++)	//��������DS1624
		{
			start_temperature_T(TemIdx);      //����DS1624��ʼ�¶�ת�� 
		}
		
		//�ȴ��������
		delay_ms(1000);
		
		//��ȡDS1624���¶�ֵ
		for(TemIdx = 0, TemValid = 0;TemIdx < TEM_SUM;TemIdx++)	//��������DS1624
		{
			//��ȡ�¶�
			ReturnValue = read_temperature_T(TemIdx, temperdata);      //��ȡDS1624���¶�ֵ 
			
			if(ReturnValue == 0 || *(uint16_t*)temperdata == 0)	//��Ӧ��ľ��������������޴��豸�����
				continue;
	
			//��ӡ��ȡ����������
			//printf("\ntemperdata[0]=%u\n", temperdata[0]); 		//������
			//printf("temperdata[1]=%u\n", temperdata[1]>>3);  	//������

			//�����¶�
			if (temperdata[1]&0x80)
			{
				Temple[TemIdx] = (double)(-((~temperdata[1]&0x7F)+1-(temperdata[0]>>3)*0.03125));
			}
			else
			{
				Temple[TemIdx]= (double)((temperdata[1]&0x7F)+(temperdata[0]>>3)*0.03125);
			}

			TemValid++;
			if(IfDebug == 1 && TemValid == 1) 	//��һ������
				printf("\n");
			
			if(IfDebug == 1)
				printf("Temple %d��%.2lf\n", TemIdx, Temple[TemIdx]); 	//������
			else
				pagTemDataAndSend(TemIdx, Temple[TemIdx]);	//����������¶�

			
				
		}
		
		
		
		
		
	}
}
