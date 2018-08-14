/*
 * Adds support for the Tabular Islamic calendar to PHP's calendar
 * functions.
 * Author: Kurt M. Weber <kmw@kurtweber.us>
 */

/*
 * We use the Tabular Islamic calendar because the standard Islamic
 * calendar relies on real-time astronomical observations to determine when
 * a new month begins, which is not feasible for computer conversion of
 * dates.  Further complicating the fact is that Islam lacks a universal
 * hierarchy, so observations are made on a country-by-country basis.
 * It is therefore possible that converted dates will differ by a few days.
 */

/**************************************************************************
 *
 * These are the externally visible components of this file:
 *
 *     void
 *     SdnToFrench(
 *         long int  sdn,
 *         int      *pYear,
 *         int      *pMonth,
 *         int      *pDay);
 *
 * Convert a SDN to a French republican calendar date.  If the input SDN is
 * before the first day of year 1 or after the last day of year 14, the
 * three output values will all be set to zero, otherwise *pYear will be in
 * the range 1 to 14 inclusive; *pMonth will be in the range 1 to 13
 * inclusive; *pDay will be in the range 1 to 30 inclusive.  If *pMonth is
 * 13, the SDN represents one of the holidays at the end of the year and
 * *pDay will be in the range 1 to 6 inclusive.
 *
 *     long int
 *     FrenchToSdn(
 *         int year,
 *         int month,
 *         int day);
 *
 * Convert a French republican calendar date to a SDN.  Zero is returned
 * when the input date is detected as invalid or out of the supported
 * range.  The return value will be > 0 for all valid, supported dates, but
 * there are some invalid dates that will return a positive value.  To
 * verify that a date is valid, convert it to SDN and then back and compare
 * with the original.
 *
 *     char *FrenchMonthName[14];
 *
 * Convert a French republican month number (1 to 13) to the name of the
 * French republican month (null terminated).  An index of 13 (for the
 * "extra" days at the end of the year) will return the string "Extra".  An
 * index of zero will return a zero length string.
 *
 * VALID RANGE
 *
 *     These routines only convert dates in years 1 through 14 (Gregorian
 *     dates 22 September 1792 through 22 September 1806).  This more than
 *     covers the period when the calendar was in use.
 *
 *     I would support a wider range of dates, but I have not been able to
 *     find an authoritative definition of when leap years were to have
 *     occurred.  There are suggestions that it was to skip a leap year ever
 *     100 years like the Gregorian calendar.
 *
 * CALENDAR OVERVIEW
 *
 *     The French republican calendar was adopted in October 1793 during
 *     the French Revolution and was abandoned in January 1806.  The intent
 *     was to create a new calendar system that was based on scientific
 *     principals, not religious traditions.
 *
 *     The year is divided into 12 months of 30 days each.  The remaining 5
 *     to 6 days in the year are grouped at the end and are holidays.  Each
 *     month is divided into three decades (instead of weeks) of 10 days
 *     each.
 *
 *     The epoch (first day of the first year) is 22 September 1792 in the
 *     Gregorian calendar.  Leap years are every fourth year (year 3, 7,
 *     11, etc.)
 *
 * TESTING
 *
 *     This algorithm has been tested from the year 1 to 14.  The source
 *     code of the verification program is included in this package.
 *
 * REFERENCES
 *
 *     I have found no detailed, authoritative reference on this calendar.
 *     The algorithms are based on a preponderance of less authoritative
 *     sources.
 *
 **************************************************************************/

#include "sdncal.h"

#define ISLAMIC_SDN_OFFSET	1948440
#define CALENDAR_CYCLE_YEARS	30
#define DAYS_PER_30_YEARS	10631

static zend_long getYearInCycle(zend_long dayInCycle, zend_long *yearEndDays);
static zend_long getMonth(zend_long dayInYear, zend_long *monthEndDays);

void SdnToTabIslamic(
					zend_long sdn,
					int *pYear,
					int *pMonth,
					int *pDay)
{
	zend_long offsetJulianDay;
	zend_long cycleNum;
	zend_long dayInCycle;
	zend_long yearInCycle;
	zend_long yearBaseDay;
	zend_long dayInYear;
	zend_long monthInYear;
	zend_long monthBaseDay;
	zend_long dayInMonth;

	zend_long yearEndDays[] = {354, 709, 1063, 1417, 1772, 2126, 2481, 2835, 3189, 3544, 3898, 4252, 4607, 4961, 5315, 5670, 6024, 6379, 6733, 7087, 7442, 7796, 8150, 8505, 8859, 9214, 9568, 9922, 10277, 10631};

	/* only leap years have 355 days, but we can include them in this list
	 * regardless because in non-leap years, we will have already advanced
	 * to the next year
	 */
	zend_long monthEndDays[] = {30, 59, 89, 118, 148, 177, 207, 236, 266, 295, 325, 355};

	if (sdn < ISLAMIC_SDN_OFFSET){
		*pYear = 0;
		*pMonth = 0;
		*pDay = 0;
		return;
	}

	offsetJulianDay = sdn - ISLAMIC_SDN_OFFSET;

	cycleNum = offsetJulianDay / DAYS_PER_30_YEARS;
	dayInCycle = offsetJulianDay - (cycleNum * DAYS_PER_30_YEARS);
	yearInCycle = getYearInCycle(dayInCycle, yearEndDays);
	*pYear = (cycleNum * 30) + yearInCycle;

	if (yearInCycle == 1){
		yearBaseDay = 0;
	} else {
		yearBaseDay = yearEndDays[yearInCycle - 2];
	}

	dayInYear = dayInCycle - yearBaseDay;
	monthInYear = getMonth(dayInYear, monthEndDays);
	*pMonth = monthInYear;
	
	if (monthInYear == 1){
		monthBaseDay = 0;
	} else {
		monthBaseDay = monthEndDays[monthInYear - 2];
	}

	dayInMonth = (dayInYear - monthBaseDay) + 1;
	*pDay = dayInMonth;
	
	return;
}

static zend_long getYearInCycle(zend_long dayInCycle, zend_long *yearEndDays){
	unsigned int i;

	/* from the calculations in SdnToTabIslamic, the first day of each 30-year
	 * cycle is represented as day 0, so we check for comparison with < rather
	 * than <=
	 */

	for (i = 0; i < CALENDAR_CYCLE_YEARS; i++){
		if (dayInCycle < yearEndDays[i]){
			return i + 1;
		}
	}

	/* should never reach this point; if we have, the problem isn't invalid
	 * input but corruption of data within the program
	 */
	return 0;
}

static zend_long getMonth(zend_long dayInYear, zend_long *monthEndDays){
	unsigned int i;

	for (i = 0; i < 12; i++){
		if (dayInYear < monthEndDays[i]){
			return i + 1;
		}
	}

	/* again, can't happen */
	return 0;
}

zend_long TabIslamicToSdn(
						int year,
						int month,
						int day)
{
	return 0;
}

const char * const TabIslamicMonthName[13] =
{
	"",
	"Vendemiaire",
	"Brumaire",
	"Frimaire",
	"Nivose",
	"Pluviose",
	"Ventose",
	"Germinal",
	"Floreal",
	"Prairial",
	"Messidor",
	"Fructidor",
	"Extra"
};
