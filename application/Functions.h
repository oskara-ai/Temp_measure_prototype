/*
 * Functions.h
 *
 *  Author: oskar
 */ 


#ifndef FUNCTIONS_H_
#define FUNCTIONS_H_

//--------------------- Lcd-näytön näppäimiatöltä luku -----------------------------------
int  ReadKeyPadWithLCD(char *szPrompt, int nMax)
{
	char text[10]={0};
	int nValue=0;
	int key;
	char *p = szPrompt;

	// näytetään kehote
	lcd_clrscr();
	while(*p )
	{
		if(*p == '\n')
		lcd_gotoxy(0,1);
		else
		lcd_putc(*p);
		p++;
	}
	do
	{
		while( (key = GetKey()) == NO_KEY); // odotetaan painiketta
		if (key == IDK_UP)
		{
			if(nValue < nMax)
			nValue++;
			//jos arvo on yli maksimin se nollautuu
			else if (nValue == nMax)
			nValue = 0;
		}
		
		else if(key == IDK_DOWN)
		{
			if(nValue)
			nValue--;
			// jos arvo on alle minimin se menee maksimiksi
			else if(!nValue)
			nValue = nMax;
		}
		else if(key == IDK_SELECT)
		{
			while( (key = GetKey()) != NO_KEY); // odotetaan painiketta vapaaksi
			return nValue;
		}
		// näytetään arvo
		itoa(nValue,text,10);
		//jos luku on alle 10 lisätään nolla
		if (nValue < 10)
		{
			lcd_gotoxy(10,1);
			lcd_putc('0');
			lcd_gotoxy(11,1);
		}
		else 
		lcd_gotoxy(10,1);
		lcd_puts(text);
		
		while( (key = GetKey()) != NO_KEY); // odotetaan painiketta vapaaksi
	}
	while(1);
	
}

//--------------------- Näytä aika-----------------------------------
void ShowTime(void)
{
	char  szVariable[8];  // tulosta arvo
	
	// Hours
	if( intsTime[IDD_HOUR] < 10)
	lcd_putc('0');
	
	itoa(intsTime[IDD_HOUR],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(':');
	
	// Minutes
	if( intsTime[IDD_MIN] < 10)
	lcd_putc('0');
	
	itoa(intsTime[IDD_MIN],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(':');
	
	// Seconds
	if( intsTime[IDD_SEC] < 10)
	lcd_putc('0');
	
	itoa(intsTime[IDD_SEC],szVariable,10);
	lcd_puts(szVariable);
}
//--------------------- Näytä päivä-----------------------------------
void ShowDate(void)
{
	char szVariable[8];		//tulosta arvo
	
	
	//Day
	itoa(intsTime[IDD_DAY],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc('.');
	
	//Month
	itoa(intsTime[IDD_MONTH],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc('.');
	
	//Year
	itoa(intsTime[IDD_YEAR],szVariable,10);
	lcd_puts(szVariable);
}

//------------näytä päivä ja aika ------------------
void ShowBoth(void)
{
	char  szVariable[10];  // print value here

	
	// Hours
	if( intsTime[IDD_HOUR] < 10)
	lcd_putc('0');
	
	itoa(intsTime[IDD_HOUR],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(':');
	
	// Minutes
	if( intsTime[IDD_MIN] < 10)
	lcd_putc('0');
	
	itoa(intsTime[IDD_MIN],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc(' ');
	
	//Day
	itoa(intsTime[IDD_DAY],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc('.');
	
	//Month
	itoa(intsTime[IDD_MONTH],szVariable,10);
	lcd_puts(szVariable);
	lcd_putc('.');
	
	//Year
	itoa(intsTime[IDD_YEAR],szVariable,10);
	lcd_puts(szVariable);
}
//----------This function lets you set time--------
void SetTime(void)
{
	unsigned int hour, min, sec;
	
	stopScreenTimer = true;
	xSemaphoreTake( xPortC, portMAX_DELAY ); // take semaphore
	
	hour = ReadKeyPadWithLCD("SET TIME\nHours? ",23);
	vTaskDelay(20);
	
	min = ReadKeyPadWithLCD("SET TIME\nMinutes? ",60);
	vTaskDelay(20);
	
	sec = ReadKeyPadWithLCD("SET TIME\nSeconds? ",60);
	vTaskDelay(20);
	
	// set time
	secondsFromMidNight = hour*3600L + min*60L + sec;
	intsTime[IDD_HOUR] = hour;
	intsTime[IDD_MIN] = min;
	intsTime[IDD_SEC] = sec;
	vTaskDelay(20); // wait 1 second
	
	stopScreenTimer = false;
	xSemaphoreGive( xPortC ); // give semaphore
}

void SetDate(void)
{
	unsigned int day, month, year;
	
	stopScreenTimer = true;
	xSemaphoreTake( xPortC, portMAX_DELAY ); // take semaphore
	
	year = ReadKeyPadWithLCD("SET DATE\nYear?   2000", 99);
	vTaskDelay(20);
	
	month = ReadKeyPadWithLCD("SET DATE\nMonth? ", 12);
	if(month == 0)
	month = 1;
	vTaskDelay(20);
	
	day = ReadKeyPadWithLCD("SET DATE\nDay? ", DaysInMonth(month, year));
	if(day == 0)
	day = 1;
	vTaskDelay(20);
	
	// set date
	intsTime[IDD_DAY] = day;
	intsTime[IDD_MONTH] = month;
	intsTime[IDD_YEAR] = year;
	vTaskDelay(20); // wait 1 second
	
	stopScreenTimer = false;
	xSemaphoreGive( xPortC ); // give semaphore
}

int DaysInMonth(int month, int year)
{
	int days = 0;
	int mNumber = month - 1;
	
	if(mNumber == eApril || mNumber == eJune || mNumber == eSeptember || mNumber == eNovember)
	days = 30;
	
	else if(mNumber == eFebruary)
	{
		if(!(year % 4))
		days = 29;
		
		else
		days = 28;
		
	}
	else
	days = 31;
	
	return days;
	
}

void StartTimer( int ticks)
{
	// 0 = stop  1 = clock  2 = clock/8 3 = clock/64  4 = clock/256 5 = clock/1024
	TCCR0B = (1<<FOC0A) | ( 1<<CS02) | (1<<CS00); // processor clock/1024   , if 16MHz => 64 us
	TIMSK0 |= (1 << OCIE0A); // reference interrupt
	OCR0A = ticks; // given number as a reference
	TCNT0 = 0;    // to start of the counter
}

SIGNAL(TIMER0_COMPA_vect) {
	static BaseType_t xTaskWoken = pdFALSE;
	static unsigned msCounter = 0;
	msCounter += 8 ; // interval 8ms
	TCNT0 = 0; // to start of the counter
	//timer0 interrupt handler
	if ( msCounter == 1000)
	{
		secondsFromMidNight++;
		xSemaphoreGiveFromISR( xClock, &xTaskWoken ); // inform the change in time
		msCounter = 0;
	}
}

#endif /* FUNCTIONS_H_ */