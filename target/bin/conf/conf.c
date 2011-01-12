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

#define MAX_TREE_PATH	15

/* Flags */
#define ALLOW_MULTI	(1<<0)	/* Allow more than one key for get/set/dump */
#define DELETE_MODE	(1<<1)	/* Delete value mode */
#define SEARCH_MODE	(1<<2)	/* Search mode */
#define IGNORE_CASE	(1<<3)	/* Ignore case when search */
#define STDOUT_RESULT	(1<<4)	/* Always copy result to stdout */
#define DIRTY_DATA	(1<<5)	/* Data modified, need save */

struct path {
	char 		**part;	/* Variable path parts */
	json_object 	**obj;	/* Objects for path parts */
	int 		count;	/* Count of part */
};

/* Globals */
static int flags = 0;

/*
 *
 */

static void
usage()
{
	printf(
	    "Usage:\n"
	    "\tconf key.subkey\t\t\t\tget/dump from that point\n"
	    "\tconf key.subkey=value\t\t\tset\n"
	    "\tconf key.subkey=value key1.sub\t\tmixed set & get\n"
	    "\tconf \"key.subkey=value with spaces\"\tset \n"
	    "\tconf [-i] -s ubkey key\t\t\t\tsearch keys \"ubkey\" and \"key\"\n"
	    "\tconf -D key.subkey\t\t\tdelete\n"
	    "\t\t-S copy result to stdout\n"
	    );
}

struct path *
allocate_path(int max)
{
	struct path * p = (struct path *)malloc(sizeof(struct path));
	bzero(p, sizeof(struct path));
	p->part = malloc(sizeof(char *) * max);
	p->obj = malloc(sizeof(json_object *) * max);
	bzero(p->part, sizeof(char *       ) * max);
	bzero(p->obj,  sizeof(json_object *) * max);
	return (p);
}

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
	struct path * p = allocate_path(max);
	char **parts = p->part;

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
find(json_object *obj, char *key, struct path * p, int level)
{
	int l;
	json_object *child;

	if (json_object_is_type(obj, json_type_array)) {
		char * check;
		int i = strtoul(key,&check,0);
		/* if key is numeric */
		if ( check != (key + strlen(key)) )
			i = -1;
		for (l = 0; l < json_object_array_length(obj); l ++ ) {
			child = json_object_array_get_idx(obj, l);

			if (!child) continue;

			if ( l == i ) {
    				/* Print path */
    				int x;
    				for (x = 0; x < level; x ++)
    					printf("%s.", p->part[x]);
    				/* Dump value */
    				printf("%d: %s\n", l, json_object_to_json_string(child));
    			}

    			p->obj[level] = child;
    			p->part[level] = (char *)malloc(16);
    			sprintf(p->part[level], "%d", l);
    			p->count = level+1;

    			find(child, key, p, level + 1);

    			p->count = level;
    			free(p->part[level]);
    			p->part[level] = 0;
    			p->obj[level] = 0;
		}
	} else if (json_object_is_type(obj, json_type_object)) {
		struct lh_entry *ent;
		struct lh_table* json_object_table = json_object_get_object(obj);
    		lh_foreach(json_object_table, ent) {
    			struct json_object* child = (struct json_object*)ent->v;

			if (!child) continue;

    			if (
    			    ((flags & IGNORE_CASE)?
    				strcasestr((char *)ent->k, key):
    				strstr((char *)ent->k, key)) != 0) {
    				/* Print path */
    				int x;
    				for (x = 0; x < level; x ++)
    					printf("%s.", p->part[x]);
    				/* Dump value */
    				printf("%s: %s\n", 
    				    (char *)ent->k,
    				    json_object_to_json_string(child));
    			}

    			p->obj[level] = child;
    			p->part[level] = (char *)ent->k;
    			p->count = level+1;

    			find(child, key, p, level + 1);

    			p->count = level;
    			p->part[level] = 0;
    			p->obj[level] = 0;
    		}
	}
}

void
deallocate_path(struct path * p)
{
	if (!p || !p->part) return;
	free(p->part);
	free(p->obj);
	free(p);
}

void
freepath(struct path * p)
{
	if (!p || !p->part) return;
	free(p->part[0]);
	deallocate_path(p);
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

json_object *
create(struct path *p, char *value, int level)
{
	json_object *obj = 0, *child;
	if (level > p->count) return (0);

//	return (0); /* Not yet*/

	child = create(p, value, level+1);
	if (!child) {
		/* Last item */
		/* Try parse as JSON object */
		obj = json_tokener_parse(value);
    		if (is_error(obj)) {
    			/* Or just create string object */
    			obj = json_object_new_string(value);
    		}
    		if (!is_error(obj)) {
    			p->obj[level] = obj;
    			return (obj);
    		}
		return (0);
	} else {
		char * check;
		int i = strtoul(p->part[level], &check, 0);
		if (check == (p->part[level] + strlen(p->part[level]))) i = -1;

		if ( p->obj[level] && json_object_get_type(p->obj[level]) == json_type_array && i >= 0 ) {
			/* We have int, and existing object is array */
			json_object_array_put_idx(p->obj[level], i, child);
		} else if ( p->obj[level] && json_object_get_type(p->obj[level]) == json_type_object) {
			/* We have not int, and existing object is HASH */
			json_object_object_add(p->obj[level], p->part[level], child);
		} else if ( !p->obj[level] ) {
			/* We have no object, need create */
			if (i>= 0) {
				/* We have int, so we need ARRAY */
				obj = json_object_new_array();
				json_object_array_put_idx(obj, i, child);
			} else {
				/* Not int, so we need HASH */
				obj = json_object_new_object();
				json_object_object_add(obj, p->part[level], child);
			}
		} else {
			/* Error */
			printf("Can`t use object \"");
			for (i = 0; i < level; i ++)
				printf(".%s", p->part[i]);
			printf("\" to attach new items\n");
		}
	}
	printf("part = %s\n", p->part[level]);
	return (obj);
}

void process(json_object *root, char * key)
{
	json_object 	*child = 0, *parentobj = 0;
	struct path 	*p = 0;
	char 		*value = "", *check, *last = "", *parent = 0;
	int 		i, idx, max = MAX_TREE_PATH;

	key = strdup(key);

	if (!(flags & SEARCH_MODE)) {
		/* Don`t do it in SEARCH_MODE */

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
	}

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
								flags |= DIRTY_DATA;
							}
							else
								fprintf(stderr, "Wrong index %s for ARRAY %s\n",
								    last, parent);
						} else
							fprintf(stderr, "Wrong index %s for ARRAY %s\n",
								    last, parent);
					} else { /* Delete from HASH */
						if (json_object_object_get(parentobj, last)) {
							json_object_object_del(parentobj, last);
							json_object_object_add(parentobj, last, json_object_new_string(value));
							flags |= DIRTY_DATA;
						}
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
			create(p, value, 0);

/*			for (i = 0; i < (p->count - 1); i ++) {
				if (p->obj[i] && !p->obj[i+1]) {
					if ( json_object_get_type(child) == json_type_array ) {
					} else if ( json_object_get_type(child) == json_type_object ) {
					} else {
						fprintf(stderr, "Can`t attach child to scalar object\n");
					}

					if ( i == p->count-1 ) {
						p->obj[i] = json_object_new_string(value);
						if ( i > 0 && json_object_get_type(child) == json_type_array )
						p->obj[i - 1], p->obj[i - 1]
					} else {
						idx = strtoul(last, &check, 0);
						if (check == last + strlen(last)) {
							// Numeric, so ARRAY //
						} else {
						}
					}
				}
			}
*/
			//flags |= DIRTY_DATA;
			/* Create/Check puth (like mkdir -p) */
			/* Create last node, type string */
		}
	} else if (key && child && !(flags & (DELETE_MODE|SEARCH_MODE))) {
		/* get, dump */
		/*
		 * JSON mode.
		 */
		printf("%s = %s\n", key, json_object_to_json_string(child));
		/*
		 * TODO: Full path mode.
		 * key.subkey.item.1=2
		 * key.item1=x
		 */

	} else if ((flags & DELETE_MODE) && !value && !(flags & SEARCH_MODE)) {
		if (!parentobj) {
			fprintf(stdout, "%s in %s not found\n", parent, key);
			goto parse_end;
		}
		DEBUG_PRINTF("Deleteing %s, \n%s\n",
		    key, json_object_to_json_string(root));

		if (json_object_get_type(parentobj) == json_type_array) {
			struct array_list *arr;
			idx = strtoul(last, &check, 0);
			DEBUG_PRINTF("Parent is ARRAY key=\"%s\"", key);

			if (check != last + strlen(last)) {
				fprintf(stderr, "Wrong index %s for ARRAY %s\n",
				    last, parent);
				goto parse_end;
			}
			/*
			 * XXX Just workaround for ARRAY delete
			 */
			arr = json_object_get_array(parentobj);
			if (idx >= arr->length) {
				fprintf(stderr, "Wrong index %s for ARRAY %s\n",
				    last, parent);
				goto parse_end;
			}
			json_object_put(arr->array[idx]);

			for (i = idx; i < arr->length - 1; i++)
				arr->array[i] = arr->array[i+1];

			arr->length --;
			flags |= DIRTY_DATA;

		} else {
			/* Delete from HASH */
			if (!json_object_object_get(parentobj, last)) {
				fprintf(stdout, "%s in %s not found\n", last, key);
			        goto parse_end;
			}
			json_object_object_del(parentobj, last);
			flags |= DIRTY_DATA;
		}

		/* Dump result */
    		DEBUG_PRINTF("Out: \n%s\n", json_object_to_json_string(root));

	} else if (flags & SEARCH_MODE) {
		struct path * p = allocate_path(max);
		find(root, key, p, 0);
		deallocate_path(p);
		free(key);
		return;

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
	json_object 	*obj;
        char 		*key, ch, *file = "hash.json";

        flags = ALLOW_MULTI;

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


	while ((ch = getopt(argc, argv, "Df:iSs")) != -1)
		switch (ch) {
		case 'f':
			file = optarg;
			break;
		case 'i':
			flags |= IGNORE_CASE;
			break;
		case 's':
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

	obj = json_object_from_file(file);

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
			process(obj, *(argv++));
	} else {
		CP("Single");
		process(obj, key);
	}

	if (flags & STDOUT_RESULT)
		printf("%s\n", json_object_to_json_string(obj));
	else if (flags & DIRTY_DATA)
		if (json_object_to_file(file, obj))
			fprintf(stderr, "Error saving to file \"%s\"\n", file);

	/* Free json_object */
        json_object_put(obj);


	return (0);
}

