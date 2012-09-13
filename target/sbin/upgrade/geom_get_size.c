
#include <sys/types.h>
#include <libgeom.h>

size_t
geom_get_size(char *gname)
{
	size_t sz;
	int g;

	g = g_open(gname, 0);
	if (g == -1)
		return (0);

	sz = g_mediasize(g);
	if (sz == -1)
		return (0);

	g_close(g);

	return (sz);
}

#ifdef GEOM_GET_SIZE_TEST
#include <stdio.h>

int
main(int argc, char **argv)
{
	char * gname;

	if (argc > 1)
		gname = argv[1];
	else
		gname = "/dev/mirror/m0s1e";

	size_t sz = geom_get_size(gname);

	printf("%10zu\n", sz);
}
#endif
