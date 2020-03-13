/*
 * dtmf-dial 0.1
 * (C) 1998 Itai Nahshon (nahshon@actcom.co.il)
 *
 * Use and redistribution are subject to the GNU GENERAL PUBLIC LICENSE.
 */
#include <math.h>
#include <stdio.h>
#include <fcntl.h>
#include <malloc.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <linux/soundcard.h>

#define DEBUG(x)

void gen_costab(void);
void dial_digit(int c);
void silent(int msec);
void dial(int f1, int f2, int msec);

char *output = "/dev/dsp";
int bits        = 8;
int speed       = 8000;
int tone_time   = 100;
int silent_time = 50;
int sleep_time  = 500;
int volume      = 100;
int format	= AFMT_U8;
int use_audio   = 1;

#define BSIZE 4096

unsigned char *buf;
int bufsize = BSIZE;
int bufidx;

#define MINTABSIZE 2
#define MAXTABSIZE 65536
signed short *costab;
int tabsize = 256;

int dialed = 0;

int right  = 0;
int left   = 0;

int fd;

void
Usage(void) {
	fprintf(stderr, "usage: dial [options] number ...\n"
			" Valid options with their default values are:\n"
			"   Duration options:\n"
			"     --tone-time   100\n"
			"     --silent-time 50\n"
			"     --sleep-time  500\n"
			"   Audio output  options:\n"
			"     --output-dev  /dev/dsp\n"
			"     --use-audio   1\n"
			"     --bufsize     4096\n"
			"     --speed       8000\n"
			"     --bits        8\n"
			"   Audio generation options:\n"
			"     --table-size  256\n"
			"     --volume      100\n"
			"     --left        0\n"
			"     --right       0\n"
			);
			
	exit(1);
}

void
initialize_audiodev(void) {
	int speed_local = speed;
	int channels = 1;
	int diff;

	if(!use_audio)
		return;

	if(right || left)
		channels = 2;

	if(ioctl(fd, SNDCTL_DSP_CHANNELS, &channels)) {
		perror("ioctl(SNDCTL_DSP_CHANNELS)");
		exit(1);
	}

	if(ioctl(fd, SNDCTL_DSP_SETFMT, &format)) {
		perror("ioctl(SNDCTL_DSP_SPEED)");
		exit(1);
	}

	if(ioctl(fd, SNDCTL_DSP_SPEED, &speed_local)) {
		perror("ioctl(SNDCTL_DSP_SPEED)");
		exit(1);
	}

	diff = speed_local - speed;
	if(diff < 0)
		diff = -diff;
	if(diff > 500) {
		fprintf(stderr,
		   "Your sound card does not support the requested speed\n");
		exit(1);
	}
	if(diff != 0) {
		fprintf(stderr,
		   "Setting speed to %d\n", speed_local);
	}
	speed = speed_local;
}

void
getvalue(int *arg, int *index, int argc,
	 char **argv, int min, int max) {

	if (*index >= argc-1)
		Usage();

	*arg = atoi(argv[1+*index]);

	if(*arg < min || *arg > max) {
		fprintf(stderr, "Value for %s should be in the range %d..%d\n", 
				argv[*index]+2, min, max);
		exit(1);
	}
	++*index;
}

void
main(int argc, char **argv)
{
	char *cp;
	int i;

	for(i = 1; i < argc; i++) {
		if(argv[i][0] != '-' ||
		   argv[i][1] != '-')
			break;

		if(!strcmp(argv[i], "--table-size")) {
			getvalue(&tabsize, &i, argc, argv,
				 MINTABSIZE, MAXTABSIZE);
		}
		else if(!strcmp(argv[i], "--tone-time")) {
			getvalue(&tone_time, &i, argc, argv,
				 10, 10000);
		}
		else if(!strcmp(argv[i], "--sleep-time")) {
			getvalue(&sleep_time, &i, argc, argv,
				 10, 10000);
		}
		else if(!strcmp(argv[i], "--silent-time")) {
			getvalue(&silent_time, &i, argc, argv,
			         10, 10000);
		}
		else if(!strcmp(argv[i], "--sleep-time")) {
			getvalue(&sleep_time, &i, argc, argv,
				 10, 100000);
		}
		else if(!strcmp(argv[i], "--volume")) {
			getvalue(&volume, &i, argc, argv,
				 0, 100);
		}
		else if(!strcmp(argv[i], "--speed")) {
			getvalue(&speed, &i, argc, argv,
				 5000, 48000);
		}
		else if(!strcmp(argv[i], "--bits")) {
			getvalue(&bits, &i, argc, argv,
				 8, 16);
		}
		else if(!strcmp(argv[i], "--bufsize")) {
			getvalue(&bufsize, &i, argc, argv,
				 4, 65536);
		}
		else if(!strcmp(argv[i], "--use-audio")) {
			getvalue(&use_audio, &i, argc, argv,
				 0, 1);
		}
		else if(!strcmp(argv[i], "--right")) {
			getvalue(&right, &i, argc, argv,
				 0, 1);
		}
		else if(!strcmp(argv[i], "--left")) {
			getvalue(&left, &i, argc, argv,
				 0, 1);
		}
		else if(!strcmp(argv[i], "--output-dev")) {
			i++;
			if(i >= argc)
				Usage();
			output = argv[i];
		}
		else
			Usage();
	}

	if(i >= argc)
		Usage();

	if(!strcmp(output, "-"))
		fd = 1;		/* stdout */
	else {
		fd = open(output, O_CREAT|O_TRUNC|O_WRONLY, 0644);
		if(fd < 0) {
			perror(output);
			exit(1);
		}
	}

	switch(bits) {
		case 8:
			format = AFMT_U8;
			break;
		case 16:
			format = AFMT_S16_LE;
			break;
		default:
			fprintf(stderr, "Value for bits should be 8 or 16\n");
			exit(1);
	}

	initialize_audiodev();

	gen_costab();
	buf = malloc(bufsize);
	if(buf == NULL) {
		perror("malloc buf");
		exit(1);
	}

	bufidx = 0;
	for(; i < argc; i++) {
		cp = argv[i];
		if(dialed)
			silent(sleep_time);
		while(cp && *cp) {
			if(*cp == ',' || *cp == ' ')
				silent(sleep_time);
			else {
				if(dialed)
					silent(silent_time);
				dial_digit(*cp);
			}
			cp++;
		}
	}
	if(bufidx > 0) {
#if 0
		while(bufidx < bufsize) {
			if(format == AFMT_U8) {
				buf[bufidx++] = 128;
			}
			else {	/* AFMT_S16_LE */
				buf[bufidx++] = 0;
				buf[bufidx++] = 0;
			}
		}
#endif
		write(fd, buf, bufidx);
	}
	exit(0);
}

void
dial_digit(int c) {
	DEBUG(fprintf(stderr, "dial_digit %#c\n", c));
	switch(c) {
	case '0':
		dial(941, 1336, tone_time);
		break;
	case '1':
		dial(697, 1209, tone_time);
		break;
	case '2':
		dial(697, 1336, tone_time);
		break;
	case '3':
		dial(697, 1477, tone_time);
		break;
	case '4':
		dial(770, 1209, tone_time);
		break;
	case '5':
		dial(770, 1336, tone_time);
		break;
	case '6':
		dial(770, 1477, tone_time);
		break;
	case '7':
		dial(852, 1209, tone_time);
		break;
	case '8':
		dial(852, 1336, tone_time);
		break;
	case '9':
		dial(852, 1477, tone_time);
		break;
	case '*':
		dial(941, 1209, tone_time);
		break;
	case '#':
		dial(941, 1477, tone_time);
		break;
	case 'A':
		dial(697, 1633, tone_time);
		break;
	case 'B':
		dial(770, 1633, tone_time);
		break;
	case 'C':
		dial(852, 1633, tone_time);
		break;
	case 'D':
		dial(941, 1633, tone_time);
		break;
	}
}

void
silent(int msec) {
	int time;
	if(msec <= 0)
		return; 

	DEBUG(fprintf(stderr, "silent %d\n", msec));

	time = (msec * speed) / 1000;
	while(--time >= 0) {
		if(format == AFMT_U8) {
			buf[bufidx++] = 128;
		}
		else {	/* AFMT_S16_LE */
			buf[bufidx++] = 0;
			buf[bufidx++] = 0;
		}
		if(right || left) {
			if(format == AFMT_U8) {
				buf[bufidx++] = 128;
			}
			else {	/* AFMT_S16_LE */
				buf[bufidx++] = 0;
				buf[bufidx++] = 0;
			}
		}
		if(bufidx >= bufsize) {
			write(fd, buf, bufsize);
			bufidx = 0;
		}
	}
	dialed = 0;
}

void
dial(int f1, int f2, int msec) {
	int i1, i2, d1, d2, e1, e2, g1, g2;
	int time;
	int val;

	if(msec <= 0)
		return;

	DEBUG(fprintf(stderr, "dial %d %d %d\n", f1, f2, msec));

	f1 *= tabsize;
	f2 *= tabsize;
	d1 = f1 / speed;
	d2 = f2 / speed;
	g1 = f1 - d1 * speed;
	g2 = f2 - d2 * speed;
	e1 = speed/2;
	e2 = speed/2;

	i1 = i2 = 0;

	time = (msec * speed) / 1000;
	while(--time >= 0) {
		val = costab[i1] + costab[i2];

		if(left || !right) {
			if(format == AFMT_U8) {
				buf[bufidx++] = 128 + (val >> 8); 
			}
			else {	/* AFMT_S16_LE */
				buf[bufidx++] = val & 0xff;
				buf[bufidx++] = (val >> 8) & 0xff;
			}
		}
		if (left != right) {
			if(format == AFMT_U8) {
				buf[bufidx++] = 128;
			}
			else {	/* AFMT_S16_LE */
				buf[bufidx++] = 0;
				buf[bufidx++] = 0;
			}
		}
		if(right) {
			if(format == AFMT_U8) {
				buf[bufidx++] = 128 + (val >> 8); 
			}
			else {	/* AFMT_S16_LE */
				buf[bufidx++] = val & 0xff;
				buf[bufidx++] = (val >> 8) & 0xff;
			}
		}

		i1 += d1;
		if (e1 < 0) {
			e1 += speed;
			i1 += 1;
		}
		if (i1 >= tabsize)
			i1 -= tabsize;

		i2 += d2;
		if (e2 < 0) {
			e2 += speed;
			i2 += 1;
		}
		if (i2 >= tabsize)
			i2 -= tabsize;

		if(bufidx >= bufsize) {
			write(fd, buf, bufsize);
			bufidx = 0;
		}
		e1 -= g1;
		e2 -= g2;
	}
	dialed = 1;
}

void
gen_costab(void) {
	int i;
	double d;

	costab = (signed short *)malloc(tabsize * sizeof(signed short));
	if(costab == NULL) {
		perror("malloc costab");
		exit(1);
	}

	for (i = 0; i < tabsize; i++) {
		d = 2*M_PI*i;
		costab[i] = (int)((volume/100.)*16383.*cos(d/tabsize));
	}
}
