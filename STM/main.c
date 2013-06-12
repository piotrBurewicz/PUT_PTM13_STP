#include <stm32f4xx_conf.h>
#include <stm32f4xx.h>
#include <misc.h>
#include <stm32f4xx_usart.h>
#include <stm32f4xx_gpio.h>
#include <stm32f4xx_rcc.h>
#include <stm32f4xx_crc.h>
#include <stdlib.h>

//#define MAX_STRLEN 12 		 // this is the maximum string length of our string in characters
typedef struct stp_data stp_data;
volatile char received_string[4];
volatile char data[65536];
volatile int CURRENT_LENGTH;
volatile int CURRENT_ARG;
volatile int READY_FLAG = 0;

void Delay(__IO uint32_t nCount) {
	while (nCount--) {
	}
}

struct stp_data {
	char f_data[65536];
	int arg;
};

void init_GPIO() {
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOD, ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14
			| GPIO_Pin_15;
	GPIO_Init(GPIOD, &GPIO_InitStructure);
}

/**
 * This funcion initializes the USART1 peripheral
 *
 * Arguments: baudrate --> the baudrate at which the USART is
 * 						   supposed to operate
 */
void init_USART1(uint32_t baudrate) {

	GPIO_InitTypeDef GPIO_InitStruct; // this is for the GPIO pins used as TX and RX
	USART_InitTypeDef USART_InitStruct; // this is for the USART1 initilization
	NVIC_InitTypeDef NVIC_InitStructure; // this is used to configure the NVIC (nested vector interrupt controller)

	/**
	 * enable APB2 peripheral clock for USART1
	 * note that only USART1 and USART6 are connected to APB2
	 * the other USARTs are connected to APB1
	 */
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

	/**
	 * enable the peripheral clock for the pins used by
	 * USART1, PB6 for TX and PB7 for RX
	 */
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

	/**
	 * This sequence sets up the TX and RX pins
	 * so they work correctly with the USART1 peripheral
	 */
	GPIO_InitStruct.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // Pins 6 (TX) and 7 (RX) are used
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF; // the pins are configured as alternate function so the USART peripheral has access to them
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz; // this defines the IO speed and has nothing to do with the baudrate!
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP; // this defines the output type as push pull mode (as opposed to open drain)
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP; // this activates the pullup resistors on the IO pins
	GPIO_Init(GPIOB, &GPIO_InitStruct); // now all the values are passed to the GPIO_Init() function which sets the GPIO registers

	/**
	 * The RX and TX pins are now connected to their AF
	 * so that the USART1 can take over control of the
	 * pins
	 */
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource6, GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOB, GPIO_PinSource7, GPIO_AF_USART1);

	/**
	 * Now the USART_InitStruct is used to define the
	 * properties of USART1
	 */
	USART_InitStruct.USART_BaudRate = baudrate; // the baudrate is set to the value we passed into this init function
	USART_InitStruct.USART_WordLength = USART_WordLength_8b; // we want the data frame size to be 8 bits (standard)
	USART_InitStruct.USART_StopBits = USART_StopBits_1; // we want 1 stop bit (standard)
	USART_InitStruct.USART_Parity = USART_Parity_No; // we don't want a parity bit (standard)
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None; // we don't want flow control (standard)
	USART_InitStruct.USART_Mode = USART_Mode_Tx | USART_Mode_Rx; // we want to enable the transmitter and the receiver
	USART_Init(USART1, &USART_InitStruct); // again all the properties are passed to the USART_Init function which takes care of all the bit setting

	/**
	 * Here the USART1 receive interrupt is enabled
	 * and the interrupt controller is configured
	 * to jump to the USART1_IRQHandler() function
	 * if the USART1 receive interrupt occurs
	 */
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE); // enable the USART1 receive interrupt

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn; // we want to configure the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0; // this sets the priority group of the USART1 interrupts
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0; // this sets the subpriority inside the group
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; // the USART1 interrupts are globally enabled
	NVIC_Init(&NVIC_InitStructure); // the properties are passed to the NVIC_Init function which takes care of the low level stuff

	// finally this enables the complete USART1 peripheral
	USART_Cmd(USART1, ENABLE);
}

/**
 * This function is used to transmit a string of characters via
 * the USART specified in USARTx.
 *
 * It takes two arguments: USARTx --> can be any of the USARTs e.g. USART1, USART2 etc.
 * 						   (volatile) char *s is the string you want to send
 *
 * Note: The string has to be passed to the function as a pointer because
 * 		 the compiler doesn't know the 'string' data type. In standard
 * 		 C a string is just an array of characters
 *
 * Note 2: At the moment it takes a volatile char because the received_string variable
 * 		   declared as volatile char --> otherwise the compiler will spit out warnings
 * */
void USART_puts(USART_TypeDef* USARTx, volatile char *s) {

	while (*s) {
		//GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
		// wait until data register is empty
		while (!(USARTx->SR & 0x00000040))
			;
		USART_SendData(USARTx, *s);
		*s++;
		//GPIO_ResetBits(GPIOD, GPIO_Pin_12);
	}
}

void USART_putByte(USART_TypeDef* USARTx, volatile uint8_t ch) {

	// wait until data register is empty
	while (!(USARTx->SR & 0x00000040))
		;
	/* Transmit Data */
	USARTx->DR = (ch & (uint16_t) 0x01FF);
	// USART_SendData(USART1, (uint8_t) ch);

}

	 uint32_t crcTable[256] = { 0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
		0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3, 0x0EDB8832, 0x79DCB8A4,
		0xE0D5E91E, 0x97D2D988, 0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
		0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE, 0x1ADAD47D, 0x6DDDE4EB,
		0xF4D4B551, 0x83D385C7, 0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
		0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5, 0x3B6E20C8, 0x4C69105E,
		0xD56041E4, 0xA2677172, 0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
		0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940, 0x32D86CE3, 0x45DF5C75,
		0xDCD60DCF, 0xABD13D59, 0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
		0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F, 0x2802B89E, 0x5F058808,
		0xC60CD9B2, 0xB10BE924, 0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,
		0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A, 0x71B18589, 0x06B6B51F,
		0x9FBFE4A5, 0xE8B8D433, 0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
		0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01, 0x6B6B51F4, 0x1C6C6162,
		0x856530D8, 0xF262004E, 0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
		0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C, 0x62DD1DDF, 0x15DA2D49,
		0x8CD37CF3, 0xFBD44C65, 0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
		0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB, 0x4369E96A, 0x346ED9FC,
		0xAD678846, 0xDA60B8D0, 0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
		0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086, 0x5768B525, 0x206F85B3,
		0xB966D409, 0xCE61E49F, 0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
		0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD, 0xEDB88320, 0x9ABFB3B6,
		0x03B6E20C, 0x74B1D29A, 0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
		0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8, 0xE40ECF0B, 0x9309FF9D,
		0x0A00AE27, 0x7D079EB1, 0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
		0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7, 0xFED41B76, 0x89D32BE0,
		0x10DA7A5A, 0x67DD4ACC, 0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
		0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252, 0xD1BB67F1, 0xA6BC5767,
		0x3FB506DD, 0x48B2364B, 0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
		0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79, 0xCB61B38C, 0xBC66831A,
		0x256FD2A0, 0x5268E236, 0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
		0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04, 0xC2D7FFA7, 0xB5D0CF31,
		0x2CD99E8B, 0x5BDEAE1D, 0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
		0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713, 0x95BF4A82, 0xE2B87A14,
		0x7BB12BAE, 0x0CB61B38, 0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
		0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E, 0x81BE16CD, 0xF6B9265B,
		0x6FB077E1, 0x18B74777, 0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
		0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45, 0xA00AE278, 0xD70DD2EE,
		0x4E048354, 0x3903B3C2, 0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
		0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0, 0xA9BCAE53, 0xDEBB9EC5,
		0x47B2CF7F, 0x30B5FFE9, 0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
		0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF, 0xB3667A2E, 0xC4614AB8,
		0x5D681B02, 0x2A6F2B94, 0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D };
uint32_t calcCrc32(uint8_t* data, uint32_t len) {
	uint32_t crc;
	int i = 0;
	crc = 0xFFFFFFFF;
	for (i = 0; i < len; i++)

		crc = ((crc >> 8) & 0x00FFFFFF) ^ crcTable[data[i] & 0xFF];

	return (crc);

}
/**
 * Frame:
 * 0xAA | 2B - lenght | 1B - arg | 0 - 65536B | CRC/XOR | 0x55
 */
void STP_send(uint8_t arg, char* data) {

	int i = 0;
	uint8_t start = 170; //0xAA
	uint16_t length = strLen(data);
	uint32_t crc = calcCrc32(data, length);
	uint8_t end = 85; //0x55

	int tmpOut = (((arg << 8) + length) << 16) + start;

	USART_putByte(USART1, start & (uint16_t) 0x01FF); //0xAA
	USART_putByte(USART1, (length & 0xff) & (uint16_t) 0x01FF); //length
	USART_putByte(USART1, (length >> 8) & (uint16_t) 0x01FF); //length
	USART_putByte(USART1, arg & (uint16_t) 0x01FF); //arg
	USART_puts(USART1, data); //data
	USART_puts(USART1, &crc);

	USART_putByte(USART1, end & (uint16_t) 0x01FF); //0x55
}

stp_data STPRead() {
	if (strLen(data) > 0 && READY_FLAG) {
		stp_data d = { .f_data = data, .arg = CURRENT_ARG };
		return d;
	} else {
		stp_data d = { .f_data = "", .arg = 0 };
		return d;
	}
}

int STPStatus() {
	return READY_FLAG;
}

int main(void) {

	SystemInit();
	init_GPIO();
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_CRC, ENABLE);
	CRC_ResetDR();
	//CRC_SetIDRegister(69);
	init_USART1(9600); // initialize USART1 @ 9600 baud
	mryg();
	STP_send(3, "dupa"); // just send a message to indicate that it works

	while (1) {
		if (/*STPStatus()*/READY_FLAG) {
			stp_data r = STPRead();
			STP_send(r.arg, r.f_data);
		}
	}
}
/*
 char* toHEX(char* buff, int len, unsigned int x) {
 buff += len;
 *buff = 0;
 char tmp;
 while (x) {
 buff--;
 tmp = x % 16;
 x /= 16;
 if (tmp >= 10)
 tmp += 'A' - 10;
 else
 tmp += '0';
 *buff = tmp;
 }
 return buff;
 }

 u32 dupa(u32 data) {
 int i;
 u32 rev = 0;
 for (i = 0; i < 32; i++)
 rev |= ((data >> i) & 1) << (31 - i);
 return rev;
 }
 ;*/

int strLen(char* str) {
	int len = 0;
	int i = 0;
	while (str[i]) {
		len++;
		i++;
	}
	return len;
}

void AnalyzeData(char* str) {
	CURRENT_LENGTH = (str[2] << 8) + str[1];
}

void actionLed(char* str) {
	if (str[3] == '1')
		GPIO_SetBits(GPIOD, GPIO_Pin_13);
	else
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);

	if (str[4] == '1')
		GPIO_SetBits(GPIOD, GPIO_Pin_14);
	else
		GPIO_ResetBits(GPIOD, GPIO_Pin_14);

	if (str[5] == '1')
		GPIO_SetBits(GPIOD, GPIO_Pin_15);
	else
		GPIO_ResetBits(GPIOD, GPIO_Pin_15);
}

void actionEcho(char* str) {
	char* tmp = &str[3];
	USART_puts(USART1, &tmp[1]);
//	USART_puts(USART1, &str[4]);
//	USART_puts(USART1, &str[5]);
	/*	int i;
	 char* tmp;
	 for (i = 3; i <= 5; i++)
	 tmp[i - 3] = str[i];
	 tmp[3] = 0;
	 USART_puts(USART1, tmp);*/
}

// this is the interrupt request handler (IRQ) for ALL USART1 interrupts
void USART1_IRQHandler(void) {
	static uint8_t reading_flag = 0;
	static uint8_t crc_flag = 0;
	static uint32_t crc_rec = 0;
	// check if the USART1 receive interrupt flag was set
	if (USART_GetITStatus(USART1, USART_IT_RXNE)) {

		static int cnt = 0; // this counter is used to determine the string length
		int t = USART1->DR; // the character from the USART1 data register is saved in t
		//STP_send(1, t);
		//AnalyzeData(t);

		GPIO_SetBits(GPIOD, GPIO_Pin_12);
		if (reading_flag == 0) {

				if ((cnt < 4)) {
					if (READY_FLAG)
						READY_FLAG = 0;
					received_string[cnt] = t;
					cnt++;
				}
				if (cnt == 4 && received_string[0] == 170) {
					//AnalyzeData(received_string);
					CURRENT_LENGTH = ((received_string[2] << 8)
							+ received_string[1]) + 1;

					received_string[cnt] = 0;
					cnt = 0;
					GPIO_ResetBits(GPIOD, GPIO_Pin_12);
					reading_flag = 1;
				}

		} else {
			if (crc_flag == 0) {
				if (cnt < CURRENT_LENGTH) {
					data[cnt] = t;
					cnt++;
				} else {
					if (data)
						;
					char asd = t;
					cnt = 0;
					GPIO_ResetBits(GPIOD, GPIO_Pin_12);
					crc_flag = 2;
				}
			} if(crc_flag == 2) {
				if ((cnt < 4)) {

					received_string[cnt] = t;
					cnt++;
				}
				if (cnt == 4) {

					crc_rec = (received_string[3] << 24) + (received_string[2] << 16) + (received_string[1] << 8) + received_string[0];
					uint32_t local_crc = calcCrc32(data, CURRENT_LENGTH-1);
					if(crc_rec != local_crc) STP_send(0,"blad");
					received_string[cnt] = 0;
					cnt = 0;
					GPIO_ResetBits(GPIOD, GPIO_Pin_12);
					reading_flag = 0;
					READY_FLAG = 1;
					crc_flag = 0;
				}
			}

		}
	}

}
void mryg() {
	int i;
	GPIO_SetBits(GPIOD, GPIO_Pin_12);
	for (i = 0; i < 1000000; i++)
		;
	GPIO_ResetBits(GPIOD, GPIO_Pin_12);

	GPIO_SetBits(GPIOD, GPIO_Pin_13);
	for (i = 0; i < 1000000; i++)
		;
	GPIO_ResetBits(GPIOD, GPIO_Pin_13);

	GPIO_SetBits(GPIOD, GPIO_Pin_14);
	for (i = 0; i < 1000000; i++)
		;
	GPIO_ResetBits(GPIOD, GPIO_Pin_14);

	GPIO_SetBits(GPIOD, GPIO_Pin_15);
	for (i = 0; i < 1000000; i++)
		;
	GPIO_ResetBits(GPIOD, GPIO_Pin_15);
	for (i = 0; i < 1000000; i++)
		;
}
