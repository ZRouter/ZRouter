/* 
 * gcc -shared -fPIC -o msleep.so -I/usr/include/lua5.1 -llua5.1 msleep.c
 * -I and -l may vary on your computer.
 * Your computer may use something besides -fPIC
*/

#include <unistd.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

static int msleep_c(lua_State *L){
	long msecs = lua_tointeger(L, -1);
	usleep(1000*msecs);
	return 0;                  /* No items returned */
}

/* Can't name this sleep(), it conflicts with sleep() in unistd.h */
static int sleep_c(lua_State *L){
	long secs = lua_tointeger(L, -1);
	sleep(secs);
	return 0;                  /* No items returned */
}

/* Register both functions */
int luaopen_sleep(lua_State *L){
	lua_register( L, "msleep", msleep_c);  
	lua_register(L, "sleep", sleep_c);
	return 0;
}
