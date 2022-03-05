/*
 * dsmtk
 * Microcontroller Tool Kit for Dallas DS89C4X0
 *
 * Copyright (c) 2005 Aleksander Mazur
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <getopt.h>

#define	PROGRAM	"dsmtk"
#define	VERSION	"0.2 (20050507)"

static const char gpl_notice[] =
  PROGRAM " comes with ABSOLUTELY NO WARRANTY; for details see COPYING.\n"
  "This is free software, and you are welcome to redistribute it\n"
  "under conditions of GNU General Public License.\n";

#define	TERMINATOR	'\n'

/* configurable parameters */

const char *comport = "/dev/ttyS0";
int baudrate = 9600;
int timeout = 2;	/* in seconds */
int small_timeout = 50000;	/* in microseconds */

/* other variables */

/* saved terminal attributes */
struct termios saved_attributes;
/* serial tty file descriptor */
int fd;
/* grab file */
FILE *grabber;

/**************************************/

/* Command-line options parsing */
void get_options(int argc, char **argv)
{
	const char shortopt[] = "t:r:o:hv";
	const struct option longopt[] = {
		{ "help",    no_argument, NULL, 'h' },
		{ "version", no_argument, NULL, 'v' },
		{ "tty",	required_argument, NULL, 't' },
		{ "rate", required_argument, NULL, 'r' },
		{ "timeout", required_argument, NULL, 'o' },
		{ NULL, 0, NULL, 0 }
	};
	int c, i;

	opterr = 0;

	while ((c = getopt_long(argc, argv, shortopt, longopt, &i)) != -1)
		switch (c) {
			case 'v':
				puts(PROGRAM " " VERSION);
				exit(0);
			case 'h':
				puts(PROGRAM " " VERSION);
				puts(gpl_notice);
				printf("Usage:\t%s [options]\n", argv[0]);
				puts("Available options (with default values):\n"
					"\t-h | --help"	"\tdisplay this usage information\n"
					"\t-v | --version"	"\tdisplay version number\n"
					"\t-t | --tty"	"\tset destination tty (/dev/ttyS0)\n"
					"\t-r | --rate"	"\tset baudrate in bps (9600)\n"
					"\t-o | --timeout"	"\tset device response timeout in seconds (2)\n"
				);
				exit(0);
			case 't':
				comport = optarg;
				break;
			case 'r':
				sscanf(optarg, "%d", &baudrate);
				break;
			case 'o':
				sscanf(optarg, "%d", &timeout);
				break;
			case '?':
				break;
			default:
				abort();
		}
}

/**************************************/

void signal_term(int sig)
{
	fprintf(stderr, "Signal %d catched - exiting\n", sig);
	signal(sig, SIG_DFL);
	exit(0);
}

void close_grabber(void)
{
	if (grabber) {
		fclose(grabber);
		grabber = NULL;
		fputs("Wrote grab file\n", stderr);
	}
}

/* waits for input at fd for timeout1 seconds and timeout2 microseconds */
int wait_for_input(int fd, int timeout1, int timeout2)
{
	struct timeval tv = { timeout1, timeout2 };
	fd_set set;

	FD_ZERO(&set);
	FD_SET(fd, &set);
	switch (select(FD_SETSIZE, &set, NULL, NULL, &tv)) {
		case -1:
			perror("select");
			return 0;
		case 1:
			return 1;
		default:
			return 0;
	}
}

/* reads one character from fd (with error handling) */
int read_char(int fd)
{
	char znak;

	switch (read(fd, &znak, 1)) {
		case 0:
			return 0;
		case 1:
			return znak;
		default:
			perror("read");
			return 0;
	}
}

/* reads line from fd to buf, up to max characters */
int read_line(int fd, char *buf, int max)
{
	char *p = buf;
	char znak = 0;
	int timeout1 = timeout, timeout2 = 0;

	max--;
	while (max && znak != TERMINATOR) {
		for (;;) {
			znak = read_char(fd);
			if (znak) {
				if (p == buf && znak == '>') {
					timeout1 = 0;
					timeout2 = small_timeout;
				}
				*p++ = znak;
				max--;
				break;
			} else if (!wait_for_input(fd, timeout1, timeout2)) {
				znak = TERMINATOR;
				break;
			}
		}
	}
	*p = 0;
	return p - buf;
}

/* returns pointer to a message matching given response code received from device */
const char *decode_response(int c)
{
	typedef struct {
		int code;
		const char *message;
	} response_t;
	static const response_t responses[] = {
		{ 'A', "INVALID ADRESS IN INTEL HEX RECORD" },
		{ 'F', "FLASH CONTROLLER ERROR" },
		{ 'G', "GOOD RECORD" },
		{ 'H', "INVALID INTEL HEX RECORD FORMAT" },
		{ 'L', "INVALID INTEL HEX RECORD LENGTH" },
		{ 'P', "FAILURE TO WRITE ONES TO ZEROS DURING PROGRAMMING" },
		{ 'R', "INVALID INTEL HEX RECORD TYPE" },
		{ 'S', "INVALID CHECKSUM IN INTEL HEX RECORD" },
		{ 'V', "VERIFY ERROR" },
		{ -1, "unknown error" }
	};
	const response_t *p = responses;

	while (p->code != c && p->code != -1)
		p++;
	return p->message;
}

int load_verify(const char *command, const char *file)
{
	int r;
	FILE *f;

	if (!file) {
		fputs("This command requires an argument - name of a file to read hex data from.\n"
		"Type \"/help\" to see details.\n", stderr);
		return -1;
	}

	if (f = fopen(file, "r")) {
		int line = 0;
		char buf[512];

		write(fd, command, strlen(command));
		write(fd, "\r\n", 2);
		read_line(fd, buf, sizeof(buf));
		fputs(buf, stderr);
		r = 0;
		while (fgets(buf, sizeof(buf), f)) {
			line++;
			write(fd, buf, strlen(buf));
			write(fd, "\r\n", 2);
			if (wait_for_input(fd, timeout, 0)) {
				char znak = read_char(fd);
				if (znak != 'G') {
					buf[0] = znak;
					buf[1] = 0;
					if (wait_for_input(fd, 0, small_timeout))
						read_line(fd, buf + 1, sizeof(buf) - 1);
					fprintf(stderr, "%s(%d): %s [%s]\n", file, line, decode_response(znak), buf);
					r = 3;
					write(fd, "\r\n", 2);
					break;
				}
			} else {
				fputs("Device is not responding", stderr);
				r = 2;
				break;
			}
		}
		fclose(f);
	} else {
		perror(file);
		r = 1;
	}

	return r;
}

void grab(const char *file)
{
	close_grabber();

	if (file) {
		grabber = fopen(file, "w");
		if (grabber)
			fprintf(stderr, "Grabbing output to %s\n", file);
		else
			perror(file);
	}
}

void handle_command(const char *buf)
{
	static const char *builtins =
	"WRAPPERS:\n"
	"\n"
	"/load\t\tLoad flash memory (L)\n"
	"/load_blind\tLoad blind (LB)\n"
	"/load_encrypt\tLoad encryption vector (LE)\n"
	"/load_extern\tLoad external memory (LX)\n"
	"/verify\t\tVerify contents of flash memory (V)\n"
	"/verify_encrypt\tVerify encryption vector (VE)\n"
	"/verify_extern\tVerify external memory (VX)\n"
	"\n"
	"These built-in commands send asociated operation code (shown in parentheses)\n"
	"followed by the content of a standard Intel hex file given as an argument.\n"
	"This program doesn't validate the file itself.\n"
	"More information about operations actually performed can be found\n"
	"in section 15 of DS89C420's Ultra-High-Speed Flash Microcontroller\n"
	"User's Guide, which can be obtained at www.maxim-ic.com .\n"
	"\n"
	"OTHER:\n"
	"\n"
	"/grab\t\tStarts grabbing output to a given file. In case of no argument\n"
	"\t\tspecified, grabbing is cancelled.\n"
	"\n"
	"Any other commands (not beginning with a slash) are transmitted directly.\n";

	char *p = strchr(buf, ' ');
	if (p)
		*p++ = 0;

	if (!strcasecmp(buf, "load"))
		load_verify("L", p);
	else if (!strcasecmp(buf, "load_blind"))
		load_verify("LB", p);
	else if (!strcasecmp(buf, "load_encrypt"))
		load_verify("LE", p);
	else if (!strcasecmp(buf, "load_extern"))
		load_verify("LX", p);
	else if (!strcasecmp(buf, "verify"))
		load_verify("V", p);
	else if (!strcasecmp(buf, "verify_encrypt"))
		load_verify("VE", p);
	else if (!strcasecmp(buf, "verify_extern"))
		load_verify("VX", p);
	else if (!strcasecmp(buf, "grab"))
		grab(p);
	else if (!strcasecmp(buf, "help"))
		fputs(builtins, stderr);
	else
		fputs("Invalid command; try \"/help\"\n", stderr);
}

void reset_terminal(void)
{
	tcsetattr(fd, TCSANOW, &saved_attributes);
	close(fd);
}

static const int baudrate_conv[] = {
	0, B0,
	50, B50,
	75, B75,
	110, B110,
	134, B134,
	150, B150,
	200, B200,
	300, B300,
	600, B600,
	1200, B1200,
	1800, B1800,
	2400, B2400,
	4800, B4800,
	9600, B9600,
	19200, B19200,
	38400, B38400,
	57600, B57600,
	115200, B115200,
	230400, B230400,
	460800, B460800,
	-1, -1
};

int int2baudrate(int br)
{
	const int *baudrate;

	for (baudrate = baudrate_conv; *baudrate != -1; baudrate += 2)
		if (baudrate[0] == br)
			return baudrate[1];

	return 0;
}

int baudrate2int(int br)
{
	const int *baudrate;

	for (baudrate = baudrate_conv; *baudrate != -1; baudrate += 2)
		if (baudrate[1] == br)
			return baudrate[0];

	return 0;
}

int main(int argc, char **argv)
{
	const char *terminal;
	struct termios ts;
	int r;
	char buf[512];
	int response_pending;

	signal(SIGINT, signal_term);
	get_options(argc, argv);

	fd = open(comport, O_RDWR);
	if (fd < 0) {
		perror(comport);
		return 1;
	}

	if (!isatty(fd)) {
		fprintf(stderr, "%s is not a tty\n", comport);
		return 2;
	}

	terminal = ttyname(fd);
	if (terminal)
		fprintf(stderr, "Connecting to %s\n", terminal);

	if (tcgetattr(fd, &saved_attributes) < 0) {
		perror("tcgetattr");
		return 3;
	}
	atexit(reset_terminal);

	memcpy(&ts, &saved_attributes, sizeof(ts));
	ts.c_iflag &= ~(INPCK|IGNPAR|PARMRK|ISTRIP|IGNBRK|BRKINT|INLCR|IXON|IXOFF|IXANY);
	ts.c_iflag |= IGNCR|ICRNL;
	ts.c_oflag &= ~(OPOST);
	ts.c_cflag &= ~(CSTOPB|PARENB|PARODD|CSIZE);
	ts.c_cflag |= HUPCL|CREAD|CS8|CLOCAL;
	ts.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOPRT|ECHOK|ECHOKE|ECHONL);
	ts.c_cc[VMIN] = 0;
	ts.c_cc[VTIME] = 0;
	r = int2baudrate(baudrate);
	if (r) {
		if (cfsetospeed(&ts, r) < 0)
			perror("cfsetospeed");
		if (cfsetispeed(&ts, r) < 0)
			perror("cfsetispeed");
	}
	r = cfgetospeed(&ts);
	fprintf(stderr, "Connection speed %d bps\n", baudrate2int(r));

	if (tcsetattr(fd, TCSANOW, &ts) < 0) {
		perror("tcsetattr");
		return 4;
	}

	for (;;) {
		char znak;

		write(fd, "\r\n", 2);
		if (wait_for_input(fd, 0, 100000))
			break;
		putc('.', stderr);
	}

	fputs("\nConnection established\nType \"/help\" to show built-in commands\n", stderr);
	response_pending = 1;

	setbuf(stdout, NULL);
	atexit(close_grabber);

	do {
		struct timeval tv = { 0, small_timeout };
		struct timeval *ptv;
		fd_set set;

		FD_ZERO(&set);
		FD_SET(fd, &set);
		if (response_pending) {
			ptv = &tv;
		} else {
			FD_SET(STDIN_FILENO, &set);
			ptv = NULL;
		}

		switch (select(FD_SETSIZE, &set, NULL, NULL, ptv)) {
			case 0:
				response_pending = 0;
				break;
			case -1:
				perror("select");
				return 5;
			default:
				if (FD_ISSET(fd, &set)) {
					int c = read_char(fd);

					putchar(c);
					if (grabber)
						putc(c, grabber);
				} else {
					int len, command;

					fgets(buf, sizeof(buf) - 1, stdin);
					if (feof(stdin))
						break;
					buf[sizeof(buf) - 1] = 0;
					len = strlen(buf);
					command = buf[0] == '/';
					if (buf[len - 1] == '\n')
						if (command) {
							buf[len - 1] = 0;
						} else {
							buf[len - 1] = '\r';
							buf[len++] = '\n';
							buf[len] = '\0';
						}
					if (command) {
						handle_command(buf + 1);
					} else {
						write(fd, buf, len);
						response_pending = 1;
					}
				}
		}
	} while (buf[0] != '>' && !feof(stdin));

	putc('\n', stderr);
	return 0;
}
