/********
gpstool.h -- header file for gpstool.c in Asmt 2
Last updated:  January 28, 2010 03:42:46 PM       

Customize this header (see submission instructions)
********/
#ifndef GPSTOOL_H_
#define GPSTOOL_H_ A2

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>
#include <assert.h>

#include "gputil.h"

/* a structure for adjusting the legs when the waypoints are sorted */
typedef struct {
	char *ID;
	int newNum;
} legInfo;

int gpsInfo( FILE *const outfile, const GpFile *filep );
int gpsDiscard( GpFile *filep, const char *which );
int gpsSort( GpFile *filep );
int gpsMerge( GpFile *filep, const char *const fnameB );

/* functions I added */
void mergeWaypts(GpFile *filep, GpFile *fileb);
void mergeRoutes(GpFile *filep, GpFile *fileb, int offset);
void mergeTrkpts(GpFile *filep,GpFile *fileb);

/* the compare function for the qsort */
typedef int (*compFn)(const void*, const void*);
int compare(GpWaypt *, GpWaypt *);

#endif
