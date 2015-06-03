/* DateStrings.cpp
 * Definitions for date strings for use with the Time library
 *
 * No memory is consumed in the sketch if your code does not call any of the string methods
 * You can change the text of the strings, make sure the short strings are each exactly 3 characters 
 * the long strings can be any length up to the constant dt_MAX_STRING_LEN defined in Time.h
 * 
 */
 
#include <avr/pgmspace.h> 
#include "Time.h"
 
// the short strings for each day or month must be exactly dt_SHORT_STR_LEN
#define dt_SHORT_STR_LEN  3 // the length of short strings

static char buffer[dt_MAX_STRING_LEN+1];  // must be big enough for longest string and the terminating null

char monthStr1[]  = "January";
char monthStr2[]  = "February";
char monthStr3[]  = "March";
char monthStr4[]  = "April";
char monthStr5[]  = "May";
char monthStr6[]  = "June";
char monthStr7[]  = "July";
char monthStr8[]  = "August";
char monthStr9[]  = "September";
char monthStr10[]  = "October";
char monthStr11[]  = "November";
char monthStr12[]  = "December";

PGM_P monthNames_P[]  = 
{
    "",monthStr1,monthStr2,monthStr3,monthStr4,monthStr5,monthStr6,
	monthStr7,monthStr8,monthStr9,monthStr10,monthStr11,monthStr12
};

char monthShortNames_P[]  = "ErrJanFebMarAprMayJunJulAugSepOctNovDec";

char dayStr0[]  = "Err";
char dayStr1[]  = "Sunday";
char dayStr2[]  = "Monday";
char dayStr3[]  = "Tuesday";
char dayStr4[]  = "Wednesday";
char dayStr5[]  = "Thursday";
char dayStr6[]  = "Friday";
char dayStr7[]  = "Saturday";

PGM_P dayNames_P[]  = { dayStr0,dayStr1,dayStr2,dayStr3,dayStr4,dayStr5,dayStr6,dayStr7};
char dayShortNames_P[]  = "ErrSunMonTueWedThrFriSat";

/* functions to return date strings */

char* monthStr(uint8_t month)
{
    strcpy_P(buffer, (PGM_P)pgm_read_word(&(monthNames_P[month])));
	return buffer;
}

char* monthShortStr(uint8_t month)
{
   for (int i=0; i < dt_SHORT_STR_LEN; i++)      
      buffer[i] = pgm_read_byte(&(monthShortNames_P[i+ (month*dt_SHORT_STR_LEN)]));  
   buffer[dt_SHORT_STR_LEN] = 0;
   return buffer;
}

char* dayStr(uint8_t day) 
{
   strcpy_P(buffer, (PGM_P)pgm_read_word(&(dayNames_P[day])));
   return buffer;
}

char* dayShortStr(uint8_t day) 
{
   uint8_t index = day*dt_SHORT_STR_LEN;
   for (int i=0; i < dt_SHORT_STR_LEN; i++)      
      buffer[i] = pgm_read_byte(&(dayShortNames_P[index + i]));  
   buffer[dt_SHORT_STR_LEN] = 0; 
   return buffer;
}
