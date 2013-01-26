/*****************
gpstool.c -- tools for modifying a GpFile structure
Creation date: February 8, 2010
Last updated:  February 24, 2010

Description:
This program contains some tools and commands that can be performed
on a GpFile that has been read into memory properly,
these commands are:
	-info (get information about the structure)
	-discard components (remove certain components of the structure)
	-keep components (keep only certain components of the structure)
	-sorwp (sort the waypoints in the structure alphabetically)
	-merge fileb (merge two different gps files) NOT FULLY FUNCTIONAL
	
Upon completion of the command specified the program will write
all the relative information left in the strucure out to
a new file (stdout is the default)

NOTE: the merge option is not fully functional.
it will only merge the waypoints and routes, as well it will not
check for duplicate waypoints ids or route ids

David Farr
0629099
farrd@uoguelph.ca
*****************/

#include "gputil.h"
#include "gpstool.h"

/* This function acts as the control function for the GpsTool
   it reads takes in command line arguments, makes sure they are valid
   and the calls the appropraite subfunctions to complete the commands */
int main(int argc, char *argv[])
{
	GpStatus status;
	GpFile *filep = malloc(sizeof(GpFile));		/* settings associated with the GPSU file */
	assert(filep);
	FILE *const in = stdin;
	FILE *const out = stdout;
	int lines, exitStatus;

	/* makes sure there are enought arguments (at least 1) */
	if(argc < 2 || argc > 3)
	{
		fprintf(stderr, "\nIvalid number of commands.\n");
		fprintf(stderr, "The proper command line arguments are:\n");
		fprintf(stderr, "	-info\n");
		fprintf(stderr, "	-discard (w,r,t)\n");
		fprintf(stderr, "	-keep (w,r,t)\n");
		fprintf(stderr, "	-sortwp\n");
		fprintf(stderr, "	-merge fileb\n\n");

		free(filep);
		return EXIT_FAILURE;
	}
	
	strToLower(argv[1]);	// convert the arg to all lowercase

	/* read the file, fatal error if anything but OK is returned */
	status = readGpFile(in, filep);
	if(status.code != OK)
	{
		fprintf(stderr, "\nFailed to read in the GPS file.\n");
		
		if(status.code == IOERR)
			fprintf(stderr, "An I/O Error occured on line %d.\n\n", status.lineno);
		else if(status.code == UNKREC)
			fprintf(stderr, "An Unknown Record Error occured on line %d.\n\n", status.lineno);
		else if(status.code == BADSEP)
			fprintf(stderr, "A Bad Seperation Error occured on line %d.\n\n", status.lineno);
		else if(status.code == FILTYP)
			fprintf(stderr, "A File Type Error occured on line %d.\n\n", status.lineno);
		else if(status.code == DATUM)
			fprintf(stderr, "A Datum Error occured on line %d.\n\n", status.lineno);
		else if(status.code == COORD)
			fprintf(stderr, "A Coordinate Error occured on line %d.\n\n", status.lineno);
		else if(status.code == NOFORM)
			fprintf(stderr, "A No Form Error occured on line %d.\n\n", status.lineno);
		else if(status.code == FIELD)
			fprintf(stderr, "A No Field Error on line %d.\n\n", status.lineno);
		else if(status.code == VALUE)
			fprintf(stderr, "A Value Error occured on line %d.\n\n", status.lineno);
		else if(status.code == DUPRT)
			fprintf(stderr, "A Duplicate Route Error occured on line %d.\n\n", status.lineno);
		else if(status.code == UNKWPT)
			fprintf(stderr, "An Unknown Waypoint Error occured on line %d.\n\n", status.lineno);
		
		freeGpFile(filep);
		free(filep);
		return EXIT_FAILURE;
	}

	/* Determine which command is being invoked */
	
	/* the information option */
	if(strcmp(argv[1], "-info") == 0)
	{
		exitStatus = gpsInfo(out, filep);
	}

	/* the discard option */
	else if(strcmp(argv[1], "-discard") == 0 && argv[2] != NULL)
	{
		/* check to make sure the command is proper */
		strToLower(argv[2]);
		if(strlen(argv[2]) > 3)
		{
			fprintf(stderr, "\nInvalid command, the proper discard command is:\n");
			fprintf(stderr, "	-discard ('w', 'r', and/or 't')\n\n");
			
			
			freeGpFile(filep);
			free(filep);
			return EXIT_FAILURE;
		}
		
		for(int index = 0; index < strlen(argv[2]); index++)
		{
			if(argv[2][index] != 'w' && argv[2][index] != 'r' && argv[2][index] != 't')
			{
				fprintf(stderr, "\nInvalid command, the proper discard command is:\n");
				fprintf(stderr, "	-discard ('w', 'r', and/or 't')\n\n");
			
			
				freeGpFile(filep);
				free(filep);
				return EXIT_FAILURE;
			}
		}
		
		exitStatus = gpsDiscard(filep, argv[2]);
	}
	
	/* the keep option (inverse of discard) */
	else if(strcmp(argv[1], "-keep") == 0 && argv[2] != NULL)
	{
		char *keep = malloc(sizeof(char) * 4);
		assert(keep);
		strcpy(keep, "wrt");
		
		/* removes the components if they are found,
		   (this determines the inverse of discard) */
		strToLower(argv[2]);
		/* check to make sure the command is proper */
		strToLower(argv[2]);
		if(strlen(argv[2]) > 3)
		{
			fprintf(stderr, "\nInvalid command, the proper keep command is:\n");
			fprintf(stderr, "	-keep ('w', 'r', and/or 't')\n\n");
			
			
			freeGpFile(filep);
			free(filep);
			return EXIT_FAILURE;
		}
		
		for(int index = 0; index < strlen(argv[2]); index++)
		{
			if(argv[2][index] != 'w' && argv[2][index] != 'r' && argv[2][index] != 't')
			{
				fprintf(stderr, "\nInvalid command, the proper keep command is:\n");
				fprintf(stderr, "	-keep ('w', 'r', and/or 't')\n\n");
			
			
				freeGpFile(filep);
				free(filep);
				return EXIT_FAILURE;
			}
			
			removeChar(argv[2][index], keep);
		}
		
		exitStatus = gpsDiscard(filep, keep);
		free(keep);
	}

	/* the sort waypoints option */
	else if(strcmp(argv[1], "-sortwp") == 0)
	{
		exitStatus = gpsSort(filep);
	}
	
	/* the merge option */
	else if(strcmp(argv[1], "-merge") == 0 && argv[2] != NULL)
	{
		char *fileb;
		
		/* append .gps to fileb if need be */
		if(strstr(argv[2], ".gps") == NULL)
		{
			fileb = malloc(sizeof(char) * (strlen(argv[2]) + 5));
			assert(fileb);
			strcpy(fileb, argv[2]);
			strcat(fileb, ".gps");
		}
		else
		{
			fileb = malloc(sizeof(char) * (strlen(argv[2]) + 1));
			assert(fileb);
			strcpy(fileb, argv[2]);
		}
		
		/* merge the files */
		exitStatus = gpsMerge(filep, fileb);
		free(fileb);
	}
	
	/* if an unrecognized argument is encountered */
	else
	{
		fprintf(stderr, "\nIvalid command.\n");
		fprintf(stderr, "The proper command line arguments are:\n");
		fprintf(stderr, "	-info\n");
		fprintf(stderr, "	-discard (w,r,t)\n");
		fprintf(stderr, "	-keep (w,r,t)\n");
		fprintf(stderr, "	-sortwp\n");
		fprintf(stderr, "	-merge fileb\n\n");
	
		freeGpFile(filep);
		free(filep);
		return EXIT_FAILURE;
	}
		
	
	/* write the new file */
	lines = writeGpFile(out, filep);
	if(lines == 0)
		fprintf(stderr, "\nProblem writing out the file\n\n");

	/* free stuff */
	freeGpFile(filep);
	free(filep);

	printf("Exit Status: %d\n", exitStatus);
	return EXIT_SUCCESS;
}

/* This function looks at the infomation in the GpFile structure
   and determines some information about the data
   - the number of waypoints, and whether or not these are sorted
   - the number of routes
   - the number of trackpoints
   - the extent (farthest SW and NE corners) */
int gpsInfo(FILE *const outfile, const GpFile *filep)
{
	double north = -90, east = -180, south = 90, west = 180;
	char n, e, s, w;
	int numTracks, sorted = true;
	
	/* check to see if the waypts are sorted and look for the extent */
	for(int index = 0; index < filep->nwaypts; index++)
	{
		/* see if the temp word is in order */
		if(index < filep->nwaypts - 1)
		{
			if(strcmp(filep->waypt[index].ID, filep->waypt[index + 1].ID) > 0)
				sorted = false;
		}
		
		/* check for the extent (SW corner and NE corner) */
		if(filep->waypt[index].coord.lat > north)
			north = filep->waypt[index].coord.lat;
		
		if(filep->waypt[index].coord.lon > east)
			east = filep->waypt[index].coord.lon;
		
		if(filep->waypt[index].coord.lat < south)
			south = filep->waypt[index].coord.lat;
		
		if(filep->waypt[index].coord.lon < west)
			west = filep->waypt[index].coord.lon;
	}
	
	/* get the number of tracks */
	GpTrack *track;
	numTracks = getGpTracks(filep, &track);
	
	/* check for the extent (SW corner and NE corner) */
	for(int index = 0; index < numTracks; index++)
	{
		if(track[index].NEcorner.lat > north)
			north = track[index].NEcorner.lat;
		
		if(track[index].NEcorner.lon > east)
			east = track[index].NEcorner.lon;
		
		if(track[index].SWcorner.lat < south)
			south = track[index].SWcorner.lat;
		
		if(track[index].SWcorner.lon < west)
			west = track[index].SWcorner.lon;
	}
	
	/* convert the lat/lon for extent to better form (used instead of + or - ) */
	if(north > 0)
		n = 'N';
	else
	{
		n = 'S';
		north = north * -1;
	}
	
	if(east > 0)
		e = 'E';
	else
	{
		e = 'W';
		east = east * -1;
	}
	
	if(south > 0)
		s = 'N';
	else
	{
		s = 'S';
		south = south * -1;
	}
	
	if(west > 0)
		w = 'E';
	else
	{
		w = 'W';
		west = west * -1;
	}
	
	/* print out the information */
	if(filep->nwaypts == 0)
		fprintf(outfile, "%d waypoints\n", filep->nwaypts);
	else
	{
		if(sorted)
			fprintf(outfile, "%d waypoints (sorted)\n", filep->nwaypts);
		else
			fprintf(outfile, "%d waypoints (not sorted)\n", filep->nwaypts);
	}
	
	fprintf(outfile, "%d routes\n", filep->nroutes);
	fprintf(outfile, "%d trackpoints\n", filep->ntrkpts);
	fprintf(outfile, "%d tracks\n", numTracks);
	
	fprintf(outfile, "Extent: SW %c%03.6lf %c%02.6lf to NE %c%03.6lf %c%02.6lf\n", w, west, s, south, e, east, n, north);
	
	if(numTracks > 0)
		free(track);
	
	return EXIT_SUCCESS;
}

/* This function will remove whatever components of the GpFile
   are specified on the command line and keep the ones that aren't,
   (if waypoints are removed then routes are removed since
   routes require waypoints) */
int gpsDiscard(GpFile *filep, const char *which)
{
	/* make sure we're not deleting all the components */
	if((strchr(which, 'w') && strchr(which, 't')) || (strchr(which, 'w') && filep->ntrkpts == 0) || (strchr(which, 't') && filep->nwaypts == 0))
	{
		fprintf(stderr, "\nA non-fatal error has occured:\n");
		fprintf(stderr, "	- all components of the GpFile were removed\n");
		fprintf(stderr, "	- the original input will be written to a file\n\n");

		return EXIT_SUCCESS;
	}
	
	
	/* discard the waypoints */
	if(strchr(which, 'w'))
	{
		for(int index = 0; index < filep->nwaypts; index++)
		{
			free(filep->waypt[index].ID);
			free(filep->waypt[index].symbol);
			free(filep->waypt[index].comment);
		}
		
		filep->nwaypts = 0;
	}
	
	/* discard the routes */
	if(strchr(which, 'r') || strchr(which, 'w') )
	{
		for(int index = 0; index < filep->nroutes; index++)
		{
			free(filep->route[index]->comment);
			free(filep->route[index]);
		}
		
		filep->nroutes = 0;
	}
	
	/* discard the trackpoints */
	if(strchr(which, 't'))
	{
		for(int index = 0; index < filep->ntrkpts; index++)
		{
			if(filep->trkpt[index].segFlag == true)
				free(filep->trkpt[index].comment);
		}
		
		filep->ntrkpts = 0;
	}
	
	return EXIT_SUCCESS;
}

/* This function will keep whatever components of the GpFile
   are specified on the command line and remove the ones that aren't,
   (if waypoints are removed then routes are removed since
   routes require waypoints) */
int gpsSort(GpFile *filep)
{
	/* check to see if the waypoints are already sorted */
	int sorted = true;
	
	/* check to see if the waypts are sorted */
	for(int index = 0; index < filep->nwaypts - 1; index++)
	{
		if(strcmp(filep->waypt[index].ID, filep->waypt[index + 1].ID) > 0)
		{
			sorted = false;
			break;
		}
	}
	
	/* return if they are already sorted */
	if(sorted)
	{
		fprintf(stderr, "\nA non-fatal error has occured:\n");
		fprintf(stderr, "	- the waypoints are already sorted\n");
		fprintf(stderr, "	- the original input will be written to a file\n\n");

		return EXIT_SUCCESS;
	}
	
	/* sort the waypoints if need be */
	legInfo *adjust = malloc(sizeof(legInfo) * filep->nwaypts);
	assert(adjust);
	
	/* save the old index numbers for the legs */
	for(int index = 0; index < filep->nwaypts; index++)
	{
		adjust[index].ID = malloc(sizeof(char) * (strlen(filep->waypt[index].ID) + 1));
		assert(adjust[index].ID);
		strcpy(adjust[index].ID, filep->waypt[index].ID);
	}

	/* use quicksort to sort the waypoints */
	qsort(filep->waypt, filep->nwaypts, sizeof(GpWaypt), (compFn)compare);
	
	/* update the array of structures with the new index numbers */
	for(int index = 0; index < filep->nwaypts; index++)
	{
		for(int leg = 0; leg < filep->nwaypts; leg++)
		{
			if(strcmp(filep->waypt[index].ID, adjust[leg].ID) == 0)
			{
				adjust[leg].newNum = index;
				break;
			}
		}
	}

	/* update the leg array in all the routes */
	for(int index = 0; index < filep->nroutes; index++)
	{
		for(int wayptIndex = 0; wayptIndex < filep->route[index]->npoints; wayptIndex++)
			filep->route[index]->leg[wayptIndex] = adjust[filep->route[index]->leg[wayptIndex]].newNum;
	}
	
	/* free the leg info structure */
	for(int index = 0; index < filep->nwaypts; index++)
		free(adjust[index].ID);
	
	free(adjust);
	
	return EXIT_SUCCESS;
}

/* This function merges two GpFile structures into one.
   waypoint ids and symbols are expanded if need be,
   route numbers are adjusted to the next block of 100, and
   fileb's date/time and distance/speed are adjust to conform to filep's */
int gpsMerge(GpFile *filep, const char *const fnameB)
{
	FILE *const input = fopen(fnameB, "r");
	if(input == NULL)
	{
		fprintf(stderr, "\nA non-fatal error has occured:\n");
		fprintf(stderr, "	- failed to open fileb\n");
		fprintf(stderr, "	- the original input will be written to a file\n\n");

		return EXIT_SUCCESS;
	}
	
	GpStatus status;
	GpFile *fileb = malloc(sizeof(GpFile));
	assert(fileb);
	
	/* read fileb into a GpFile structure, and verify it's OK */
	status = readGpFile(input, fileb);
	fclose(input);
	if(status.code != OK)
	{
		fprintf(stderr, "\nA non-fatal error has occured:\n");
		fprintf(stderr, "	- failed to read in fileb\n");
		fprintf(stderr, "	- the original input will be written to a file\n\n");

		freeGpFile(fileb);
		free(fileb);
		return EXIT_SUCCESS;
	}
	
	int offset = filep->nwaypts;
	
	/* merge the components of fileb with filep,
	   but only if fileb contains that component */
	if(fileb->nwaypts > 0)
		mergeWaypts(filep, fileb);
	
	if(fileb->nroutes > 0)
		mergeRoutes(filep, fileb, offset);

	if(fileb->ntrkpts > 0)
		mergeTrkpts(filep, fileb);
	
	freeGpFile(fileb);
	free(fileb);
	return EXIT_SUCCESS;
}

/* This function merges the waypoints of two structures NOT COMPLETE!
   Input: two Gpfiles whose waypoints are to be combined into one
          (fileb added to filep) */
void mergeWaypts(GpFile *filep, GpFile *fileb)
{
	if(filep->nwaypts > 0)
	{
		int id, symbol;
		
		/* determine which ID is bigger, adjust the smaller one */
		if(strlen(filep->waypt[0].ID) > strlen(fileb->waypt[0].ID))
		{
			id = strlen(filep->waypt[0].ID) + 1;
			
			for(int index = 0; index < fileb->nwaypts; index++)
			{
				fileb->waypt[index].ID = realloc(fileb->waypt[index].ID, sizeof(char) * id);
				assert(fileb->waypt[index].ID);
				
				while(strlen(fileb->waypt[index].ID) < id - 1)
					strcat(fileb->waypt[index].ID, " ");
			}
		}
		else if(strlen(filep->waypt[0].ID) < strlen(fileb->waypt[0].ID))
		{
			id = strlen(fileb->waypt[0].ID) + 1;
			
			for(int index = 0; index < filep->nwaypts; index++)
			{
				filep->waypt[index].ID = realloc(filep->waypt[index].ID, sizeof(char) * id);
				assert(filep->waypt[index].ID);
				
				while(strlen(filep->waypt[index].ID) < id - 1)
					strcat(filep->waypt[index].ID, " ");
			}
		}
		
		/* determine which symbol is bigger, adjust the smaller one */
		if(strlen(filep->waypt[0].symbol) > strlen(fileb->waypt[0].symbol))
		{
			symbol = strlen(filep->waypt[0].symbol) + 1;
			
			for(int index = 0; index < fileb->nwaypts; index++)
			{
				fileb->waypt[index].symbol = realloc(fileb->waypt[index].symbol, sizeof(char) * symbol);
				assert(fileb->waypt[index].symbol);
				
				while(strlen(fileb->waypt[index].symbol) < symbol - 1)
					strcat(fileb->waypt[index].symbol, " ");
			}
		}
		else if(strlen(filep->waypt[0].symbol) < strlen(fileb->waypt[0].symbol))
		{
			symbol = strlen(fileb->waypt[0].symbol) + 1;
			
			for(int index = 0; index < filep->nwaypts; index++)
			{
				filep->waypt[index].symbol = realloc(filep->waypt[index].symbol, sizeof(char) * symbol);
				assert(filep->waypt[index].symbol);
				
				while(strlen(filep->waypt[index].symbol) < symbol - 1)
					strcat(filep->waypt[index].symbol, " ");
			}
		}
	}
	
	filep->waypt = realloc(filep->waypt, sizeof(GpWaypt) * (filep->nwaypts + fileb->nwaypts));
	assert(filep->waypt);
	
	for(int index = 0; index < fileb->nwaypts; index++)
	{
		filep->waypt[filep->nwaypts + index].ID = malloc(sizeof(char) * (strlen(fileb->waypt[index].ID) + 1));
		assert(filep->waypt[filep->nwaypts + index].ID);
		strcpy(filep->waypt[filep->nwaypts + index].ID, fileb->waypt[index].ID);
		
		filep->waypt[filep->nwaypts + index].coord.lat = fileb->waypt[index].coord.lat;
		filep->waypt[filep->nwaypts + index].coord.lon = fileb->waypt[index].coord.lon;
		
		filep->waypt[filep->nwaypts + index].symbol = malloc(sizeof(char) * (strlen(fileb->waypt[index].symbol) + 1));
		assert(filep->waypt[filep->nwaypts + index].symbol);
		strcpy(filep->waypt[filep->nwaypts + index].symbol, fileb->waypt[index].symbol);
		
		filep->waypt[filep->nwaypts + index].textChoice = fileb->waypt[index].textChoice;
		filep->waypt[filep->nwaypts + index].textPlace = fileb->waypt[index].textPlace;
		
		filep->waypt[filep->nwaypts + index].comment = malloc(sizeof(char) * (strlen(fileb->waypt[index].comment) + 1));
		assert(filep->waypt[filep->nwaypts + index].comment);
		strcpy(filep->waypt[filep->nwaypts + index].comment, fileb->waypt[index].comment);
	}
	
	filep->nwaypts = filep->nwaypts + fileb->nwaypts;
}

/* This function merges the  routes of two structures
   Input: two Gpfiles whose routes are to be combined into one
	  (fileb added to filep) and the offset for the new routes */
void mergeRoutes(GpFile *filep, GpFile *fileb, int offset)
{
	int max = 0, start;
	
	/* find the highest route no. */
	for(int index = 0; index < filep->nroutes; index++)
	{
		if(filep->route[index]->number > max)
			max = filep->route[index]->number;
	}
	
	/* find out where the new route no's will start */
	start = (max / 100) + 1;
	start = start * 100;
	
	/* merge the routes */
	filep->route = realloc(filep->route, sizeof(GpRoute*) * (filep->nroutes + fileb->nroutes));
	assert(filep->route);
	
	for(int index = 0; index < fileb->nroutes; index++)
	{
		filep->route[filep->nroutes + index] = malloc(sizeof(GpRoute) + (sizeof(int) * fileb->route[index]->npoints));
		assert(filep->route[filep->nroutes + index]);
		
		filep->route[filep->nroutes + index]->number = start + (fileb->route[index]->number % 100);
		
		filep->route[filep->nroutes + index]->comment = malloc(sizeof(char) * strlen(fileb->route[index]->comment) + 1);
		assert(filep->route[filep->nroutes + index]->comment);
		strcpy(filep->route[filep->nroutes + index]->comment, fileb->route[index]->comment);
		
		filep->route[filep->nroutes + index]->npoints = fileb->route[index]->npoints;
		for(int k = 0; k < fileb->route[index]->npoints; k++)
			filep->route[filep->nroutes + index]->leg[k] = fileb->route[index]->leg[k] + offset;
	}
	
	filep->nroutes = filep->nroutes + fileb->nroutes;
}

/* This function merges the trackpoints of two structures, it also
   converts the date/time and distance/speed of fileb to conform to filep
   Input: two Gpfiles whose waypoints are to be combined into one
          (fileb added to filep) */
void mergeTrkpts(GpFile *filep, GpFile *fileb)
{
	filep->trkpt = realloc(filep->trkpt, sizeof(GpTrkpt) * (filep->ntrkpts + fileb->ntrkpts));
	assert(filep->trkpt);
	
	char *dateFormat, *date;
	int timeShift;
	struct tm temp;
	temp.tm_isdst = -1;
	
	dateFormat = malloc(sizeof(char) * (strlen(filep->dateFormat) + 10));
	assert(dateFormat);
	date = malloc(sizeof(char) * 26);
	assert(date);
	strcpy(dateFormat, filep->dateFormat);
	strcat(dateFormat, " %H:%M:%S");
	
	timeShift = filep->timeZone - fileb->timeZone;
	
	for(int index = 0; index < fileb->ntrkpts; index++)
	{
		filep->trkpt[filep->ntrkpts + index].coord.lat = fileb->trkpt[index].coord.lat;
		filep->trkpt[filep->ntrkpts + index].coord.lon = fileb->trkpt[index].coord.lon;
		filep->trkpt[filep->ntrkpts + index].segFlag = fileb->trkpt[index].segFlag;
		filep->trkpt[filep->ntrkpts + index].duration = fileb->trkpt[index].duration;
		
		if(filep->trkpt[filep->ntrkpts + index].segFlag)
		{
			filep->trkpt[filep->ntrkpts + index].comment = malloc(sizeof(char) * (strlen(fileb->trkpt[index].comment) + 1));
			assert(filep->trkpt[filep->ntrkpts + index].comment);
			strcpy(filep->trkpt[filep->ntrkpts + index].comment, fileb->trkpt[index].comment);
		}
		
		/* date/time (needs to be converted)*/
		gmtime_r(&fileb->trkpt[index].dateTime, &temp);
		temp.tm_hour = temp.tm_hour - 5; // WHY???
		temp.tm_hour = temp.tm_hour + timeShift;
		strftime(date, sizeof(char) * 26, dateFormat, &temp);
		strptime(date, dateFormat, &temp);
		filep->trkpt[filep->ntrkpts + index].dateTime = mktime(&temp);
		
		/* the distance and speed (needs to be converted) */
		double dist = fileb->trkpt[index].dist;
		float speed = fileb->trkpt[index].speed;
		
		/* convert everything to m and m/s */
		if(fileb->unitHorz == 'K')
		{
			dist = dist * 1000.0;
			speed = speed / 3.6;
		}
		else if(fileb->unitHorz == 'F')
		{
			dist = dist / 3.2808399;
			speed = speed / 3.2808399;
		}
		else if(fileb->unitHorz == 'N')
		{
			dist = dist * 1852.0;
			speed = speed / 1.94384449;
		}
		else if(fileb->unitHorz == 'S')
		{
			dist = dist * 1609.344;
			speed = speed / 2.23693629;
		}
		
		/* convert from m and m/s to new format */
		if(filep->unitHorz == 'K')
		{
			dist = dist / 1000.0;
			speed = speed * 3.6;
		}
		else if(fileb->unitHorz == 'F')
		{
			dist = dist * 3.2808399;
			speed = speed * 3.2808399;
		}
		else if(fileb->unitHorz == 'N')
		{
			dist = dist / 1852.0;
			speed = speed * 1.94384449;
		}
		else if(fileb->unitHorz == 'S')
		{
			dist = dist / 1609.344;
			speed = speed * 2.23693629;
		}
		
		filep->trkpt[filep->ntrkpts + index].dist = dist;
		filep->trkpt[filep->ntrkpts + index].speed = speed;
	}
	
	free(dateFormat);
	free(date);
	filep->ntrkpts = filep->ntrkpts + fileb->ntrkpts;
}

/* This function compares two strings in a GpWaypt structure
   it is used as a function pointer in a call to qsort
   to sort an array of waypoints */
int compare(GpWaypt *a, GpWaypt *b)
{
	return (strcmp(a->ID, b->ID));
}
