#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


char hex2bin4b( char in )
{
	     if ('a' <= in && in <= 'f') return (in - 'a' + 0x0a);
	else if ('A' <= in && in <= 'F') return (in - 'A' + 0x0a);
	else if ('0' <= in && in <= '9') return (in - '0' + 0x00);
	else 
	{
		fprintf(stderr, "Wrong character %c\n", in);
		exit(1);
	}

}

void hex_string_out(char * str)
{
	int i;
	for (i = 0; i < strlen(str); i +=2 )
	{
		while ( str[i] && isspace(str[i]) ) i ++;
		printf("%c", (hex2bin4b(str[i])<<4) | hex2bin4b(str[i+1]));
	}
}

int main(int argc, char ** argv)
{
	int i;

	if (argc < 2) return 1;
	for ( i = 1; i < argc; i ++ )
		hex_string_out(argv[i]);

	return 0;
}

