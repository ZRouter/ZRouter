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

void bp()
{
	printf("bp\n");
}

int main(int argc, char ** argv)
{
	json_object *obj;
	struct stat sb;

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

        char *input;

	if (stat(argv[1], &sb)) {
		// perror
		exit(1);
	}
	int f = open(argv[1], O_RDONLY);
	if (f < 0 ) {
		// perror
		exit(1);
	}
	input = mmap(0, sb.st_size, PROT_READ, 0, f, 0);
	// Check "input"
	printf(
	    "===========================================\n"
	    " Dump:\n"
	    "===========================================\n"
	    "%s\n"
	    "===========================================\n",
	    input
	);

        obj = json_tokener_parse(input);
        if (is_error(obj)) {
                printf("error parsing json: %s\n",
		       json_tokener_errors[-(unsigned long)obj]);
        } else {
                printf("%s\n", json_object_to_json_string(obj));
                json_object_put(obj);
        }

        munmap(input, sb.st_size);
        close(f);


	return 0;
}

