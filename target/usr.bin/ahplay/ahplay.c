/**
 * Copyright (c) 2024 Hiroki Mori
 * Copyright (c) 2022 github.com/System233
 * 
 * This software is released under the MIT License.
 * https://opensource.org/licenses/MIT
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <aacdecoder_lib.h>
#include <out123.h>

CStreamInfo *getinfo(HANDLE_AACDECODER decoder)
{
CStreamInfo*info;

	info = aacDecoder_GetStreamInfo(decoder);
	fprintf(stderr, "sampleRate:%d,bitRate:%d,frameSize:%d,channels:%d\n",
	    info->sampleRate,
	    info->bitRate,
	    info->frameSize,
	    info->numChannels
	);

	return info;
}

int main(int argc, char const *argv[])
{
	int s;
	struct hostent *servhost;
	struct sockaddr_in server;
	char send_buf[256];
	const char *host;
	int port;
	const char *path;
	static UCHAR inbuf[20480];
	static INT_PCM outbuf[20480];
	UCHAR *pInbuf = inbuf;
	out123_handle *out;
	int len;
	CStreamInfo*info=NULL;

	if(argc != 2 && argc != 4){
		fprintf(stderr,"Usage: ahplay [filename] | [server] [port] [path]\n");
		return 1;
	}

	if(argc == 2){
		if (strcmp(argv[1], "-")) {
			if (( s = open(argv[1], O_RDONLY)) < 0) {
				fprintf(stderr, "socket error\n");
				return 1;
			}
		} else {
			s = 0;
		}
	} else {    /* argc == 4 */
		host = argv[1];
		port = atoi(argv[2]);
		path = argv[3];
		servhost = gethostbyname(host);
		bzero(&server, sizeof(server));
		bcopy(servhost->h_addr, &server.sin_addr, servhost->h_length);
		server.sin_family = AF_INET;
		server.sin_port = htons(port);

		if ( ( s = socket(AF_INET, SOCK_STREAM, 0) ) < 0 ) {
			fprintf(stderr, "socket error\n");
			return 1;
		}
		if ( connect(s, (struct sockaddr *)&server, sizeof(server))
		     == -1 ){
			fprintf(stderr, "connect error\n");
			return 1;
		}
		sprintf(send_buf, "GET %s HTTP/1.0\r\n", path);
		write(s, send_buf, strlen(send_buf));

		sprintf(send_buf, "Host: %s:%d\r\n", host, port);
		write(s, send_buf, strlen(send_buf));

		sprintf(send_buf, "\r\n");
		write(s, send_buf, strlen(send_buf));

		/* skip header */
		char h1, h2;
		read(s, &h1, 1);
		while (1){
			read(s, &h2, 1);
			if (h1 == '\n' && (h2 == '\r' || h2 == '\n'))
				break;
			h1 = h2;
		}
		if (h2 == '\r')
			read(s, &h2, 1);
	}

	HANDLE_AACDECODER decoder = aacDecoder_Open(TT_MP4_ADTS,2);

	out = out123_new();
	if(!out)
	   return 1;
	if(out123_open(out, NULL, NULL))
	   return 1;

	while(len = read(s, inbuf, sizeof(inbuf))) {
		UINT inLen = len;
		UINT bytesValid = len;
		while(bytesValid){
			if(aacDecoder_Fill(decoder, &pInbuf, &inLen,
			    &bytesValid) != AAC_DEC_OK){
				fprintf(stderr, "aacDecoder_Fill error");
				return 1;
			}
			AAC_DECODER_ERROR err;
			while ((err = aacDecoder_DecodeFrame(decoder, outbuf,
			    sizeof(outbuf) / sizeof(*outbuf), 0))
			    == AAC_DEC_OK)
			{
				if(!info) {
					info = getinfo(decoder);
					if(out123_start(out,
	    				    info->sampleRate,
	    				    info->numChannels,
					    MPG123_ENC_SIGNED_16))
						return 1;
				}

				out123_play(out, outbuf,
				    info->frameSize * info->numChannels * 2);
			}
			if(err != AAC_DEC_NOT_ENOUGH_BITS){
				fprintf(stderr, "decode fail :%#x", err);
				return 1;
			}
		}
	};
exit:
	out123_del(out);
	if (s != 0)
		close(s);
	aacDecoder_Close(decoder);

	return 0;
}
