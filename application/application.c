
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

#include "Variables.h"
#include "Functions.h"			// funktiot
#include "AppTask.h"			// taskien funktiot

//--------------------------------------------------------
// pääohjelma luo taskit ja käynnistää systeemin
int main( void )
{
	xDisplay = xQueueCreate( 2, sizeof(DISPLAY_MESSAGE));// luodaan postilaatikko näyttötaskille

	// luodaan semafori 
	vSemaphoreCreateBinary(xPortC);
	vSemaphoreCreateBinary( xPortB );

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

