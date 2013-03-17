#include <stm32f4xx.h>
#include <misc.h>			 
#include <stm32f4xx_usart.h>
#include <stm32f4xx_gpio.h>

int main(void)
{
 
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOCEN;
	RCC->APB2ENR |= RCC_APB2ENR_USART6EN | RCC_APB2ENR_USART1EN; //włączenie zegara od usart
 
	GPIO_Port_Init (GPIOA, USART6_TX, GPIO_Mode_AF, GPIO_OType_PP, GPIO_Speed_25MHz, GPIO_PuPd_NOPULL);
	GPIO_Port_Init (GPIOC, USART1_RX, GPIO_Mode_AF, GPIO_OType_PP, GPIO_Speed_25MHz, GPIO_PuPd_NOPULL);
	//my diody inaczej robimy//GPIO_Port_Init (GPIOD, LED1_Green | LED2_Orange, GPIO_Mode_OUT, GPIO_OType_PP, GPIO_Speed_25MHz, GPIO_PuPd_NOPULL);
	GPIO_InitTypeDef GPIO_InitStructure = {
		.GPIO_Speed = GPIO_Speed_2MHz,
		.GPIO_PuPd = GPIO_PuPd_NOPULL,
		.GPIO_Mode = GPIO_Mode_OUT,
		.GPIO_OType = GPIO_OType_PP,
		.GPIO_Pin = GPIO_Pin_12 | GPIO_Pin_13 | GPIO_Pin_14 | GPIO_Pin_15
	};

	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_Port_AF_Init(GPIOA, USART6_TX, GPIO_AF_USART6);
	GPIO_Port_AF_Init(GPIOC, USART1_RX, GPIO_AF_USART1);
 
	USART6->CR1 = 0x0000;
	USART6->CR2 = 0x0000;
	USART1->CR1 = 0x0000;
	USART1->CR2 = 0x0000;
 
	USART6->CR1 |= USART_CR1_UE; //włączenie usart
	USART1->CR1 |= USART_CR1_UE; //włączenie usart
 
	USART6->BRR = 84000000/9600;
	USART1->BRR = 84000000/9600;
 
	USART6->CR1 |= USART_CR1_TE; //właczenie transmisji
	USART1->CR1 |= USART_CR1_RE; //właczenie odbioru 
	
	while (1)
	{
		GPIO_ToggleBits(GPIOD, GPIO_Pin_12);
		while( !(USART1->SR & USART_SR_RXNE) ); //oczekuje danych w rejestrze danych
		uint16_t a = USART1->DR;
		GPIO_ResetBits(GPIOD, GPIO_Pin_12);
		 		
		GPIO_ToggleBits(GPIOD, GPIO_Pin_13);
 		while( !(USART6->SR & USART_SR_TXE) ); //oczekuje zwolnienia rejestru danych
		USART6->DR = a;
		while( !(USART6->SR & USART_SR_TC) ); //oczekuje zakonczenia transmisji
		GPIO_ResetBits(GPIOD, GPIO_Pin_13);
	}
}

void usartSend()
{
	
}

uint16_t usartRecieve()
{

}
