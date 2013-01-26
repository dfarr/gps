/*****************
gputil.c -- utilities for a gps file parser
Creation date: January 18, 2010
Last updated:  February 16, 2010

Description:
This program takes in a gps file containing waypoints, routes, and trackpoints
and fills a structure containing all the information that is neccessary
from the file, an error will be returned if a problem is encountered.
Also all the tracks in the structure will be calculated if the file is
read in correctly.

David Farr
0629099
farrd@uoguelph.ca
*****************/

#include "gputil.h"


/* This function reads in the first two characters of a line in a GPS file,
   and then determines what function(if any) to pass control
   to based on the letter read in*/
GpStatus readGpFile( FILE *const gpf, GpFile *filep )
{
	/* initialize the status structure */
	GpStatus status;
	status.code = OK;
	status.lineno = 0;

	/* initialize the settings in GpFile */
	filep->dateFormat = malloc(sizeof(char) * 9);
	assert(filep->dateFormat);
	strcpy(filep->dateFormat, GP_DATEFORMAT);
	filep->timeZone = GP_TIMEZONE;
	filep->unitHorz = GP_UNITHORZ;
	filep->unitTime = GP_UNITTIME;

	/* initialize the waypoint structure */
	filep->waypt = malloc(sizeof(GpWaypt));
	assert(filep->waypt);
	filep->nwaypts = 0;

	/* initialize the route structure */
	filep->route = malloc(sizeof(GpRoute*));
	assert(filep->route);
	filep->nroutes = 0;

	/* initialize the trackpoint structure */
	filep->trkpt = malloc(sizeof(GpTrkpt));
	assert(filep->trkpt);
	filep->ntrkpts = 0;

	/* max lengths of strings (are doubled if need be) */
	int lineMax = 20;
	int fieldMax = 20;

	/* used to scan in W lines that correspond to route */
	int flag = 0;

	/* some necessary variables */
	char c;
	char *line = malloc(sizeof(char) * lineMax);
	assert(line);
	char *fieldDef = malloc(sizeof(char) * fieldMax);
	assert(fieldDef);
	
	/* deal with all the different types of lines (based on the first character of each line) */
	while ((c = fgetc(gpf)) != EOF)
	{		
		int index = 0;
		while (c != '\n' && c != '\r' && c != EOF)
		{
			if(index == lineMax)
			{
				lineMax = lineMax * 2;
				line = realloc(line, sizeof(char) * lineMax);
				assert(line);
			}

			line[index++] = c;
			c = fgetc(gpf);
		}
		line[index] = '\0';

		/* convert the line to lowercase for some of the line types */
		if(line[0] != 'W' && line[0] != 'w' && line[0] != 'R' && line[0] != 'r' && line[0] != 'T' && line[0] != 't')
			strToLower(line);

		status.lineno++;	// keep incrementing the line number, needed if an error occurs

		/* look for a bad separation error */
		if(line[0] != '\0' && !isspace(line[0]) && !isspace(line[1]))
		{
			free(line);
			free(fieldDef);

			status.code = BADSEP;
			return status;
		}

		/* perform an action based on the type(first letter in line), some letters are ignored */
		switch(line[0])
		{
			/**** 
			about the flag
				- flag initialized to 0
				- r sets flag to 2
				- f sets flag from 2->1, then 1->0
				- everything else sets flag to 0
				- w, if flag = 0 scan in waypoint, else scan in legpoint 
			****/

			case('i'):
				flag = 0;

				status.code = verifyFile(line);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				break;
			case('s'):
				flag = 0;

				status.code = changeSettings(line, filep);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				break;
			case('m'):
				flag = 0;

				status.code = verifyDatum(line);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				break;
			case('u'):
				flag = 0;

				status.code = verifyCoords(line);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				break;
			case('f'):
				/* make fieldDef bigger if needed */
				if(strlen(line) >= fieldMax)
				{
					fieldMax = strlen(line) + 1;
					fieldDef = realloc(fieldDef, sizeof(char) * fieldMax);
					assert(fieldDef);
				}

				/* save the line as the field def */
				strcpy(fieldDef, line);
				fieldDef[strlen(fieldDef)] = '\0';

				/* change the flag */
				if(flag == 2)
					flag = 1;
				else if(flag == 1)
					flag = 0;

				break;
			case('w'):
			case('W'):		
				/* scan in a waypoint */
				if(flag == 0)
				{
					/* increment the counter and malloc another place in the array */
					filep->nwaypts++;
					filep->waypt = realloc(filep->waypt, sizeof(GpWaypt) * filep->nwaypts);
					assert(filep->waypt);
	
					status.code = scanGpWaypt(line, fieldDef, &filep->waypt[filep->nwaypts - 1]);
					if(status.code != OK)
					{
						free(line);
						free(fieldDef);
						return status;
					}
				}

				/* scan in a legpoint */
				else
				{
					/* increment the counter and then malloc another spot 
					   in the array(include space for leg[] int array) */
					filep->route[filep->nroutes - 1]->npoints++;
					filep->route[filep->nroutes - 1] = realloc(filep->route[filep->nroutes - 1], sizeof(GpRoute) + (sizeof(int) * filep->route[filep->nroutes - 1]->npoints));
					assert(filep->route[filep->nroutes - 1]);
					
					status.code = scanGpLeg(line, fieldDef, filep->waypt, filep->nwaypts, filep->route[filep->nroutes - 1]);
					if(status.code != OK)
					{
						free(line);
						free(fieldDef);
						return status;
					}
				}

				break;
			case('r'):
			case('R'):
				/* increment the counter and malloc another spot in the array of pointers
				   and then malloc the structure that pointer is pointing to */
				filep->nroutes++;
				filep->route = realloc(filep->route, sizeof(GpRoute*) * filep->nroutes);
				assert(filep->route);
				filep->route[filep->nroutes - 1] = malloc(sizeof(GpRoute));
				assert(filep->route[filep->nroutes - 1]);
				filep->route[filep->nroutes - 1]->npoints = 0;		// initialize the # of legpoints

				/* scan in the route */
				status.code = scanGpRoute(line, filep->route[filep->nroutes - 1]);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				/* make sure the route number is not being repeated */
				for(int index = 0; index < filep->nroutes - 1; index++)
				{
					if(filep->route[index]->number == filep->route[filep->nroutes - 1]->number)
					{
						free(line);
						free(fieldDef);
	
						status.code = DUPRT;
						return status;
					}

				}

				/* set flag to initiate the reading of legpoints */
				flag = 2;

				break;
			case('t'):
			case('T'):
				flag = 0;

				/* increment the counter and malloc another spot in the array */
				filep->ntrkpts++;
				filep->trkpt = realloc(filep->trkpt, sizeof(GpTrkpt) * filep->ntrkpts);
				assert(filep->trkpt);

				/* scan in the trackpoints */
				status.code = scanGpTrkpt(line, fieldDef, filep->dateFormat, &filep->trkpt[filep->ntrkpts - 1]);
				if(status.code != OK)
				{
					free(line);
					free(fieldDef);
					return status;
				}

				break;
			case('c'):
			case('a'):
			case('h'):
			case('\0'):
				/* these are the "do nothing" cases */
				flag = 0;
				break;
			default:
				/* return an error if an unknown type of line is encountered */
				free(line);
				free(fieldDef);

				status.code = UNKREC;
				return status;

				break;
		}
 	}
	
	/* make sure the gps file was readable */
	if(ferror(gpf))
	{
		free(line);
		free(fieldDef);

		status.code = IOERR;
		return status;
	}

	/* free the strings used to scan in the data */
	free(line);
	free(fieldDef);

	return status;
}

/* This function is called when an 'I' line is encountered,
   it verifies that the file is a GPSU file
   input: line, the line being scanned
   output: FILTYP or OK */
GpError verifyFile(char *line)
{
	if(strstr(line, "gpsu") == NULL)
		return FILTYP;

	return OK;
}

/* This function is called when an 'S' line is encountered,
   it changes the settings in the filep struct
   input: line -> the line being scanned, filep -> where the settings will be stored
   output: VALUE or OK */
GpError changeSettings(char *line, GpFile *filep)
{
	/* get the "value" portion of the string, everything after the = sign */
	char value[strlen(line)];

	/* this is a check for the equals sign.
	   valgrind will still produce errors because below it takes
	   the spot AFTER the equals sign */
	if(strchr(line, '=') == NULL)
		return VALUE;

	strcpy(value, strchr(line, '=') + 1);	// somewhat bad stuff
	removeWhite(value);

	/* create the token based on the length of the value
	   (cannot exceed this length) */
	char token[strlen(value)];

	/* get the date format */
	if(strstr(line, "dateformat"))
	{
		strcpy(filep->dateFormat, "");	// clear the current format

		/* get all the tokens of the date */
		while(strcmp(value, "") != 0)
		{
			strToken(token, '/', value);	// get all the tokens (everything before and after the '/'s)

			if(strcmp(token, "dd") == 0)
				strcat(filep->dateFormat, "%d");
			else if(strcmp(token, "mm") == 0)
				strcat(filep->dateFormat, "%m");
			else if(strcmp(token, "mmm") == 0)
				strcat(filep->dateFormat, "%B");
			else if(strcmp(token, "yy") == 0)
				strcat(filep->dateFormat, "%y");
			else if(strcmp(token, "yyyy") == 0)
				strcat(filep->dateFormat, "%Y");
			else
				return VALUE;

			/* add the / if need be */
			if(strcmp(value, "") != 0)
				strcat(filep->dateFormat, "/");
		}
	}

	/* get the timezone */
	else if(strstr(line, "timezone"))
	{
		int timeZone;
		strToken(token, ':', value);	// get the token (everything before the ':')
		
		int test = sscanf(token, "%d", &timeZone);
		if(test != 1)
			return VALUE;
		/* range for timezone -12 -> 14 */
		if(timeZone < -12 || timeZone > 14)
			return VALUE;
		else
			filep->timeZone = timeZone;
	}

	/* get the units */
	else if(strstr(line, "units"))
	{
		strToken(token, ',', value);	// get the token (everything before the ',')

		/* if horizontal unit -> m,s then time untit -> s */
		if(strcmp(token, "m") == 0 || strcmp(token, "f") == 0)
		{
			strToUpper(token);
			filep->unitHorz = token[0];
			filep->unitTime = 'S';
		}
		/* if horizontal unit -> k, n,s then time untit -> h */
		else if(strcmp(token, "k") == 0 || strcmp(token, "n") == 0 || strcmp(token, "s") == 0)
		{
			strToUpper(token);
			filep->unitHorz = token[0];
			filep->unitTime = 'H';
		}
		else
			return VALUE;
	}

	return OK;
}

/* This function is called when an 'M' line is encountered,
   it verifies that the file has WGS 84
   input: line, the line being scanned
   output: DATUM or OK */
GpError verifyDatum(char *line)
{
	if(strstr(line, "wgs 84") == NULL && strstr(line, "wgs84") == NULL)
		return DATUM;

	return OK;
}

/* This function is called when an 'I' line is encountered,
   it verifies that the file has the proper coordinates
   input: line, the line being scanned
   output: COORD or OK */
GpError verifyCoords(char *line)
{
	if(strstr(line, "lat lon deg") == NULL && strstr(line, "latlondeg") == NULL)
		return COORD;

	return OK;
}

/* This function will scan in Way Points
   and store them into a contiguous array */
GpError scanGpWaypt(const char *buff, const char *fieldDef, GpWaypt *wp)
{
	char data[strlen(buff) + 1];
	char order[strlen(fieldDef) + 1];
	int position = 1, pos = 0, spot;	// used for the data

	/* some flags */
	int id = false;
	int lat = false;
	int lon = false;
	int test;

	/* copy the buff and fieldDef into new variables */
	strcpy(data, buff);
	strcpy(order, fieldDef);

	/* seperate stuff in the f line and convert to all lowercase */
	strToLower(order);
	seperateData(order);

	/* set the position up for reading data */
	while(isspace(data[position]))
		position++;

	memmove(data, data + position, sizeof(char) * (strlen(data) - position + 1));
	position = 0;

	/* check to make sure buff isnt blank */
	if(!verifyLine(data))
		return FIELD;

	/* check to make sure there is an f-line */
	if(!verifyLine(order))
		return NOFORM;

	/* for the token */
	char orderToken[strlen(order)];

	/* set some defaults */
	wp->symbol = malloc(sizeof(char) * 2);
	assert(wp->symbol);
	strcpy(wp->symbol, "");
	wp->textChoice = 'I';
	wp->textPlace = 2;
	wp->comment = malloc(sizeof(char) * 2);
	assert(wp->comment);
	strcpy(wp->comment, "");

	/* go through every element in the field definition line and
	   fill up the waypoint structure with the info */
	while(pos < strlen(order))
	{
		/* get one token at a time from the f line */
		spot = 0;
		while(order[pos] != '\t' && order[pos] != '\0')
			orderToken[spot++] = order[pos++];

		orderToken[spot] = '\0';
		pos++;

		/* get the id */
		if(strncmp(orderToken, "id", 2) == 0)
		{
			wp->ID = malloc((sizeof(char) * strlen(orderToken)) + 1);
			assert(wp->ID);

			/* get the id */
			int index;
			for(index = 0; index < strlen(orderToken); index++)
			{
				if(position >= strlen(data))
					wp->ID[index] = ' ';
				else
					wp->ID[index] = data[position++];
			}

			wp->ID[index] = '\0';

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;

			id = true;
		}

		/* get the latitude */
		else if(strcmp(orderToken, "latitude") == 0)
		{
			char dataToken[strlen(orderToken)];

			/* get the latitude */
			int index = 0;
			while(!isspace(data[position]) && data[position] != '\0')
				dataToken[index++] = data[position++];

			dataToken[index] = '\0';

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;

			/* make sure there is no commas */
			if(strchr(dataToken, ','))
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			/* convert the number to +, - instead of N, S */
			if(dataToken[0] == 'N' || dataToken[0] == 'n')
				dataToken[0] = '+';
			else if(dataToken[0] == 'S' || dataToken[0] == 's')
				dataToken[0] = '-';
		
			test = sscanf(dataToken, "%lf", &wp->coord.lat);
			if(test != 1)
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			/* latitiude must be between -90 and 90 */
			if(wp->coord.lat > 90 || wp->coord.lat < -90)
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			lat = true;
		}

		/* get the longitude */
		else if(strcmp(orderToken, "longitude") == 0)
		{
			char dataToken[strlen(orderToken)];

			/* get the latitude */
			int index = 0;
			while(!isspace(data[position]) && data[position] != '\0')
				dataToken[index++] = data[position++];

			dataToken[index] = '\0';

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;


			/* make sure there is no commas */
			if(strchr(dataToken, ','))
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			/* convert the number to +, - instead of E, W */
			if(dataToken[0] == 'E' || dataToken[0] == 'e')
				dataToken[0] = '+';
			else if(dataToken[0] == 'W' || dataToken[0] == 'w')
				dataToken[0] = '-';
		
			test = sscanf(dataToken, "%lf", &wp->coord.lon);
			if(test != 1)
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			/* longitude must be between -180 and 180 */
			if(wp->coord.lon > 180 || wp->coord.lon < -180)
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			lon = true;
		}

		/* get the symbol */
		else if(strncmp(orderToken, "symbol", 6) == 0)
		{
			wp->symbol = realloc(wp->symbol, (sizeof(char) * strlen(orderToken)) + 1);
			assert(wp->symbol);

			/* get the symbol */
			int index;
			for(index = 0; index < strlen(orderToken); index++)
			{
				if(position >= strlen(data))
					wp->symbol[index] = ' ';
				else
					wp->symbol[index] = data[position++];
			}

			wp->symbol[index] = '\0';

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;
		}
	
		/* get the text choice */
		else if(strcmp(orderToken, "t") == 0)
		{
			char text = toupper(data[position++]);	// get the letter

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;

			/* must be a valid choice */
			if(text != '-' && text != 'I' && text != 'C' && text != '&' && text != '+' && text != '^')
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}

			wp->textChoice = text;
		}

		/* get the text place */
		else if(strcmp(orderToken, "o") == 0)
		{
			char dataToken[strlen(orderToken)];

			/* get the latitude */
			int index = 0;
			while(!isspace(data[position]) && data[position] != '\0')
				dataToken[index++] = data[position++];

			dataToken[index] = '\0';

			/* go to the beginning of the next word */
			while(isspace(data[position]))
				position++;

			strToUpper(dataToken);	// get the direction

			/* must be a valid direction, #s correspond to directions */
			if(strcmp(dataToken, "N") == 0)
				wp->textPlace = 0;
			else if(strcmp(dataToken, "NE") == 0)
				wp->textPlace = 1;
			else if(strcmp(dataToken, "E") == 0)
				wp->textPlace = 2;
			else if(strcmp(dataToken, "SE") == 0)
				wp->textPlace = 3;
			else if(strcmp(dataToken, "S") == 0)
				wp->textPlace = 4;
			else if(strcmp(dataToken, "SW") == 0)
				wp->textPlace = 5;
			else if(strcmp(dataToken, "W") == 0)
				wp->textPlace = 6;
			else if(strcmp(dataToken, "NW") == 0)
				wp->textPlace = 7;
			else
			{
				free(wp->symbol);
				free(wp->comment);
				return VALUE;
			}
		}

		/* get the comment */
		else if(strcmp(orderToken, "comment") == 0)
		{
			if(strlen(data) > position)
			{
				wp->comment = realloc(wp->comment, sizeof(char) * (strlen(data) - position) + 1);
				assert(wp->comment);
	
				int index = 0;
				while(data[position] != '\0')
					wp->comment[index++] = data[position++];
	
				wp->comment[index] = '\0';
			}
		}

		/* if an alt field is encountered */
		else if(strncmp(orderToken, "alt", 3) == 0)
		{
			/* skip this field */
			while(!isspace(data[position]) && data[position] != '\0')
				position++;

			while(isspace(data[position]))
				position++;
		}

		/* if an unwanted field is encountered */
		else
		{
			free(wp->symbol);
			free(wp->comment);
			return FIELD;
		}
	}

	/* make sure everything was read in from the W line */
	if(position < strlen(data))
	{
		free(wp->symbol);
		free(wp->comment);
		return FIELD;	
	}

	/* make sure all required fields were read in */
	if(!id || !lat || !lon)
	{
		free(wp->symbol);
		free(wp->comment);
		return FIELD;
	}

	return OK;
}

/* This function will scan in a Route
   and store them into a structure pointed to
   in an array of pointers */
GpError scanGpRoute(const char *buff, GpRoute *rp)
{
	char data[strlen(buff)];
	strcpy(data, buff);

	/* seperate the line into tokens */
	seperateData(data);
	char dataToken[strlen(data)];

	/* verify the buff is not blank */
	if(!verifyLine(data))
		return FIELD;

	/* set some defaults */
	rp->comment = malloc(sizeof(char) * 1);
	assert(rp->comment);
	strcpy(rp->comment, "");

	/* get the route number */
	strToken(dataToken, '\t', data);	
	int test = sscanf(dataToken, "%d", &rp->number);
	
	/* make sure a number (not negative was read in) */
	if(test != 1 || rp->number < 0)
	{
		free(rp->comment);
		return VALUE;	
	}

	/* get the comment */
	if(strcmp(data, "") != 0)
	{
		rp->comment = realloc(rp->comment, sizeof(char) * (strlen(data) + 1));
		assert(rp->comment);

		int index = 0;
		while(data[index] != '\0')
		{
			if(isspace(data[index]))
				data[index] = ' ';

			index++;
		}

		strcpy(rp->comment, data);
	}

	return OK;
}

/* This function scans in a legpoint that
   corresponds to a given route */
GpError scanGpLeg(const char *buff, const char *fieldDef, const GpWaypt *wp, const int nwp, GpRoute *rp)
{
	/* copy over the line and field def */
	char order[strlen(buff)];
	strcpy(order, fieldDef);
	
	char data[strlen(buff)];
	strcpy(data, buff);
	
	/* seperate stuff in the f line and convert to all lowercase */
	strToLower(order);
	seperateData(order);
	
	/* point spot to the first token' */
	int spot = 1;
	while(isspace(data[spot]))
		spot++;
	
	memmove(data, data + spot, sizeof(char) * (strlen(data) - spot + 1));
	spot = 0;
	
	/* check to make sure buff isnt blank */
	if(!verifyLine(data))
		return FIELD;

	/* check to make sure there is an f-line */
	if(!verifyLine(order))
		return NOFORM;
	
	/* set up some variables */
	int id = false, found = false;
	char orderToken[strlen(order)];
	char dataToken[strlen(data)];
	
	while(strcmp(order, "") != 0)
	{
		strToken(orderToken, '\t', order);
		
		if(strncmp(orderToken, "id", 2) == 0)
		{
			int index;
			for(index = 0; index < strlen(orderToken); index++)
			{
				if(spot >= strlen(data))
					dataToken[index] = ' ';
				else
					dataToken[index] = data[spot++];
			}
			dataToken[index] = '\0';
			
			/* find the waypoint with the same id as in the w line */
			for(index = 0; index < nwp; index++)
			{
				if(strcmp(dataToken, wp[index].ID) == 0)
				{
					rp->leg[rp->npoints - 1] = index;
					found = true;
					break;
				}
			}
			
			id = true;
		}
		
		/* ignore an unrecognized field */
		else
		{
			while(!isspace(data[spot]) && data[spot] != '\n')
				spot++;
			
			while(isspace(data[spot]))
				spot++;
		}
		
	}
	
	/* make sure ID is contained in the field def */
	if(!id)
		return FIELD;

	/* if the leg point isnt found in the array of waypoints */
	if(!found)
		return UNKWPT;

	return OK;
}

/* This function will scan in Track Points
   and store them into a contiguous array */
GpError scanGpTrkpt(const char *buff, const char *fieldDef, const char *dateFormat, GpTrkpt *tp)
{
	char data[strlen(buff)];
	char order[strlen(fieldDef)];

	/* some flags */
	int lat = false;
	int lon = false;
	int duration = false;
	int dist = false;
	int speed = false;
	int dateFlag = false;
	int timeFlag = false;
	int test;

	/* copy the buff and fieldDef into new variables */
	strcpy(data, buff);
	seperateData(data);
	strcpy(order, fieldDef);
	strToLower(order);
	seperateData(order);

	/* check to make sure buff isnt blank */
	if(!verifyLine(data))
		return FIELD;

	/* check to make sure there is an f-line */
	if(!verifyLine(order))
		return NOFORM;

	/* for the tokens */
	char dataToken[strlen(data)];
	char orderToken[strlen(order)];

	/* time variables */
	char *date = malloc(sizeof(char));
	assert(date);
	char *time = malloc(sizeof(char));
	assert(time);

	/* go through every element in the field definition line and
	   fill up the trackpoint structure with the info */
	while(strcmp(order, "") != 0)
	{
		/* get one token at a time from both the f and w line */
		strToken(orderToken, '\t', order);	// F line
		strToken(dataToken, '\t', data);	// W line
		
		/* get the latitude */
		if(strcmp(orderToken, "latitude") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			if(strchr(dataToken, ','))
			{
				free(date);
				free(time);
				return VALUE;
			}

			if(dataToken[0] == 'N' || dataToken[0] == 'n')
				dataToken[0] = '+';
			else if(dataToken[0] == 'S' || dataToken[0] == 's')
				dataToken[0] = '-';
		
			test = sscanf(dataToken, "%lf", &tp->coord.lat);
			lat = true;
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}

			if(tp->coord.lat > 90 || tp->coord.lat < -90)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
		}

		/* get the longitude */
		else if(strcmp(orderToken, "longitude") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			if(strchr(dataToken, ','))
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}

			if(dataToken[0] == 'E' || dataToken[0] == 'e')
				dataToken[0] = '+';
			else if(dataToken[0] == 'W' || dataToken[0] == 'w')
				dataToken[0] = '-';
		
			test = sscanf(dataToken, "%lf", &tp->coord.lon);
			lon = true;
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}

			if(tp->coord.lon > 180 || tp->coord.lon < -180)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
		}

		/* get the date */
		else if(strcmp(orderToken, "date") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			date = realloc(date, sizeof(char) * (strlen(dataToken) + 1));
			assert(date);
			strcpy(date, dataToken);
			
			dateFlag = true;
		}

		/* get the time */
		else if(strcmp(orderToken, "time") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			time = realloc(time, sizeof(char) * strlen(dataToken) + 1);
			assert(time);
			strcpy(time, dataToken);
			
			timeFlag = true;
		}

		/* get the comment flag and comment(if flag = 1) */
		else if(strcmp(orderToken, "s") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			int flag;
			test = sscanf(dataToken, "%d", &flag);
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}

			/* if there is no comment */
			if(flag == 0)
			{
				tp->segFlag = flag;
				tp->comment = NULL;
			}

			/* get the comment */
			else if(flag == 1)
			{
				tp->comment = malloc(sizeof(char) * strlen(data) + 1);
				assert(tp->comment);
				tp->segFlag = flag;

				/* get the comment */
				int index;
				for(index = 0; index < strlen(data); index++)
				{
					if(data[index] != '\t')
						tp->comment[index] = data[index];
					else
						tp->comment[index] = ' ';
				}
				tp->comment[index] = '\0';

				/* theses flags are no longer needed */
				duration = true;
				dist = true;
				speed = true;

				break;	// get out of the while loop(rest of line is comment)
			}
			else
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
		}

		/* get the duration (hours:minutes:seconds) */
		else if(strcmp(orderToken, "duration") == 0)
		{
			long hour, min, sec;
			
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			/* make sure : is in string */
			if(!strchr(dataToken, ':'))
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}	
			
			test = sscanf(dataToken, "%ld:%ld:%ld", &hour, &min, &sec);
			if(test != 3)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
			
			tp->duration = (hour * 60 * 60) + (min * 60) + sec;
			duration = true;
		}

		/* get the duration (seconds) */
		else if(strcmp(orderToken, "seconds") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			test = sscanf(dataToken, "%ld", &tp->duration);
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
			
			duration = true;
		}

		/* get the distance */
		else if(strcmp(orderToken, "ft") == 0 || strcmp(orderToken, "nm") == 0 || strcmp(orderToken, "km") == 0 || strcmp(orderToken, "m") == 0 || strcmp(orderToken, "miles") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			test = sscanf(dataToken, "%lf", &tp->dist);
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}

			dist = true;
		}

		/* get the speed */
		else if(strcmp(orderToken, "ft/s") == 0 || strcmp(orderToken, "knots") == 0 || strcmp(orderToken, "km/h") == 0 || strcmp(orderToken, "m/s") == 0 || strcmp(orderToken, "mph") == 0)
		{
			/* check to see if there is a value */
			if(strcmp(dataToken, "") == 0)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return FIELD;
			}
			
			test = sscanf(dataToken, "%f", &tp->speed);
			if(test != 1)
			{
				free(date);
				free(time);
				if(tp->segFlag == 1)
					free(tp->comment);
				
				return VALUE;
			}
			
			speed = true;
		}
		
		/* if an unexpected field is encountered */
		else if(strncmp(orderToken, "alt", 3) != 0)
		{
			free(time);
			free(date);
			if(tp->segFlag == 1)
				free(tp->comment);

			return FIELD;
		}
	}
	
	/* make sure there are no more data fields */
	if(strcmp(data, "") != 0 && tp->segFlag != 1)
	{
		free(time);
		free(date);
		if(tp->segFlag == 1)
			free(tp->comment);

		return FIELD;
	}

	/* make sure all required numerical fields were read in */
	if(!lat || !lon || !duration || !dist || !speed)
	{
		free(time);
		free(date);
		if(tp->segFlag == 1)
			free(tp->comment);

		return FIELD;
	}

	/* make sure both a date and time have been read in */
	if(!dateFlag || !timeFlag)
	{
		free(time);
		free(date);
		if(tp->segFlag == 1)
			free(tp->comment);

		return FIELD;
	}


	/* get everything ready to make the time_t */
	char *fullDateFormat = malloc(sizeof(char) * 18);
	assert(fullDateFormat);
	char *fullDate = malloc(sizeof(char) * (strlen(date) + strlen(time) + 2));
	assert(fullDate);

	strcpy(fullDateFormat, dateFormat);
	strcat(fullDateFormat, " %H:%M:%S");

	strcpy(fullDate, date);
	strcat(fullDate, " ");
	strcat(fullDate, time);

	/* make the proper time_t type */
	struct tm timeStruct;
	timeStruct.tm_isdst = -1;
	strptime(fullDate, fullDateFormat, &timeStruct);
	tp->dateTime = mktime(&timeStruct);
	
	/* free the strings used */
	free(time);
	free(date);
	free(fullDateFormat);
	free(fullDate);
	
	return OK;
}

/* This function frees all the structures inside filep */
void freeGpFile(GpFile *filep)
{
	/* free the stuff in waypoints */
	for(int index = 0; index < filep->nwaypts; index++)
	{
		free(filep->waypt[index].ID);
		free(filep->waypt[index].symbol);
		free(filep->waypt[index].comment);
	}
	
	/* free the stuff in routes */
	for(int index = 0; index < filep->nroutes; index++)
	{
		free(filep->route[index]->comment);
		free(filep->route[index]);
	}

	/* free the stuff in trackpoints */
	for(int index = 0; index < filep->ntrkpts; index++)
	{
		if(filep->trkpt[index].segFlag == true)
			free(filep->trkpt[index].comment);
	}
	/* free the rest of the stuff */
	free(filep->dateFormat);
	free(filep->waypt);
	free(filep->route);
	free(filep->trkpt);
}

/* This functions takes in a GpFile structure containing all the relative
   information in the gps file and a file pointer (can be pointed to stdout)
   and using these inputs writes all the information out to a file in a readable format 
   input: file pointer to write out to
   output: the number of lines written (0 indicates an error) */
int writeGpFile(FILE *const gpf, const GpFile *filep)
{
	int line = 0;
	
	/* some variables used multiple times */
	struct tm temp;
	temp.tm_sec = 0;
	temp.tm_min = 0;
	temp.tm_hour = 0;
	temp.tm_mon = 0;
	temp.tm_mday = 1;
	temp.tm_year = 99;
	char fullDate[21], format[18], date[12], time[9];
	strcpy(format, filep->dateFormat);
	strcat(format, " %H:%M:%S");

	/* print the H, I, S, M and U lines */
	fprintf(gpf, "H  SOFTWARE NAME & VERSION\n");
	fprintf(gpf, "I  GPSU 4.30 01 David's Version\n\n");
	
	char outDate[12];
	char value[strlen(filep->dateFormat)];
	char token[strlen(filep->dateFormat)];
	
	strcpy(outDate, "");
	strcpy(value, filep->dateFormat);
	while(strcmp(value, "") != 0)
	{
		strToken(token, '/', value);	// get all the tokens (everything before and after the '/'s)

		if(strcmp(token, "%d") == 0)
			strcat(outDate, "dd");
		else if(strcmp(token, "%m") == 0)
			strcat(outDate, "mm");
		else if(strcmp(token, "%B") == 0)
			strcat(outDate, "mmm");
		else if(strcmp(token, "%y") == 0)
			strcat(outDate, "yy");
		else if(strcmp(token, "%Y") == 0)
			strcat(outDate, "yyyy");

		/* add the / if need be */
		if(strcmp(value, "") != 0)
			strcat(outDate, "/");
	}

	fprintf(gpf, "S DateFormat=%s\n", outDate);
	fprintf(gpf, "S Timezone=%d:00\n", filep->timeZone);
	fprintf(gpf, "S Units=%c,%c\n\n", filep->unitHorz, filep->unitTime);
	
	fprintf(gpf, "H R DATUM\n");
	fprintf(gpf, "M E               WGS 84 100  0.0000000E+00  0.0000000E+00 0 0 0\n\n");
	
	fprintf(gpf, "H  COORDINATE SYSTEM\n");
	fprintf(gpf, "U  LAT LON DEG\n\n");
	
	line = 13;
	
	/* determine the F line for the waypoints and print it out */
	int symbol = false, comment = false;
	if(filep->nwaypts > 0)
	{
		if(strcmp(filep->waypt[0].symbol, "") != 0)
			symbol = true;
		
		for(int index = 0; index < filep->nwaypts; index++)
		{
			if(strcmp(filep->waypt[index].comment, "") != 0)
			{
				comment = true;
				break;
			}
		}
		
		fprintf(gpf, "F ID");
		for(int i = 2; i < strlen(filep->waypt[0].ID); i++)
			fprintf(gpf, "-");
		
		fprintf(gpf, " Latitude    Longitude    ");
		
		if(symbol)
		{
			fprintf(gpf, "Symbol");
			for(int s = 6; s < strlen(filep->waypt[0].symbol); s++)
				fprintf(gpf, "-");
			
			fprintf(gpf, " ");
		}
		
		fprintf(gpf, "T O  ");
		
		if(comment)
			fprintf(gpf, "Comment");
		
		fprintf(gpf, "\n");
		line ++;
	}
	
	/* print out the W lines */
	for(int index = 0; index < filep->nwaypts; index++)
	{
		char lat, lon;
		double latitude, longitude;
		
		latitude = filep->waypt[index].coord.lat;
		longitude = filep->waypt[index].coord.lon;
		
		/* convert the latitude */
		if(latitude > 0)
			lat = 'N';
		else
		{
			lat = 'S';
			latitude = latitude * -1;
		}
		
		/* convert the longitude */
		if(longitude > 0)
			lat = 'E';
		else
		{
			lon = 'W';
			longitude = longitude * -1;
		}
			
		
		fprintf(gpf, "W %s %c%-9lf  %c%-10lf  ", filep->waypt[index].ID, lat, latitude, lon, longitude);
		
		if(symbol)
			fprintf(gpf, "%s ", filep->waypt[index].symbol);
		
		fprintf(gpf, "%c ", filep->waypt[index].textChoice);
		
		if(filep->waypt[index].textPlace == 0)
			fprintf(gpf, "N  ");
		else if(filep->waypt[index].textPlace == 1)
			fprintf(gpf, "NE ");
		else if(filep->waypt[index].textPlace == 2)
			fprintf(gpf, "E  ");
		else if(filep->waypt[index].textPlace == 3)
			fprintf(gpf, "SE ");
		else if(filep->waypt[index].textPlace == 4)
			fprintf(gpf, "S  ");
		else if(filep->waypt[index].textPlace == 5)
			fprintf(gpf, "SW ");
		else if(filep->waypt[index].textPlace == 6)
			fprintf(gpf, "W  ");
		else if(filep->waypt[index].textPlace == 7)
			fprintf(gpf, "NW ");
		
		if(comment)
			fprintf(gpf, "%s", filep->waypt[index].comment);
		
		fprintf(gpf, "\n");
		line++;
	}
	
	if(filep->nwaypts > 0)
	{
		fprintf(gpf, "\n");
		line++;	
	}
	
	/* print out the r, f, and w lines */
	for(int index = 0; index < filep->nroutes; index++)
	{		
		/* the R line */
		fprintf(gpf, "R %d %s\n", filep->route[index]->number, filep->route[index]->comment);
		
		/* the F line */
		fprintf(gpf, "F ID");
		for(int i = 2; i < strlen(filep->waypt[0].ID); i++)
			fprintf(gpf, "-");
			
		fprintf(gpf, "\n");
			
		/* the legs (W lines) */
		for(int legno = 0; legno < filep->route[index]->npoints; legno++)
		{
			fprintf(gpf, "W %s\n", filep->waypt[filep->route[index]->leg[legno]].ID);
			line++;
		}
			
		fprintf(gpf, "\n");
		line += 3;
	}
	
	/* print out the H lines */
	GpTrack *track;
	int tracks = getGpTracks(filep, &track);
	
	if(tracks > 0)
	{
		fprintf(gpf, "H Track     Pnts. Date               Time     StopTime Duration        ");
		if(filep->unitHorz == 'M')
			fprintf(gpf, " m     m/s\n");
		else if(filep->unitHorz == 'K')
			fprintf(gpf, "km    km/h\n");
		else if(filep->unitHorz == 'F')
			fprintf(gpf, " f     f/s\n");
		else if(filep->unitHorz == 'N')
			fprintf(gpf, "nm    nm/h\n");
		else if(filep->unitHorz == 'S')
			fprintf(gpf, "sm    sm/h\n");
		
		line++;
		
		/* print out the individual track info */	
		for(int index = 0; index < tracks; index++)
		{
			/* the track and pnts. */
			fprintf(gpf, "H %5d     ", track[index].seqno);
			
			if(index + 1 < tracks)
				fprintf(gpf, "%5d ", track[index + 1].seqno - track[index].seqno - 1);
			else
				fprintf(gpf, "%5d ", filep->ntrkpts - track[index].seqno);
			
			/* the date and time */
			localtime_r(&track[index].startTrk, &temp);
			strftime(fullDate, sizeof(char) * 21, format, &temp);
			strToken(date, ' ', fullDate);
			strToken(time, ' ', fullDate);
			fprintf(gpf, "%-18s %-8s ", date, time);
			
			/* the end time */
			localtime_r(&track[index].endTrk, &temp);
			strftime(fullDate, sizeof(char) * 21, format, &temp);
			strToken(date, ' ', fullDate);
			strToken(time, ' ', fullDate);
			fprintf(gpf, "%-8s ", time);
			
			/* the duration */
			long dur = track[index].duration;
   			int hour, min, sec;
			
			hour = dur / 3600;
			temp.tm_hour = hour;
			
			dur = dur - (3600 * hour);
			min = dur / 60;
			temp.tm_min = min;
			
			sec = dur - (60 * min);
			temp.tm_sec = sec;
			
			strftime(fullDate, sizeof(char) * 21, format, &temp);
			strToken(date, ' ', fullDate);
			strToken(time, ' ', fullDate);
			fprintf(gpf, "%-8s ", time);
			
			/* print off the distance and the speed */
			fprintf(gpf, "%9.3lf %7.1f\n", track[index].dist, track[index].speed);
			line++;
		}
		
		fprintf(gpf, "\n");
		line++;
		
		free(track);
	}
	
	/* print off the F line for the trkpts */
	if(filep->ntrkpts > 0)
	{
		fprintf(gpf, "F Latitude    Longitude    Date               Time     S Duration          ");
		if(filep->unitHorz == 'M')
			fprintf(gpf, " m     m/s\n");
		else if(filep->unitHorz == 'K')
			fprintf(gpf, "km    km/h\n");
		else if(filep->unitHorz == 'F')
			fprintf(gpf, " f     f/s\n");
		else if(filep->unitHorz == 'N')
			fprintf(gpf, "nm    nm/h\n");
		else if(filep->unitHorz == 'S')
			fprintf(gpf, "sm    sm/h\n");
		
		line++;
	}
	
	/* print off the T lines */
	for(int index = 0; index < filep->ntrkpts; index++)
	{
		char lat, lon;
		double latitude, longitude;
		
		latitude = filep->trkpt[index].coord.lat;
		longitude = filep->trkpt[index].coord.lon;
		
		/* convert the latitude */
		if(latitude > 0)
			lat = 'N';
		else
		{
			lat = 'S';
			latitude = latitude * -1;
		}
		
		/* convert the longitude */
		if(longitude > 0)
			lat = 'E';
		else
		{
			lon = 'W';
			longitude = longitude * -1;
		}
			
		
		fprintf(gpf, "T %c%-9lf  %c%-10lf  ", lat, latitude, lon, longitude);
		
		/* print out the date and time */
		localtime_r(&filep->trkpt[index].dateTime, &temp);
		strftime(fullDate, sizeof(char) * 21, format, &temp);
		strToken(date, ' ', fullDate);
		strToken(time, ' ', fullDate);
		fprintf(gpf, "%-18s %-8s ", date, time);
		
		/* print out the seg flag */
		if(filep->trkpt[index].segFlag)
			fprintf(gpf, "1 %s\n", filep->trkpt[index].comment);
		else
		{
			fprintf(gpf, "0 ");
		
			/* the duration */
			long dur = filep->trkpt[index].duration;
			int hour, min, sec;
				
			hour = dur / 3600;
			temp.tm_hour = hour;
				
			dur = dur - (3600 * hour);
			min = dur / 60;
			temp.tm_min = min;
				
			sec = dur - (60 * min);
			temp.tm_sec = sec;

			strftime(fullDate, sizeof(char) * 21, format, &temp);
			strToken(date, ' ', fullDate);
			strToken(time, ' ', fullDate);
			fprintf(gpf, "%-8s   ", time);
				
			/* print off the distance and the speed */
			fprintf(gpf, "%9.3lf %7.1f\n", filep->trkpt[index].dist, filep->trkpt[index].speed);
		}
		
		line++;
	}
	
	/* make sure there isn't an IOERR */
	if(ferror(gpf))
		return 0;

	return line;
}

/* This function gets the information about the tracks */
int getGpTracks(const GpFile *filep, GpTrack **tp)
{
	/* make sure there are trackpoints */
	if(filep->ntrkpts == 0)
	{
		*tp = NULL;
		return 0;
	}
	
	/* initialize the array */
	*tp = NULL;
	int numTracks = 0;

	time_t tempEnd;
	double tempDist;
	long tempDuration;
	double north = -90, east = -180, south = 90, west = 180;
	
	for(int index = 0; index < filep->ntrkpts; index++)
	{
		if(filep->trkpt[index].segFlag == true)
		{
			/* add the "temporary" info */
			if(numTracks != 0)
			{
				(*tp)[numTracks - 1].endTrk = tempEnd;
				(*tp)[numTracks - 1].dist = tempDist;
				(*tp)[numTracks - 1].duration = tempDuration;

				/* calculate speed */
				(*tp)[numTracks - 1].speed = (float)(tempDist / (float)(tempDuration / 3600.0));

				/* store the corners */
				(*tp)[numTracks - 1].NEcorner.lat = north;
				(*tp)[numTracks - 1].NEcorner.lon = east;

				(*tp)[numTracks - 1].SWcorner.lat = south;
				(*tp)[numTracks - 1].SWcorner.lon = west;

				/* calculate the average */
				(*tp)[numTracks - 1].meanCoord.lat = (north + south) / (double)2;
				(*tp)[numTracks - 1].meanCoord.lon = (east + west) / (double)2;
				
				/* reset the direction variables */
				north = -90;
				east = -180;
				south = 90;
				west = 180;
			}

			/* increment the counter and expand the array */
			numTracks++;
			*tp = realloc(*tp, sizeof(GpTrack) * numTracks);
			assert(*tp);

			/* set a few things */
			(*tp)[numTracks - 1].seqno = index + 1;
			(*tp)[numTracks - 1].startTrk = filep->trkpt[index].dateTime;
		}
		else
		{
			tempEnd = filep->trkpt[index].dateTime;
			tempDist = filep->trkpt[index].dist;
			tempDuration = filep->trkpt[index].duration;
		}

		/* check for the most extreme latitudes */
		if(filep->trkpt[index].coord.lat > north)
			north = filep->trkpt[index].coord.lat;

		if(filep->trkpt[index].coord.lat < south)
			south = filep->trkpt[index].coord.lat;

		if(filep->trkpt[index].coord.lon > east)
			east = filep->trkpt[index].coord.lon;

		if(filep->trkpt[index].coord.lon < west)
			west = filep->trkpt[index].coord.lon;
	}

	/* add the "temporary" info */
	(*tp)[numTracks - 1].endTrk = tempEnd;
	(*tp)[numTracks - 1].dist = tempDist;
	(*tp)[numTracks - 1].duration = tempDuration;

	/* calculate speed */
	(*tp)[numTracks - 1].speed = (float)(tempDist / (float)(tempDuration / 3600.0));

	/* store the corners */
	(*tp)[numTracks - 1].NEcorner.lat = north;
	(*tp)[numTracks - 1].NEcorner.lon = east;

	(*tp)[numTracks - 1].SWcorner.lat = south;
	(*tp)[numTracks - 1].SWcorner.lon = west;

	/* calculate the average */
	(*tp)[numTracks - 1].meanCoord.lat = (north + south) / (double)2;
	(*tp)[numTracks - 1].meanCoord.lon = (east + west) / (double)2;

	return numTracks;
}
/* GENERAL AND STRING FUNCTIONS FROM HERE DOWN */

/* converts a string to all lowercase
   input: a string
   output: string in all lowercase */
void strToLower(char *line)
{
	for(int index = 0; index < strlen(line); index++)
		line[index] = tolower(line[index]);
}

/* converts a string to all uppercase
   input: a string
   output: string in all lowercase */
void strToUpper(char *line)
{
	for(int index = 0; index < strlen(line); index++)
		line[index] = toupper(line[index]);
}

/* removes all whitespace from a string
   input: a string
   output: string with no spaces */
void removeWhite(char *line)
{
	int place = 0;
	for(int index = 0; index < strlen(line); index++)
	{
		if(!isspace(line[index]))
			line[place++] = line[index];
	}
	line[place] = '\0';
}

/* removes a character from a given string
   input: a string
   output: string without the specified character */
void removeChar(char remove, char *string)
{
	int place = 0;
	for(int index = 0; index < strlen(string); index++)
	{
		if(string[index] != remove)
			string[place++] = string[index];
	}
	string[place] = '\0';
}

/* breaks a string up into tokens (one at a time everytime this function is called)
   based on a seperating character 
   input: a string and the seperating character
   output: the string from the start to the first seperating character found,
	   if no seperating character found then the entire string will be returned */
void strToken(char *token, char seperator, char *string)
{
	/* goes through the string and adds chars until the seperator or EOL is reached */
	int index = 0;
	while(string[index] != seperator && string[index] != '\0')
	{
		token[index] = string[index];
		index++;
	}
	token[index] = '\0';

	/* adjusts the string pointer for the next token bad stuff */
	if(strchr(string, seperator) != NULL)
	{
		char *move = strchr(string, seperator) + 1;
		memmove(string, move, sizeof(char) * strlen(move) + 1);
	}
	else
		strcpy(string, "");
}

/* seperates data on a given line by
   removing all whitespace and inserts a tab '\t' in its place,
   also removes the 'type' letter associated with the line
   input: string being seperated
   output: string where the whitespace replaced by a single tab */
void seperateData(char *line)
{
	/* remove the initial f and spaces */
	int temp = 1;
	while(isspace(line[temp]))
		temp++;

	/* get the field defintion seperated by '\t's */
	int place = 0;
	for(int index = temp; index < strlen(line); index++)
	{
		if(!isspace(line[index]))
			line[place] = line[index];
		else
		{
			while(isspace(line[index + 1]))
				index++;

			line[place] = '\t';
		}

		place++;
	}
	line[place] = '\0';
}

/* verifies a line is not blank,
   input: a string being verified
   output: true if not blank otherwise false */
int verifyLine(char *line)
{
	for(int index = 0; index < strlen(line); index++)
	{
		if(!isspace(line[index]))
			return true;
	}

	return false;
}
