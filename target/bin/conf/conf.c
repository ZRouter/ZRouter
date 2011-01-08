#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>


#include <json/json.h>

#ifdef DEBUG
#define	DEBUG_PRINTF(x, ...) printf(x, __VA_ARGS__)
#define CP(x)	printf("%s:%d: %s\n", __func__, __LINE__,  x)
#else
#define DEBUG_PRINTF(x, ...)
#define CP(x)
#endif


#define ALLOW_MULTI	(1<<0)	/* Allow more than one key for get/set/dump */
#define DELETE_MODE	(1<<1)	/* Delete value mode */
#define SEARCH_MODE	(1<<2)	/* Search mode */
#define STDOUT_RESULT	(1<<3)	/* Always copy result to stdout*/

void usage()
{
	printf(
	    "Usage:\n"
	    "\tconf key.subkey\t\t\t\tget/dump from that point\n"
	    "\tconf key.subkey=value\t\t\tset\n"
	    "\tconf key.subkey=value key1.sub\t\tmixed set & get\n"
	    "\tconf \"key.subkey=value with spaces\"\tset \n"
	    "\tconf -s ubkey\t\t\t\tsearch\n"
	    "\tconf -D key.subkey\t\t\tdelete\n"
	    "\t\t-S copy result to stdout\n"
	    );
}

struct path {
	char 		**part;	/* Variable path parts */
	json_object 	**obj;	/* Objects for path parts */
	int 		count;	/* Count of part */
};


/** Split input string by "." char
 *
 * max must have limit of parts count
 *
 * @param path the string, parametr path joined by '.'
 * @returns a json_object of type json_type_string
 */
struct path *
splitpath(char *path, int max)
{
	int i;
	char **parts;
	struct path * p = (struct path *)malloc(sizeof(struct path));
	bzero(p, sizeof(struct path));
	p->part = parts = malloc(sizeof(char *) * max);
	p->obj = malloc(sizeof(json_object *) * max);
	bzero(p->part, sizeof(char *       ) * max);
	bzero(p->obj,  sizeof(json_object *) * max);

	path = strdup(path);

	for (
	    i = 0, *parts++ = path;
	    (path = strchr(path, '.'));
	    *path++ = '\0', *parts++ = path, i++ )
		if (!*path && (i++ >= max))
			break;
	p->count = i+1;
	return (p);

}

void
freepath(struct path * p)
{
	if (!p || !p->part) return;
	free(p->part[0]);
	free(p->part);
	free(p->obj);
	free(p);
}

json_object * get (json_object *obj, struct path *p, int idx)
{
	json_object *child = 0;
	DEBUG_PRINTF("%d --------------------------------------------\n", __LINE__);


	if (json_object_is_type(obj, json_type_array)) {
		char * check;
		unsigned int i = strtoul(p->part[idx],&check,0);
		if ( check == (p->part[idx] + strlen(p->part[idx])))
			child = json_object_array_get_idx(obj, i);
		else
			printf("Array require int for index\n");
	} else if (json_object_is_type(obj, json_type_object))
		child = json_object_object_get(obj, p->part[idx]);

	if (!child) {
		return (0);
	}

	p->obj[idx] = child;

	if (idx < (p->count - 1)) {
		DEBUG_PRINTF("get(child, \"%s\") count=%d\n", p->part[idx+1], p->count);
		DEBUG_PRINTF(
		    "--------------------------------------------\n"
		    "child dump:\n%s\n" 
		    "--------------------------------------------\n", 
		    json_object_to_json_string(child));
		child = get(child, p, idx+1);
	}

	return (child);
}

void process(json_object *root, char * key, int flags)
{
	json_object 	*child, *parentobj = 0;
	struct path 	*p;
	char 		*value, *check, *last, *parent = 0;
	int 		i, idx, max = 10;

	key = strdup(key);
	value = strchr(key, '=');
	/* Split key and value */
	if (value) *value++ = '\0';

	/* Parse path and get nodes */
	p = splitpath(key, max);
	child = get(root, p, 0);

	if (p->count == 0) {
		fprintf(stderr, "Error parsing key=%s value=%s\n", key, value);
		goto parse_end;
	}

	if (p->count > 1) {
		parentobj = p->obj[p->count-2];
		parent = p->part[p->count-2];
	}

	last = p->part[p->count-1];

	DEBUG_PRINTF("parent = %s, last = %s\n", parent, last);


	if (key && value && !(flags & (DELETE_MODE|SEARCH_MODE))) /* set */
	{
		if (child) {
			/* Existing child */
			if (( json_object_get_type(child) == json_type_array &&
				json_object_array_length(child) ) ||
			   ( json_object_get_type(child) == json_type_object &&
				json_object_get_object(child)->count ))
				fprintf(stderr, "Node \"%s\" has one or more childs, "
				    "please delete it first.\n", key);
			else {
				DEBUG_PRINTF("Set \"%s\" from \"%s\" to \"%s\"\n",
				    key, 
				    json_object_to_json_string(child),
				    value);
				/* delete node */

				if (parentobj) {
					if (json_object_get_type(parentobj) ==
					    json_type_array) {
						idx = strtoul(last, &check, 0);
						DEBUG_PRINTF("Parent is ARRAY key=\"%s\"", key);

						if (check == last + strlen(last)) {
							if (idx < json_object_array_length(parentobj)) {
								//json_object_put(child);
								json_object_array_put_idx(parentobj, idx, json_object_new_string(value));
							}
							else
								fprintf(stderr, "Wrong index %s for ARRAY %s\n",
								    last, parent);
						} else
							fprintf(stderr, "Wrong index %s for ARRAY %s\n",
								    last, parent);
					} else { /* Delete from HASH */
						json_object_object_del(parentobj, last);
						json_object_object_add(parentobj, last, json_object_new_string(value));
					}

					/* Dump result */
    					DEBUG_PRINTF("Out: \n%s\n", json_object_to_json_string(root));
				}
				/* create string with value */
			}
		} else {
			/* Create child */
			printf("NOTYET Create \"%s\" with value %s\n",
			    key, json_object_to_json_string(p->obj[p->count-2]));
			/* Create/Check puth (like mkdir -p) */
			/* Create last node, type string */
		}
	} else if (key && child && !(flags & (DELETE_MODE|SEARCH_MODE))) {
		/* get, dump */
		/*
		 * TODO: JSON mode.
		 */
		printf("%s = %s\n", key, json_object_to_json_string(child));
		/*
		 * TODO: Full path mode.
		 * key.subkey.item.1=2
		 * key.item1=x
		 */

	} else if ((flags & DELETE_MODE) && !value) {
		if (parentobj) {
			DEBUG_PRINTF("Deleteing %s, \n%s\n",
			    key, json_object_to_json_string(root));

			if (json_object_get_type(parentobj) == json_type_array) {
				struct array_list *arr;
				idx = strtoul(last, &check, 0);
				DEBUG_PRINTF("Parent is ARRAY key=\"%s\"", key);

				if (check == last + strlen(last)) {
					/*
					 * XXX Just workaround for ARRAY delete
					 */
					arr = json_object_get_array(parentobj);
					if (idx < arr->length) {
						json_object_put(arr->array[idx]);

						for (i = idx; i < arr->length - 1; i++)
							arr->array[i] = 
								arr->array[i+1];

						arr->length --;
					}
					else
						fprintf(stderr, "Wrong index %s for ARRAY %s\n",
						    last, parent);
				} else
					fprintf(stderr, "Wrong index %s for ARRAY %s\n",
						    last, parent);
			} else /* Delete from HASH */
				json_object_object_del(parentobj, last);

			/* Dump result */
    			DEBUG_PRINTF("Out: \n%s\n", json_object_to_json_string(root));
		}

	} else if (flags & SEARCH_MODE) {

	} else {
		/* Invalid mode */
		usage();
	}

parse_end:
	free(key);
	freepath(p);
}

int
main(int argc, char ** argv)
{
	json_object *obj;
	struct stat sb;
        char *input, *key, ch, *file = "hash.json";
        int f, flags = ALLOW_MULTI;

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


	while ((ch = getopt(argc, argv, "f:sDS")) != -1)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 's':
			flags &= ~(ALLOW_MULTI);
			flags |= SEARCH_MODE;
			break;
		case 'D':
			flags &= ~(ALLOW_MULTI);
			flags |= DELETE_MODE;
			break;
		case 'S':
			flags |= STDOUT_RESULT;
			break;
		default:
			usage();
		}
	argv += optind;
	argc -= optind;
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

	CP("parse");
        obj = json_tokener_parse(input);
	CP("parse done");

        munmap(input, sb.st_size);
        close(f);

        if (is_error(obj)) {
                DEBUG_PRINTF("error parsing json: %s\n",
		       json_tokener_errors[-(unsigned long)obj]);
		return (1);
        }

	CP("dump");

        DEBUG_PRINTF("%s\n", json_object_to_json_string(obj));


	if (flags & ALLOW_MULTI) {
		CP("Multi");
		for (; argc; argc--)
			process(obj, *(argv++), flags);
	} else {
		CP("Single");
		process(obj, key, flags);
	}

	if (flags & STDOUT_RESULT)
		printf("%s\n", json_object_to_json_string(obj));

	/* Free json_object */
        json_object_put(obj);


	return (0);
}

