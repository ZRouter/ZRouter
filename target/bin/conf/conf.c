#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>


#include <json/json.h>

#ifdef DEBUG
#define	DEBUG_PRINTF(x, ...) printf(x, __VA_ARGS__)
#else
#define DEBUG_PRINTF(x, ...)
#endif


void bp()
{
	DEBUG_PRINTF("bp\n");
}

json_object * get (json_object *obj, char * key)
{
	char *newkey, *firstkey;
	json_object *child = 0;
	DEBUG_PRINTF("--------------------------------------------\n");

	firstkey = newkey = strdup(key);

	newkey = strchr(newkey, '.');
	if (newkey) {
		*newkey++ = '\0';
		DEBUG_PRINTF("newkey=\"%s\" ", newkey);
	}
	DEBUG_PRINTF("firstkey=\"%s\"\n", firstkey);

	if (json_object_is_type(obj, json_type_array))
		child = json_object_array_get_idx(obj, strtoul(firstkey,0,0));
	else if (json_object_is_type(obj, json_type_object))
		child = json_object_object_get(obj, firstkey);

	if (!child) {
		free(firstkey);
		return (0);
	}

	if (newkey) {
		DEBUG_PRINTF("get(child, \"%s\")\n", newkey);
		DEBUG_PRINTF(
		    "--------------------------------------------\n"
		    "child dump:\n%s\n" 
		    "--------------------------------------------\n", 
		    json_object_to_json_string(child));
		child = get(child, newkey);
	}

	free(firstkey);
	return (child);
}

int main(int argc, char ** argv)
{
	json_object *obj, *child;
	struct stat sb;
        char *input;
        int f;

	MC_SET_DEBUG(1);

	// getopt
	// check flags
	// check storage location from ENV ot flags
	// check storage
///////////////////////////////////
	// while storage {
	// ?? uncompress block
	// send to JSON parser
	// ?? maybe ask key existings
	// }
// or /////////////////////////////////
	// read and uncompress storage
///////////////////////////////////
	// if (get || dump) { get ((dump)?all) value; cleanup; return; }
	// if set {}


	if (argc < 2) return 1;


	if (stat(argv[1], &sb)) {
		//XXX perror
		return (1);
	}

	f = open(argv[1], O_RDONLY);
	if (f < 0) {
		//XXX perror
		return (1);
	}

	input = mmap(0, sb.st_size, PROT_READ, 0, f, 0);
	//XXX Check "input"

        obj = json_tokener_parse(input);

        munmap(input, sb.st_size);
        close(f);

        if (is_error(obj)) {
                DEBUG_PRINTF("error parsing json: %s\n",
		       json_tokener_errors[-(unsigned long)obj]);
		return (1);
        }


        DEBUG_PRINTF("%s\n", json_object_to_json_string(obj));

	if (1) //mode_get
	{
		// argv[2] is variable path
		/* config.interfaces.wan.0.address */
		if (argc<3) return (1);

		child = get(obj, argv[2]);
		if (child) {
			printf("%s = %s\n",
			    argv[2], json_object_to_json_string(child));
		}
	}
	else if (0) // mode_dump
	{
	}
	else if (0) // mode set
	{
	}
	else if (0) // mode delete
	{
	}
	else if (0) // mode search
	{
	}
	else {
		// Invalid mode
	}

	/* Free json_object */
        json_object_put(obj);


	return (0);
}

