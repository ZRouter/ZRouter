/*
  http://rio.la.coocan.jp/lab/oss/000prologue.htm
*/

#include <fcntl.h>
#include <limits.h>
#include <sys/soundcard.h>
#include <math.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>

/*
 * 16 (bits) * 48000 (Hz) * 1 (sec) = 96000 (bytes)
 */
 
#define BUFSIZE 96000

static int setup_dsp( int fd);

int 
main(int argc, char ** argv)
{
	double total = 1.0, t; 

	double freq = 440.0;

	short  buf[ BUFSIZE / sizeof(short) ];
	int    fd, i;
	char   ch;
	int    silence = 0;

	while ((ch = getopt(argc, argv, "skb")) != -1) {
		if(ch == 's') {
			silence = 1;
		}
		if(ch == 'k') {
			freq = 1000.0;
		}
	}

	if ( ( fd = open( "/dev/dsp", O_WRONLY ) ) == -1 ) {
		perror( "open()" );
		return 1;
	}

	if ( setup_dsp( fd) != 0 ) {
		fprintf( stderr, "Setup /dev/dsp failed.\n" );
		close( fd );
		return 1;
	}


	for ( i = 0; i < BUFSIZE / sizeof(short); i ++ ) {
		t = ( total / (BUFSIZE / sizeof(short)) ) * i;
		if (silence == 1)
			buf[i] = 0;
		else
			buf[i] = SHRT_MAX * sin( 2.0 * 3.14159 * freq * t );
	}
    

	while (1)
	if ( write( fd, buf, BUFSIZE ) == -1 ) {
		perror( "write()" );
		close( fd );
		return 1;
	}

	close( fd );
	return 0;
}


static int
setup_dsp( int fd)
{
	int fmt;
	int freq    = 48000;
	int channel = 1;

#if BYTE_ORDER == LITTLE_ENDIAN
	fmt = AFMT_S16_LE;
#else
	fmt = AFMT_S16_BE;
#endif

	if ( ioctl( fd, SOUND_PCM_SETFMT, &fmt ) == -1 ) {
		perror( "ioctl( SOUND_PCM_SETFMT )" );
		return -1;
	}

	if ( ioctl( fd, SOUND_PCM_WRITE_CHANNELS, &channel ) == -1 ) {
		perror( "iotcl( SOUND_PCM_WRITE_CHANNELS )" );
		return -1;
	}

	if ( ioctl( fd, SOUND_PCM_WRITE_RATE, &freq ) == -1 ) {
		perror( "iotcl( SOUND_PCM_WRITE_RATE )" );
		return -1;
	}

	return 0;
}
