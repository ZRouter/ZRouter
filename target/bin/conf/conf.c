#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <json/json.h>

void bp()
{
	printf("bp\n");
}

int main(int argc, char ** argv)
{
	json_object *obj;

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

        char *input = (char*)malloc(1024);

	FILE *f = fopen(argv[1], "r");
        while (fgets(input, 1024, f) != NULL) {
                obj = json_tokener_parse(input);
                if (is_error(obj)) {
                        printf("error parsing json: %s\n",
			       json_tokener_errors[-(unsigned long)obj]);
                } else {
                        printf("%s\n", json_object_to_json_string(obj));
                        json_object_put(obj);
                }
        }
        fclose(f);


	return 0;
}

