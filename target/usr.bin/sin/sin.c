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
 * 量子化ビット数 : 16 bits
 * サンプリング周波数 : 44.1 KHz
 * チャンネル数 : 1
 * 再生時間 : 5 sec
 *
 * 分のバッファ領域
 *
 * 16 (bits) * 44100 (Hz) * 5 (sec) = 3528000 (bits)
 *                                  = 441000  (bytes)
 */
#define BUFSIZE 480000

static int setup_dsp( int fd, int be );

int 
main(int argc, char ** argv)
{
	/* 再生時間 5 秒 */
	double total = 5.0, t; 

	/* 再生音周波数 440 Hz ( A ) */
	double freq = 440.0;

	short  buf[ BUFSIZE / sizeof(short) ];
	int    fd, i;
	char   ch;
	int    silence = 0;
	int    bigendian = 0;

	while ((ch = getopt(argc, argv, "skb")) != -1) {
		if(ch == 's') {
			silence = 1;
		}
		if(ch == 'k') {
			freq = 1000.0;
		}
		if(ch == 'b') {
			bigendian = 1;
		}
	}

	if ( ( fd = open( "/dev/dsp", O_WRONLY ) ) == -1 ) {
		perror( "open()" );
		return 1;
	}


	/* /dev/dsp の設定 */
	if ( setup_dsp( fd, bigendian ) != 0 ) {
		fprintf( stderr, "Setup /dev/dsp failed.\n" );
		close( fd );
		return 1;
	}


	/* 再生用の正弦波データ作成 */
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


/*
 * /dev/dsp を以下の様に設定する。
 *
 * 量子化ビット数     : 16   bits
 * サンプリング周波数 : 44.1 KHz
 * チャンネル数       : 1
 * PCM データは符号付き、リトルエンディアン
 *
 */
static int
setup_dsp( int fd, int be )
{
	int fmt     = AFMT_S16_LE;
	int freq    = 48000;
	int channel = 1;

	/* サウンドフォーマットの設定 */
	if ( be )
		fmt = AFMT_S16_BE;

	if ( ioctl( fd, SOUND_PCM_SETFMT, &fmt ) == -1 ) {
		perror( "ioctl( SOUND_PCM_SETFMT )" );
		return -1;
	}

	/* チャンネル数の設定 */
	if ( ioctl( fd, SOUND_PCM_WRITE_CHANNELS, &channel ) == -1 ) {
		perror( "iotcl( SOUND_PCM_WRITE_CHANNELS )" );
		return -1;
	}

	/* サンプリング周波数の設定 */
	if ( ioctl( fd, SOUND_PCM_WRITE_RATE, &freq ) == -1 ) {
		perror( "iotcl( SOUND_PCM_WRITE_RATE )" );
		return -1;
	}

	return 0;
}

/* End of sound.c */

