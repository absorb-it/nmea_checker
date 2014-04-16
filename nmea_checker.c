/*****************************************************************************
* 
* NMEA checksum validator
* 
* License: GPL
* Copyright (c) 2014 Rene Ejury
* 
* Description:
* 
* This file contains the NMEA checksum validator program
* 
* This program verifys the checksum of NMEA sentences, if ok it retransmits them
* 
* 
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* 
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
* 
* 
*****************************************************************************/

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

/* baudrate settings are defined in <asm/termbits.h>, which is
included by <termios.h> */
#define BAUDRATE B4800

#define version "0.2"

int open_input(char *input_dev) {
	int fd;
	struct termios newtio;
	printf("opening device '%s' for input\n", input_dev);

	fd = open(input_dev, O_RDWR | O_NOCTTY ); 
	if (fd <0) {perror(input_dev); exit(-1); }
	
	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	newtio.c_cc[VERASE]   = 0;     /* del */
	newtio.c_cc[VKILL]    = 0;     /* @ */
	newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 1;     /* blocking read until 1 character arrives */
	newtio.c_cc[VSWTC]    = 0;     /* '\0' */
	newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	newtio.c_cc[VEOL]     = 0;     /* '\0' */
	newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	newtio.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	return fd;
};

int open_output(char *output_dev) {
	int fd;
	struct termios newtio;
	printf("opening device '%s' for output\n", output_dev);

	fd = open(output_dev, O_RDWR | O_NOCTTY ); 
	if (fd <0) {perror(output_dev); exit(-1); }

	// unlock device to enable access for ptmx pseudoterminals
	grantpt(fd);
	unlockpt(fd);	
	if (ptsname(fd))
		fprintf(stderr, "\tslave output device: '%s'\n", ptsname(fd));
	
	bzero(&newtio, sizeof(newtio)); /* clear struct for new port settings */
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;
	newtio.c_cc[VINTR]    = 0;     /* Ctrl-c */ 
	newtio.c_cc[VQUIT]    = 0;     /* Ctrl-\ */
	newtio.c_cc[VERASE]   = 0;     /* del */
	newtio.c_cc[VKILL]    = 0;     /* @ */
	newtio.c_cc[VEOF]     = 4;     /* Ctrl-d */
	newtio.c_cc[VTIME]    = 0;     /* inter-character timer unused */
	newtio.c_cc[VMIN]     = 0;     /* blocking read until 1 character arrives */
	newtio.c_cc[VSWTC]    = 0;     /* '\0' */
	newtio.c_cc[VSTART]   = 0;     /* Ctrl-q */ 
	newtio.c_cc[VSTOP]    = 0;     /* Ctrl-s */
	newtio.c_cc[VSUSP]    = 0;     /* Ctrl-z */
	newtio.c_cc[VEOL]     = 0;     /* '\0' */
	newtio.c_cc[VREPRINT] = 0;     /* Ctrl-r */
	newtio.c_cc[VDISCARD] = 0;     /* Ctrl-u */
	newtio.c_cc[VWERASE]  = 0;     /* Ctrl-w */
	newtio.c_cc[VLNEXT]   = 0;     /* Ctrl-v */
	newtio.c_cc[VEOL2]    = 0;     /* '\0' */

	tcflush(fd, TCIFLUSH);
	tcsetattr(fd,TCSANOW,&newtio);
	return fd;
};

int calc_checksum(char *nmea) {
	char xor = 0;
	int i = 0, len_nmea = strlen(nmea);
	while (i < len_nmea) {
		xor ^= (char)(nmea[i++]);
	}
	return xor;
}

FILE *log_all = NULL, *log_ok = NULL, *log_wrong = NULL;
FILE *inp_file = NULL, *out_file = NULL;

void close_all_exit(int status) {
	if (inp_file)
		fclose(inp_file);
	if (out_file)
		fclose(out_file);
	if (log_all)
		fclose(log_all);
	if (log_ok)
		fclose(log_ok);
	if (log_wrong)
		fclose(log_wrong);
	exit(status);
}

void flush_open_files() {
	if (out_file)
		fflush(out_file);
	if (log_all)
		fflush(log_all);
	if (log_ok)
		fflush(log_ok);
	if (log_wrong)
		fflush(log_wrong);
}

int main(int argc, char *argv[])
{
	char *begin = NULL, *end = NULL;
	int inp = -1, output = -1, a, timestamp = -1;
	
	printf("NMEA Checker, version %s\n", version);
	printf ("------------------------------------------------------------------------\n");
	if ( argc < 2 || strcmp(argv[1],"-h") == 0 || strcmp(argv[1],"--help" ) == 0) {
		printf ("NMEA Checker reads Input from one serial line or file and verifies\n");
		printf (" the NMEA checksum. If ok, NMEA sentence will be transferred to\n");
		printf (" the Output device or file.\n");
		printf ("------------------------------------------------------------------------\n");
		printf ("usage: nmea_checker [-d] [-t] InputFile|InputDevice [OutputFile|OutputDevice]\n\n");
		printf ("       device can also be '/dev/ptmx' pseudoterminal\n");
		printf ("-d\tlog all the data to nmea_checker_[all|ok|wrong].log\n");
		printf ("-t\tinclude timestamp in log-data (but not in output file)\n\n");
		printf ("example: ./nmea_checker -d -t /dev/ttyUSB0 /dev/ttyUSB1\n\n");
		close_all_exit(EXIT_FAILURE);
	}
	for (a = 1; a < argc; a++) {
		if ( strcmp(argv[a], "-d") == 0 ) {
// 			printf("logging to nmea_checker_[all|ok|wrong].log\n");
			log_all = fopen("nmea_checker_all.log", "w");
			if (log_all == NULL)
				close_all_exit(EXIT_FAILURE);

			log_wrong = fopen("nmea_checker_wrong.log", "w");
			if (log_wrong == NULL)
				close_all_exit(EXIT_FAILURE);

			log_ok = fopen("nmea_checker_ok.log", "w");
			if (log_ok == NULL)
				close_all_exit(EXIT_FAILURE);
		}
		else if ( strcmp(argv[a], "-t") == 0 ) {
// 			printf("adding timestamp\n");
			timestamp = 1;
		}
		else {
			struct stat buf;
			if (stat(argv[a], &buf) != 0 || S_ISREG(buf.st_mode) || S_ISLNK(buf.st_mode)) {
				if (inp < 0 && !inp_file) {
					printf("opening file '%s' as input\n", argv[a]);
					inp_file = fopen(argv[a], "r");
					if (inp_file == NULL)
						close_all_exit(EXIT_FAILURE);
				} else {
					printf("opening file '%s' as output\n", argv[a]);
					out_file = fopen(argv[a], "w");
					if (out_file == NULL)
						close_all_exit(EXIT_FAILURE);
				}				
			}
			else if (S_ISCHR(buf.st_mode)) {
				if (inp < 0 && !inp_file) {
					inp = open_input(argv[a]);
					if (inp < 0) {
						printf( "failed to open input device\n");
						close_all_exit(EXIT_FAILURE);
					}
				} else {
					output = open_output(argv[a]);
					if (output < 0) {
						printf( "failed to open output device\n");
						close_all_exit(EXIT_FAILURE);
					}
				}				
			}
			else {
				printf("file or device '%s' not found. aborting.\n", argv[a]);
				close_all_exit(EXIT_FAILURE);
			}
		}
	}
	
	printf ("------------------------------------------------------------------------\n");
	
	while(1) {
		int res, pos = 0;
		char current_checksum, calculated_checksum;
		char buf[255] = {0};
		time_t t;
		struct tm *tmp;
		char timestr[200] = {0};
		
		// read all characters of one line (line ends with 0x0a)
		while (pos < 250) {
			if (inp_file) {
				buf[pos] = fgetc(inp_file);	// read from file
				if (buf[pos] == EOF)
					close_all_exit(0);
			} else {
				read(inp, buf+pos, 1);		// read from device
			}
			if (buf[pos] == 0x0a)
				break;
			if (buf[pos] != 0)
				pos++;
		}
		
		// if time is required, set string accordingly
		if (timestamp != -1) {
			t = time(NULL);
			tmp = localtime(&t);
			strftime(timestr, sizeof(timestr), "%F,%T,", tmp);
		}
		
		// process read NMEA line
		if (pos++ > 0) {
			buf[pos] = 0; 					// limit the string by the read length
			begin = strchr(buf, '$');		// find beginning of real nmea data
			end = strchr(buf, '*');			// find end of real nmea data
			
			if (end != NULL && begin != NULL) {
				*end = 0; 					// limit string to the 'real' nmea information
				current_checksum = strtol(end + 1, NULL, 16);
				calculated_checksum = calc_checksum(begin + 1);
				*end = '*'; 				// remove the limit
				
				if (current_checksum == calculated_checksum) {
					printf("  checksum is ok: %s%s", timestr, begin);
					if (output != -1)
						(output, begin, strlen(begin));
					if (out_file)
						fprintf(out_file, "%s", begin);
					if (log_ok)
						fprintf(log_ok, "%s%s", timestr, begin);	
				}
				else {
					printf("# checksum wrong: %s%s", timestr, begin);
					if (log_wrong)
						fprintf(log_wrong, "%s%s", timestr, begin);
				}
				
				if (log_all)
					fprintf(log_all, "%s%s", timestr, begin);
				flush_open_files();
			}
		}
	}
    close_all_exit(0);
}
