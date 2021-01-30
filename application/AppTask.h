/*
 * AppTask.h
 *
 *  Author: oskar
 */ 


#ifndef APPTASK_H_
#define APPTASK_H_

static void vClock( void *pvParameters )
{
	( void ) pvParameters; // Just to stop compiler warnings.
	
	vSemaphoreCreateBinary( xClock );// luodaan semafori
	static DISPLAY_MESSAGE message;
	
	StartTimer(125); // = 8 msekunnin v�lein keskeytys 8*125 = 1000ms = 1s
	
	// esiasetettu p�iv�
	intsTime[IDD_DAY] = 25;
	intsTime[IDD_MONTH] = 1;
	intsTime[IDD_YEAR] = 2021;
	
	message.data = 0;
	message.idMessage = IDM_MAIN_SCREEN;
	
	for( ;; )
	{
		xSemaphoreTake( xClock, portMAX_DELAY ); // odotetaan tietoa keskeytyksest�

		taskENTER_CRITICAL(); /////////////////////////////////////////
		intsTime[ IDD_HOUR ]   =  secondsFromMidNight / 3600L;         ////
		intsTime[ IDD_MIN ] = (secondsFromMidNight % 3600L) / 60L ; ////
		intsTime[ IDD_SEC ] =  secondsFromMidNight % 60L;           ////
		taskEXIT_CRITICAL(); //////////////////////////////////////////
		
		//jos p�iv� on vaihtunut
		if(secondsFromMidNight > (24*3600L))
		{
			secondsFromMidNight = 0;
			intsTime[IDD_DAY]++;
		}
		
		// jos kuukausi on vaihtunut
		if(intsTime[IDD_DAY] > DaysInMonth(intsTime[IDD_MONTH], intsTime[IDD_MONTH]))
		{
			intsTime[IDD_DAY] = 1;
			intsTime[IDD_MONTH]++;
		}
		
		// jos vuosi on vaihtunut
		if(intsTime[IDD_MONTH] > 12)
		{
			intsTime[IDD_MONTH] = 1;
			intsTime[IDD_YEAR]++;
		}
		
		// kolmen sekunnin palautus
		if(screenTimer >= waitTime)
		xQueueSend(xDisplay, (void*)&message, 0);
		
		// jos ajastin ei ole pys�htynyt lis�� ajastin
		if(!stopScreenTimer)
		screenTimer++;
		
	}
}
//--------------------- lcd-n�ytt�� p�ivitt�v� taski -------------------------------
static void vLcdHandler( void *pvParameters )
{
	
	// perus pohjat
	static char *pDisplay[] = {"%b\n%i01 C"}; // default
	static char *pDisplay2[] = {"Max=%i04\nMin=%i03"};//yl�s, alas
	static char *pDisplay3[] = {"Ave=%i02\n%i03 C...%i04 C"}; // vasen, oikea
	static char *pDisplay4[] = {"Up=Set Time\nDown=Set Date"}; //select
	static char *pDisplay5[] = {"Set Time\n%t"};//select yl�s
	static char *pDisplay6[] = {"Set Date\n%d"};//select alas
	
	volatile  char *pChDisplay =0,  // osoitin, jota osoittaa nytn pohjatekstiss olevaan kirjaimeen
	*pChVariable=0; // osoitin, jota kytet��n muuttujan arvon tulostukseen
	int             i;
	char            szVariable[8]; // muuttujan arvo tulostetaan t�h�n
	static DISPLAY_MESSAGE message;
	bool skip = false; // v�ltet��n tulostamasta ylim��r�isi�

	( void ) pvParameters;	/* Just to stop compiler warnings. */
	
	for( ;; )
	{
		xQueueReceive(xDisplay,&message, portMAX_DELAY); // odotetaan viesti�
		
		// jos edellinen ruutu on eri, tyhjenn� n�ytt� ja vaihda nykyiseen
		if(lastScreen != message.idMessage)
		{
			lcd_clrscr();
			lastScreen = message.idMessage;
		}
		//vaihdellaan n�ytt�jen v�lill�
		switch( message.idMessage)
		{
			
			case IDM_MAIN_SCREEN:
			pChDisplay = pDisplay[0];
			break;
			
			case IDM_MINMAX:
			pChDisplay = pDisplay2[0];
			break;
			
			case IDM_AVERAGE:
			pChDisplay = pDisplay3[0];
			break;
			
			case IDM_TIME_DATE:
			pChDisplay = pDisplay4[0];
			break;
			
			case IDM_SET_TIME:
			pChDisplay = pDisplay5[0];
			break;
			
			case IDM_SET_DATE:
			pChDisplay = pDisplay6[0];
			break;
			
			default:
			break;
		}
		
		xSemaphoreTake(xPortB, portMAX_DELAY);
		
		lcd_gotoxy(0, 0);	// lcd-n�yt�n alkuun
		
		// tulostetaan n�ytt� t�ss� silmukassa
		while(*pChDisplay != 0)
		{
			// onko muuttujan tulostuspaikka?
			if( *pChDisplay == '%')
			{
				pChDisplay++; //ohitetaan %-merkki
				
				// mink� tyypin dataa?
				switch(*pChDisplay)
				{
					case 't':
					taskENTER_CRITICAL();
					ShowTime();
					taskEXIT_CRITICAL();
					skip = true;
					break;
					
					case 'd':
					taskENTER_CRITICAL();
					ShowDate();
					taskEXIT_CRITICAL();
					skip = true;
					break;
					
					case 'b':
					taskENTER_CRITICAL();
					ShowBoth();
					taskEXIT_CRITICAL();
					skip = true;
					break;
					
					case 'i': pChDisplay++;// int-tyypin kokonaislukuja
					// muuttujan arvo poimitaan ints-taulukosta indeksin m��r��m�st� paikasta
					// indeksi annettu muodossa 09, 10,11,..
					i = (*pChDisplay - '0')*10;
					pChDisplay++;
					i +=  (*pChDisplay- '0'); // indeksi ints-taulukkoon
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
						
						pChVariable++; // seuraava kirjain
						if(*pChVariable == 0)
						lcd_putc('.');
						
						pChVariable--;
						
						lcd_putc(*pChVariable); // tulosta muuttuja
						while(xSerialPutChar(&xSerialPort, *pChVariable)==pdFAIL);//l�hetet��n merkit sarjaliikenneportin kautta
						pChVariable++;	//seuraava merkki
					}
				}
			}
			else if( *pChDisplay == '\n')
			{
				lcd_gotoxy(0,1); // 2nd line
			}
			else
			// n�yt�n pohjateksti
			lcd_putc(*pChDisplay); // merkki n�kyviin
			pChDisplay++; // seuraava kirjain
		}
		xSemaphoreGive(xPortB);
	}
}
//--------------------------------------------------------

//--------------------- painikkeita lukeva taski -------------------------------
static void vKeyPadHandler( void *pvParameters )
{
	static unsigned char ch = 0;
	const TickType_t xDelay = 500 / portTICK_PERIOD_MS;	// 0.5sec wait
	static DISPLAY_MESSAGE message;
	
	message.data = 0;
	
	( void ) pvParameters; /* Just to stop compiler warnings. */


	for( ;; )
	{
		// odota
		vTaskDelay(xDelay);
		
		do
		{
			xSemaphoreTake( xPortC, portMAX_DELAY);
			ch =GetKey();vTaskDelay(1);
			xSemaphoreGive( xPortC);
			
		}while (ch == NO_KEY);
		
		// nollaa n�ytt�aika
		screenTimer = 0;

		switch( ch )
		{
			case IDK_SELECT: // n�pp�inten k�ytt�
			message.idMessage = IDM_TIME_DATE; // viestin id
			xQueueSend( xDisplay, (void*)&message,0); // l�het� viesti n�yt�lle
			break;
			//--------------------------------------------------------
			case IDK_DOWN: case IDK_UP:
			//jos n�ytt� on "set time or date"
			if (lastScreen == IDM_TIME_DATE)
			{
				// jos valittu up aloita aseta aika
				if (ch == IDK_UP)
				{
					SetTime();
					vTaskDelay(20);
					message.idMessage = IDM_MAIN_SCREEN;
				}
				// jos painettu down aloita aseta p�iv�
				else if (ch == IDK_DOWN)
				{
					SetDate();
					vTaskDelay(20);
					message.idMessage = IDM_MAIN_SCREEN;
				}
			}
			else if (lastScreen != IDM_SET_TIME && lastScreen != IDM_TIME_DATE)
			message.idMessage = IDM_MINMAX;
			
			xQueueSend( xDisplay, (void*)&message,0); // l�het� viesti n�yt�lle
			break;
			//--------------------------------------------------------
			case IDK_RIGHT: case IDK_LEFT:
			
			if(lastScreen != IDM_SET_TIME && lastScreen != IDM_TIME_DATE)
			{
				message.idMessage = IDM_AVERAGE;	//keskiarvo
				xQueueSend( xDisplay, (void*)&message,0);
			}
			break;
			default:; // muut n�pp�imet
		}
	}
}


//--------------------- L�mp�tilan mittaus taski -----------------------------------
static void vDoMeasurements( void *pvParameters )
{
	( void ) pvParameters; // Just to stop compiler warnings.
	int value ;
	
	ints[IDD_MAX] = -600;
	ints[IDD_MINI] = 1300;
	
	static unsigned char  numberOfSensors;
	numberOfSensors = GetSensorCount(); // samassa johtimessa l�mp�antureiden m��r�

	for( ;; )
	{
		if (numberOfSensors)
		{
			value  = (float)(GetTemperature(0)/1000);
			//minimi
			if (ints[IDD_MINI] >= value)
			{
				ints[IDD_MINI] = value;
			}//maksimi
			if (ints[IDD_MAX] < value)
			{
				ints[IDD_MAX]= value;
			}
			//keskiarvo
			ints[IDD_AVE] = (ints[IDD_MAX] + ints[IDD_MINI])/2;
			
			taskENTER_CRITICAL(); //////////////////////////
			ints[ IDD_TEMP ] = value ;					////
			taskEXIT_CRITICAL(); ///////////////////////////
		}
		vTaskDelay(100);
	}
}





#endif /* APPTASK_H_ */