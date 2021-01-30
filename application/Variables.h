/*
 * Variables.h
 *
 *  Author: oskar
 */ 


#ifndef VARIABLES_H_
#define VARIABLES_H_

// message names-------------------- (IDM = identification of message)
#define IDM_SET_TIME 1
#define IDM_MINMAX 2
#define IDM_AVERAGE	3
#define IDM_TIME_DATE 4
#define IDM_MAIN_SCREEN 5
#define IDM_SET_DATE 6

// viestien rakenne
typedef struct
{
	char idMessage; // viestin tunniste
	char data;      // dataa
}DISPLAY_MESSAGE;

typedef enum
{
	eJanuary,
	eFebruary,
	eMarch,
	eApril,
	eMay,
	eJune,
	eJuly,
	eAugust,
	eSeptember,
	eOctober,
	eNovember,
	eDecember
} eMonths;

// -- taskien esittelyt ---------------
static void vLcdHandler( void *pvParameters );
static void vKeyPadHandler( void *pvParameters );
static void vDoMeasurements( void *pvParameters );
static void vClock( void *pvParameters );

// -- introduction of functions ----
void ShowTime(void);
void ShowDate(void);
void ShowBoth(void);

void SetTime(void);
void SetDate(void);

int DaysInMonth(int month, int year);
void StartTimer( int ticks);
int  ReadKeyPadWithLCD(char *szPrompt, int nMax);

/* Baud rate used by the serial port tasks. */
#define mainCOM_BAUD_RATE			(9600)
#define comBUFFER_LEN				(50)

//-- taskien yhteinen data -------------------------------
unsigned  secondsFromMidNight = 0; // ajan ylläpitäjä
unsigned screenTimer = 5;
unsigned lastScreen = 0;
bool stopScreenTimer = false;
#define waitTime 3
#define INTS_MAX 7
#define INTS_MAX_TIME 6

// yhteisille tiedoille käytettävät nimet  (IDD = identification of data)
#define IDD_TEMP 1
#define IDD_AVE 2
#define IDD_MINI 3
#define IDD_MAX 4
//#define IDD_LASTKEY 5
int ints[INTS_MAX] = {0};
	
//AIKA----------------------------------------------------

#define IDD_HOUR 0
#define IDD_MIN 1
#define IDD_SEC 2

#define IDD_DAY 3
#define IDD_MONTH 4
#define IDD_YEAR 5
int intsTime[INTS_MAX_TIME] = {0};// tietokanta = yhteiset kokonaislukutiedot
	
//--postilaatikot-----------------------------------------
static QueueHandle_t xDisplay; // näytön postilaatikko
//--semaforit---------------------------------------------
static SemaphoreHandle_t xClock; //aika
static SemaphoreHandle_t xPortB; // portin B semafori
static SemaphoreHandle_t xPortC; // portin C semafori
//--------------------------------------------------------
xComPortHandle xSerialPort;

#endif /* VARIABLES_H_ */