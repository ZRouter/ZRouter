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


void usage()
{
	printf(
	    "Usage:\n"
	    "\tconf key.subkey\t\t\t\tget/dump from that point\n"
	    "\tconf key.subkey=value\t\t\tset\n"
	    "\tconf \"key.subkey=value with spaces\"\tset \n"
	    "\tconf -s ubkey\t\t\t\tsearch\n"
	    "\tconf -D key.subkey\t\t\tdelete\n"
	    );
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

void free_obj(struct json_object *obj) 
{
	int n, l;
	switch (json_object_get_type(obj)) {
	case json_type_object:
		{
        		json_object_object_foreach(obj, key, val)
            			free_obj(val);
                }
                json_object_put(obj);
		break;
        case json_type_array:
        	l = json_object_array_length(obj);
                for (n=0; n < l; n++)
            		free_obj(json_object_array_get_idx(obj, n));
                json_object_put(obj);
                break;
        default:
                json_object_put(obj);
        }
}


int main(int argc, char ** argv)
{
	json_object *obj, *child;
	struct stat sb;
        char *input, *key, *value, ch, *file = "hash.json";
        int f, search = 0, delete = 0;

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


	while ((ch = getopt(argc, argv, "f:sD")) != -1)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 's':
			search = 1;
			break;
		case 'D':
			delete = 1;
			break;
		default:
			usage();
		}
	argv += optind;
	key = *argv;

	if (!key) {
		usage();
		return (1);
	}




	if (stat(file, &sb)) {
		//XXX perror
		return (1);
	}

	f = open(file, O_RDONLY);
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

	value = strchr(key, '=');
	/* Split key and value */
	if (value) *value++ = '\0';

	if (key && value && !delete && !search) /* set */
	{
		child = get(obj, key);
		if (child) {
			/* XXX Check for chils 
			printf("Node \"%s\" has one or more childs, please delete first\n",
			    key);
			*/
			/* Existing child */
			printf("Set \"%s\" from \"%s\" to \"%s\"\n",
			    key, json_object_to_json_string(child), value);
		} else {
			/* Create child */
			printf("Create \"%s\" with value %s\n",
			    key, json_object_to_json_string(child));
		}
	}
	else if (key && !delete && !search) /* get, dump */
	{
		/* config.interfaces.wan.0.address */

		child = get(obj, key);
		if (child) {
			printf("%s = %s\n",
			    key, json_object_to_json_string(child));
		}

	} else if (delete && !value) {
		child = get(obj, key);
		if (child) {
			printf("Deleteing %s, \"%s\"\n",
			    key, json_object_to_json_string(child));
			printf("CP\n");
			free_obj(child);
			printf("CP\n");
			/* Dump result */
    			//DEBUG_PRINTF
    			printf("%s\n", json_object_to_json_string(obj));
		}

	} else if (search) {

	} else {
		/* Invalid mode */
		usage();
	}

	/* Free json_object */
        json_object_put(obj);


	return (0);
}

