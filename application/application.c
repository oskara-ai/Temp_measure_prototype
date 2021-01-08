
#define F_CPU 16000000L

// Scheduler include files. --------------------------------------------
#include "FreeRTOS.h"
#include "task.h"
#include "croutine.h"
#include "semphr.h"
#include "serial.h"
#include "queue.h"

// avr tarpeen ajastinpiirin takia
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

// c kielen funktiot
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
// paikalliset laitteet
#include "../ds1820/sensor.h"   // lämpömittauksen funktiot
#include "..\devices\device.h"  // lcd-paneeli

// viestien nimet-------------------- (IDM = identification of message)
#define IDM_LEDS 1
#define IDM_UPDATE_DISPLAY 2
#define IDM_DOWN 3
#define IDM_RL	4
// viestien rakenne
typedef struct
{
	char idMessage; // viestin tunniste
	char data;      // dataa
}DISPLAY_MESSAGE;

typedef struct RECORD {
	int value;
	char hours;
	char minutes;
	char seconds;
	struct RECORD *pNext;
}RECORD;

//-- taskien yhteinen data -------------------------------
unsigned  secondsFromMidNight = 0; // ajan ylläpitäjä

// -- taskien esittelyt ---------------
static void vLcdHandler( void *pvParameters );
static void vKeyPadHandler( void *pvParameters );
static void vDoMeasurements( void *pvParameters );
static void vClock( void *pvParameters );
void ShowTime(void);
void StartTimer( int ticks);
void StartTimer2( int ticks);
void DS18X20_eeprom_to_scratchpad();
void DS18X20_scratchpad_to_eeprom();


/* Baud rate used by the serial port tasks. */
#define mainCOM_BAUD_RATE			(9600)
#define comBUFFER_LEN				(50)

//-- taskien yhteinen data -------------------------------

#define INTS_MAX 10
#define INTS_MAX_TIME 6
// yhteisille tiedoille käytettävät nimet  (IDD = identification of data)
#define IDD_TEMP 5
#define IDD_AVE 6
#define IDD_MIN 7
#define IDD_MAX 8
#define IDD_LASTKEY 9
int ints[INTS_MAX] = {0};

//AIKA----------------------------------------------------

#define IDD_SEC 1
#define IDD_MINU 2
#define IDD_HOUR 3

#define IDD_DAY 4
#define IDD_MONTH 5
#define IDD_YEAR 6
int tietoKanta[INTS_MAX_TIME] = {0};// tietokanta = yhteiset kokonaislukutiedot

//--------------------------------------------------------
//--postilaatikot
static QueueHandle_t xDisplay; // näytön postilaatikko
//--semaforit
static SemaphoreHandle_t xADC; // AD-muuntimen varaussemafori
static SemaphoreHandle_t xClock; //aika

xComPortHandle xSerialPort;
//--------------------------------------------------------
// pääohjelma luo taskit ja käynnistää systeemin
int main( void )
{
	xDisplay = xQueueCreate( 2, sizeof(DISPLAY_MESSAGE));// luodaan postilaatikko näyttötaskille

	vSemaphoreCreateBinary( xADC );// luodaan semafori
	vSemaphoreCreateBinary(xClock);

	// alustetaan sarjaportti
	xSerialPort = xSerialPortInitMinimal(0, mainCOM_BAUD_RATE, comBUFFER_LEN ,10);
	lcd_init(LCD_DISP_ON);  // lcd-kuntoon

	// luodaan taskit
	xTaskCreate( vLcdHandler, 0, configMINIMAL_STACK_SIZE, 0,  (tskIDLE_PRIORITY + 2), NULL );
	xTaskCreate( vDoMeasurements, 0, configMINIMAL_STACK_SIZE, 0,  (tskIDLE_PRIORITY + 2), NULL );
	xTaskCreate( vKeyPadHandler, 0, configMINIMAL_STACK_SIZE, 0,  (tskIDLE_PRIORITY + 2), NULL );
	xTaskCreate( vClock, 0, configMINIMAL_STACK_SIZE, 0,  (tskIDLE_PRIORITY + 2), NULL );

	vTaskStartScheduler();  // ajastus päälle


	return 0;
}
static void vClock( void *pvParameters )
{
	( void ) pvParameters; // Just to stop compiler warnings.

	vSemaphoreCreateBinary( xClock );// luodaan semafori
	char  szVariable[8];
	
	StartTimer(125); // = 8 msekunnin välein keskeytys 8*125 = 1000ms = 1s
	StartTimer2(125);
	
	
	// taskilla tulee aina  olla ikisilmukka
	for( ;; )//.........
	{
		xSemaphoreTake( xClock, portMAX_DELAY ); // odotetaan tietoa keskeytyksestä

		taskENTER_CRITICAL(); /////////////////////////////////////////
		tietoKanta[ IDD_HOUR ]   =  secondsFromMidNight / 3600L;         ////
		tietoKanta[ IDD_MINU ]= (secondsFromMidNight % 3600L) / 60L ; ////
		tietoKanta[ IDD_SEC ]=  secondsFromMidNight % 60L;           ////
		taskEXIT_CRITICAL(); //////////////////////////////////////////
		// näytetään aika
		//ShowTime();
	}
}



void ShowTime(void)
{
	char  szVariable[8];  // muuttujan arvo tulostetaan tähän

	lcd_gotoxy(0,0);
	
	// tunnit
	if( tietoKanta[IDD_HOUR] < 10)
	lcd_putc('0');
	
	itoa(tietoKanta[IDD_HOUR],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(':');
	
	// minuutit
	if( tietoKanta[IDD_MINU] < 10)
	lcd_putc('0');
	
	itoa(tietoKanta[IDD_MINU],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(':');
	
	// sekunnit
	if( tietoKanta[IDD_SEC] < 10)
	lcd_putc('0');
	
	itoa(tietoKanta[IDD_SEC],szVariable,10);
	lcd_puts(szVariable);
	
	
	
}

static void vDoMeasurements( void *pvParameters )
{
	( void ) pvParameters; // Just to stop compiler warnings.
	float value ;
	float maxValue = 0;
	float minValue = 999;
	float avegValue;
	int address = 0;

	static unsigned char  numberOfSensors;
	
	numberOfSensors = GetSensorCount(); // samassa johtimessa lämpöantureiden määrä


	for( ;; )
	{

		if (numberOfSensors)
		{
			value  = (float)(GetTemperature(0)/1000);

			if (minValue >= value)
			{
				minValue = value;
			}
			if (maxValue < value)
			{
				maxValue= value;
			}

			avegValue = (minValue+maxValue)/2;
			
			taskENTER_CRITICAL(); //////////////////////////
			ints[ IDD_TEMP ] = value ;
			ints[IDD_MIN] = minValue;
			ints[IDD_MAX] = maxValue;
			ints[IDD_AVE] = avegValue;					 ////
			taskEXIT_CRITICAL(); ///////////////////////////
		}
		
		vTaskDelay(1000);
	}
	
}
//--------------------- lcd-näyttöä päivittävä taski -------------------------------
static void vLcdHandler( void *pvParameters )
{

	static char *pDisplay[] = {"%t	\ntemp:%i05C"};//no key
	static char *pDisplay2[] = {"MAX:%i08   \nMIN:%i07"};//up or down
	static char *pDisplay3[] = {"cl..cl%i03 Ave:%i06   \nMin-Max=%i07-%i08"};//right or left
	
	volatile  char *pChDisplay =0,  // osoitin, jota osoittaa nytn pohjatekstiss olevaan kirjaimeen
	*pChVariable=0; // osoitin, jota kytetään muuttujan arvon tulostukseen
	int             i;
	char            szVariable[8]; // muuttujan arvo tulostetaan tähän
	static DISPLAY_MESSAGE message;
	bool skip = false;

	( void ) pvParameters;	/* Just to stop compiler warnings. */
	
	
	// taskilla tulee aina  olla ikisilmukka
	for( ;; )//.................
	{
		xSemaphoreTake(xDisplay, portMAX_DELAY);
		xQueueReceive(xDisplay,&message, portMAX_DELAY); // odotetaan viestiä
		

		switch( message.idMessage)
		{
			
			case IDM_UPDATE_DISPLAY: case NO_KEY:
			if( message.data == 0 )
			pChDisplay = pDisplay[0];
			lcd_clrscr();
			
			lcd_gotoxy(0, 0);	// lcd-näytön alkuun
			// tulostetaan näyttö tässä silmukassa
			while(*pChDisplay != 0)
			{
				// onko muuttujan tulostuspaikka?
				if( *pChDisplay == '%')
				{
					pChDisplay++; //ohitetaan %-merkki
					// minkä tyypin dataa?
					switch(*pChDisplay)
					{
						// int-tyypin kokonaislukuja
						// muuttujan arvo poimitaan ints-taulukosta indeksin määräämästä paikasta
						case 'i': pChDisplay++;
						// indeksi annettu muodossa 09, 10,11,..
						i = (*pChDisplay - '0')*10;
						pChDisplay++;
						i +=  (*pChDisplay- '0'); // indeksi ints-taulukkoon
						taskENTER_CRITICAL();
						itoa(ints[i],szVariable,10);
						taskEXIT_CRITICAL();
						skip = false;
						
						break;
						
						case 't':
						
						taskENTER_CRITICAL();
						ShowTime();
						taskEXIT_CRITICAL();
						skip = true;
						break;
						
						
					}
					// tulostetaan muuttuja
					if(!skip)	{
						pChVariable = szVariable;
						while(*pChVariable != 0)
						{
							lcd_putc(*pChVariable); // merkki näkyviin
							while(xSerialPutChar(&xSerialPort, *pChVariable)==pdFAIL);//odotetaan kunnes jonoon mahtuu
							pChVariable++; // seuraava kirjain
						}
					}
				}
				else if(*pChDisplay == '\n')
				{
					lcd_gotoxy(0, 1);
				}
				
				else
				lcd_putc(*pChDisplay); // merkki näkyviin
				pChDisplay++; // seuraava kirjain
				
			}
			
			break;
			
			case IDM_DOWN:
			
			if( message.data == 0 )
			pChDisplay = pDisplay2[0];
			lcd_gotoxy(0, 0);
			lcd_clrscr();
			
			while(*pChDisplay != 0)
			{
				// onko muuttujan tulostuspaikka?
				if( *pChDisplay == '%')
				{
					pChDisplay++; //ohitetaan %-merkki
					// minkä tyypin dataa?
					switch(*pChDisplay)
					{
						// int-tyypin kokonaislukuja
						// muuttujan arvo poimitaan ints-taulukosta indeksin määräämästä paikasta
						case 'i': pChDisplay++;
						// indeksi annettu muodossa 09, 10,11,..
						i = (*pChDisplay - '0')*10;
						pChDisplay++;
						i +=  (*pChDisplay - '0'); // indeksi ints-taulukkoon
						taskENTER_CRITICAL();
						itoa(ints[i],szVariable,10);
						taskEXIT_CRITICAL();
						skip = false;
						break;
					}
					
					if(!skip)	{
						// tulostetaan muuttuja
						pChVariable = szVariable;
						while(*pChVariable != 0)
						{
							lcd_putc(*pChVariable); // merkki näkyviin
							
							pChVariable++; // seuraava kirjain
						}
					}
				}
				else if(*pChDisplay == '\n')
				{
					lcd_gotoxy(0, 1);
				}
				else
				// näytön pohjateksti
				lcd_putc(*pChDisplay); // merkki näkyviin
				pChDisplay++; // seuraava kirjain
			}
			_delay_ms(1000);
			
			break;
			
			case IDM_RL:
			
			if( message.data == 0 )
			pChDisplay = pDisplay3[0];
			lcd_gotoxy(0, 0);
			lcd_clrscr();
			
			
			while(*pChDisplay != 0)
			{
				// onko muuttujan tulostuspaikka?
				if( *pChDisplay == '%')
				{
					pChDisplay++; //ohitetaan %-merkki
					// minkä tyypin dataa?
					switch(*pChDisplay)
					{
						case 'i': pChDisplay++;
						// indeksi annettu muodossa 09, 10,11,..
						i = (*pChDisplay - '0')*10;
						pChDisplay++;
						i +=  (*pChDisplay - '0'); // indeksi ints-taulukkoon
						taskENTER_CRITICAL();
						itoa(ints[i],szVariable,10);
						taskEXIT_CRITICAL();
						skip = false;
						break;
					}
					
					if(!skip)
					{
						// tulostetaan muuttuja
						pChVariable = szVariable;
						while(*pChVariable != 0)
						{
							lcd_putc(*pChVariable); // merkki näkyviin
							pChVariable++; // seuraava kirjain
						}
					}
				}
				else if(*pChDisplay == '\n')
				{
					lcd_gotoxy(0, 1);
				}
				else
				// näytön pohjateksti
				lcd_putc(*pChDisplay); // merkki näkyviin

				pChDisplay++; // seuraava kirjain
			}
			break;
			
			default:
			break;
		}
		xSemaphoreGive(xDisplay);
	}
	
}


//--------------------- painikkeita lukeva taski -------------------------------
static void vKeyPadHandler( void *pvParameters )
{
	static unsigned char ch = 0;
	static DISPLAY_MESSAGE message;
	
	( void ) pvParameters; /* Just to stop compiler warnings. */

	// taskilla tulee aina  olla ikisilmukka
	for( ;; )//=================
	{
		
		do  //tämä silmukka kuormittaa!!!
		{
			xSemaphoreTake( xADC, portMAX_DELAY );
			ch =GetKey();vTaskDelay(1);
			xSemaphoreGive( xADC );

		}while (ch == NO_KEY);

		switch( ch )
		{
			case IDK_SELECT: case NO_KEY:  // näppäinten käyttö
			
			taskENTER_CRITICAL(); //////////////////////////////////
			ints[IDD_LASTKEY] = ch;                             ////
			taskEXIT_CRITICAL();  //////////////////////////////////
			// lähetään viesti lcd-näyttötaskille,jotta se päivittää näytön
			message.data      = 0; // näytön numero
			message.idMessage = IDM_UPDATE_DISPLAY;
			xQueueSend( xDisplay, (void*)&message,0);
			
			break;
			
			case IDK_DOWN: case IDK_UP:
			
			taskENTER_CRITICAL(); //////////////////////////////////
			ints[IDD_LASTKEY] = ch;                             ////
			taskEXIT_CRITICAL();  //////////////////////////////////
			// lähetään viesti lcd-näyttötaskille,jotta se päivittää näytön
			message.data      = 0; // näytön numero
			message.idMessage = IDM_DOWN;
			xQueueSend( xDisplay, (void*)&message,0);
			
			break;
			
			case IDK_RIGHT: case IDK_LEFT:
			
			taskENTER_CRITICAL(); //////////////////////////////////
			ints[IDD_LASTKEY] = ch;                             ////
			taskEXIT_CRITICAL();  //////////////////////////////////
			// lähetään viesti lcd-näyttötaskille,jotta se päivittää näytön
			message.data      = 0; // näytön numero
			message.idMessage = IDM_RL;
			xQueueSend( xDisplay, (void*)&message,0);

			break;
			
			default:; // muut näppäimet
		}
	} // =========================
}

void StartTimer( int ticks)
{
	// ks. manuaalia doc2549.pdf sivulta 118 alkaen
	// 0 = stop  1 = clock  2 = clock/8 3 = clock/64  4 = clock/256 5 = clock/1024
	TCCR0B = (1<<FOC0A) | ( 1<<CS02) | (1<<CS00); // prosessorin kellotaajuus/1024   , jos 16MHz => 64 us
	TIMSK0 |= (1 << OCIE0A); // vertailuarvokeskeytys
	OCR0A = ticks; // laitetaan vertailuarvoksi annettu lukema
	TCNT0 = 0;    // laskuri alkuun
}

void StartTimer2( int ticks)
{
	// ks. manuaalia doc2549.pdf sivulta 118 alkaen
	// 0 = stop  1 = clock  2 = clock/8 3 = clock/64  4 = clock/256 5 = clock/1024
	TCCR2B = (1<<FOC2A) | ( 1<<CS22) | (1<<CS20); // prosessorin kellotaajuus/1024   , jos 16MHz => 64 us
	TIMSK2 |= (1 << OCIE2A); // vertailuarvokeskeytys
	OCR2A = ticks; // laitetaan vertailuarvoksi annettu lukema
	TCNT2 = 0;    // laskuri alkuun
}

SIGNAL(TIMER0_COMPA_vect) {
	static BaseType_t xTaskWoken = pdFALSE;
	static unsigned msCounter = 0;
	msCounter += 8 ; // interval 8ms
	TCNT0 = 0; // laskuri alkuun
	//timer0 interrupt handler
	if ( msCounter == 1000)
	{
		secondsFromMidNight++;
		xSemaphoreGiveFromISR( xClock, &xTaskWoken ); // ilmoitetaan kellonajan muutos
		msCounter = 0;
	}
}
////////////////////////////////////////////////////////////////
SIGNAL(TIMER2_COMPA_vect) {
	static BaseType_t xTaskWoken = pdFALSE;
	static unsigned msCounter = 0;
	msCounter += 8 ; // interval 8ms
	TCNT2 = 0; // laskuri alkuun
	//timer0 interrupt handler
	if ( msCounter == 1000)
	{
		//		secondsFromMidNight++;
		//		xSemaphoreGiveFromISR( xClock, &xTaskWoken ); // ilmoitetaan kellonajan muutos
		//		msCounter = 0;
	}
}