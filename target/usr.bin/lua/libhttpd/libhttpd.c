/* -*-mode: C++; style: K&R; c-basic-offset: 4 ; -*- */

/**
 *  libhttpd.c
 *   Simple network primitives for use by the Lua 5.0 scripting engine.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General  Public License (LGPL)
 *  as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 *
 *  Steve Kemp
 *  ---
 *  http://www.steve.org.uk/
 *
 */


/*
 *  Each of the functions which passes, or returns, a socket merely
 * accessess them via Lua's "tonumber", or "fromnumber" routines.
 * 
 *  This is suboptimal, however in the absense of threads it is likely
 * to work out well in practice.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#define __USE_BSD 1
#include <string.h>
 
#include <arpa/inet.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <dirent.h>

#include <netdb.h>

#include <unistd.h>
 
#define MYNAME		"libhttpd"
#define VERSION	        "$Id: libhttpd.c,v 1.18 2005/10/31 16:50:54 steve Exp $"



#include "lua.h"
#include "lauxlib.h"



/*
 * Find and return the version identifier from our CVS marker.
 * The memory returned is static.
 */
char * getVersion( )
{
    char *start  = NULL;
    char *end    = NULL;
    int length   = 0;
    char *prolog = "Release " RELEASE " CVS ID v";

    static char marker[128] = {'\0'};

    /*
     * If we have already calculated the revision information
     * then return it.
     */
    if ( strlen( marker ) > 0 )
    {
	return( marker );
    }


    start = strstr( VERSION, ",v " );
    if ( start == NULL )
	return NULL;

    /* Add on the ",v " text. */
    start += 3;
   
    /* Now find the next space - after the version marker */
    end = strstr( start, " " );
    if ( end == NULL )
	return NULL;


    /* The length of the version marker revision ID. */
    length  = end - start;

    /* Add in the prolog + revision information */
    strncpy( marker, prolog, strlen( prolog ) );
    strncat( marker, start, length );

    return( marker );
}


/**
 * Return an error string to the LUA script.
 * @param L The Lua intepretter object.
 * @param info An error string to return to the caller.  Pass NULL to use the return value of strerror.
 */
static int pusherror(lua_State *L, const char *info)
{
    lua_pushnil(L);

    if (info==NULL)
	lua_pushstring(L, strerror(errno));
    else
	lua_pushfstring(L, "%s: %s", info, strerror(errno));

    lua_pushnumber(L, errno);
    return 3;
}


/**
 * Bind a socket to a port, and return it.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pBind(lua_State *L)
{
    int sockfd;
    struct sockaddr_in serv_addr;
    int on   = 1;
    int port = 0;

    if (lua_isnumber(L, 1))
	port =lua_tonumber(L, 1);
    else
	return( pusherror(L, "bind(int) requires a port number" ) );


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) 
        return( pusherror(L, "ERROR opening socket") );


    /* Enable address reuse */
    setsockopt( sockfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );

    memset( &serv_addr, '\0', sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port);

    /* bind */
    if (bind(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) 
	return( pusherror(L, ("ERROR on binding") ));

    /* queue five connections. */
    listen(sockfd,5);

    /* Return socket */
    lua_pushnumber(L, sockfd);
    return( 1 );
}




/**
 * Connect a socket to a host/port, and return it.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pConnect(lua_State *L)
{
    struct sockaddr_in sa;
    struct hostent *hp;
    int ret,sockfd;
    const char *host;
    int port = 0;

    if (lua_isstring(L, 1))
	host = lua_tostring(L, 1);
    else
	return( pusherror(L, "connect(string,int) incorrect first argument" ) );
    if (lua_isnumber(L, 2))
	port = lua_tonumber(L, 2);
    else
	return( pusherror(L, "connect(string,int) incorrect second argument" ) );

    /* Get the host. */ 
    hp = gethostbyname(host);
    bcopy((char *)hp->h_addr, (char *)&sa.sin_addr, hp->h_length);
    sa.sin_family = hp->h_addrtype;
    sa.sin_port = htons(port);
    sockfd = socket(hp->h_addrtype, SOCK_STREAM, 0);

    /* connect */
    ret = connect(sockfd, (struct sockaddr *)&sa, sizeof(sa));
    if ( ret < 0 )
	return( pusherror(L, "Failed to connect" ) );

    /* Return socket */
    lua_pushnumber(L, sockfd);
    return( 1 );
}



/**
 * Accept a new connection upon a socket.  Return the new client.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pAccept(lua_State *L)
{
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in  cli_addr;

    if (lua_isnumber(L, 1))
	sockfd =lua_tonumber(L, 1);
    else
	return( pusherror(L, "accept(int) requires listening socket." ) );
    
    /* accept() */
    clilen = sizeof(cli_addr);
    newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);

    /* Return error on failure */
    if (newsockfd < 0) 
	return( pusherror(L,"ERROR on accept"));

    
    /*
     * Return the new socket and the connecting IP address.
     */
    lua_pushnumber(L,newsockfd);
    lua_pushstring(L,inet_ntoa(cli_addr.sin_addr));
    return 2;
}



/**
 * Read from a socket, return the data and the length read.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pRead(lua_State *L)
{
    int sockfd;
    int n = 0;

    /* Buffer we read() into */
    char buffer[4096];
    memset( buffer, '\0', sizeof(buffer));
    
    if (lua_isnumber(L, 1))
	sockfd =lua_tonumber(L, 1);
    else
	return( pusherror(L, "read(int)" ) );
    
    /* Do the read */
    n = read(sockfd,buffer,sizeof(buffer)-1);

    if ( n == -1 )
	return( pusherror(L, "Problem reading from socket" ) );

    /* Return the data, and the length of that data */
    lua_pushnumber(L, n );
    lua_pushlstring(L, buffer, n );
    
    return( 2 );
}



/**
 * Write data to a socket.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pWrite(lua_State *L)
{
    int sockfd;
    const char *data = NULL;
    int length       = 0;
    int bytesSent    = 0 ;

    /* Get the number of arguments. */
    int n = lua_gettop(L);

    if ( ( n != 2 ) && ( n != 3 ) )
	return( pusherror(L, "write(int,string,[size])" ) );

    if (lua_isnumber(L, 1))
	sockfd =lua_tonumber(L, 1);
    else
	return( pusherror(L, "write(int,string,[size])" ) );

    /*
     * Supplied the string only
     */
    if ( n == 2 )
    {
	if (lua_isstring(L, 2))
	    data = lua_tostring(L, 2);
	else
	    return( pusherror(L, "write(int,string,[size])" ) );

	/*
	 * Get the string length from Lua.  This takes account of
	 * embedded NULLs.
	 */
	length = lua_strlen(L, 2 );
    }
    else if ( n == 3 )
    {
	if (lua_isstring(L, 2))
	    data = lua_tostring(L, 2);
	else
	    return( pusherror(L, "write(int,string,[size])" ) );

	/*
	 * Caller provided the length.
	 */
	if (lua_isnumber(L, 3))
	    length = lua_tonumber(L, 3);
	else
	    return( pusherror(L, "write(int,string,[size])" ) );
    }


    /*
     * Loop sending the data.
     */
    while (bytesSent < length)
    {
	/* Send some */
	int sent = write( sockfd, data + bytesSent, length - bytesSent);
	
	if (sent <= 0 || errno == EPIPE)
	{
	    return( pusherror(L, "Problem writing to socket" ) );
	}

	bytesSent += sent ;
    }   

    return 0;
}



/**
 * Close a socket.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pClose(lua_State *L)
{
    int sockfd;

    if (lua_isnumber(L, 1))
	sockfd =lua_tonumber(L, 1);
    else
	return( pusherror(L, "close(int)" ) );

    close( sockfd );

    return( 0 );
}



/**
 * Test to see if the given file/path is a directory.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pIsDir(lua_State *L)
{
    const char *fileName = NULL;
    struct stat st;

    if (lua_isstring(L, 1))
	fileName = lua_tostring(L, 1);
    else
	return( pusherror(L, "is_dir(string)" ) );

    if (stat(fileName, &st) == 0) 
	lua_pushboolean(L, S_ISDIR(st.st_mode) );
    else
	return( pusherror(L, "stat failed!" ) );

    return 1;
}



/**
 * Test to see if the given file/path is a file.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pIsFile(lua_State *L)
{
    const char *fileName = NULL;
    struct stat st;

    if (lua_isstring(L, 1))
	fileName = lua_tostring(L, 1);
    else
	return( pusherror(L, "is_file(string)" ) );

    if (stat(fileName, &st) == 0) 
	lua_pushboolean(L, S_ISREG(st.st_mode) );
    else
	return( pusherror(L, "stat failed!" ) );

    return 1;
}



/**
 * Return a table of all the directory entries in the given dir.
 * @param L The lua intepreter object.
 * @return The number of results to be passed back to the calling Lua script.
 */
static int pReadDir(lua_State *L)
{
    const char *dirName = NULL;
    struct dirent *dp;
    DIR *dir;
    int count = 0;
       
    if (lua_isstring(L, 1))
	dirName = lua_tostring(L, 1);
    else
	return( pusherror(L, "readdir(string)" ) );

    /* creates a table/array to hold the results */
    lua_newtable(L);

    dir = opendir(dirName);
    while (dir) 
    {
	if ((dp = readdir(dir)) != NULL) 
        {
	    /* Store in the table. */
	    lua_pushnumber(L, count);
	    lua_pushstring(L, dp->d_name );
	    lua_settable(L, -3);

	    count += 1;
	}
	else
	{
	    closedir( dir );
	    dir = 0;
	}
    }


    /* Make sure LUA knows how big our table is. */
    lua_pushliteral(L, "n");
    lua_pushnumber(L, count -1 );
    lua_rawset(L, -3);

    return 1;
}



/**
 * Mappings between the LUA code and our C code.
 */
static const luaL_reg R[] =
{
    {"bind",		pBind},
    {"connect",		pConnect},
    {"accept",		pAccept},
    {"close",           pClose},
    {"read",		pRead},
    {"write",		pWrite},
    {"is_dir",          pIsDir},
    {"is_file",         pIsFile},
    {"readdir",         pReadDir},
    {NULL,		NULL}
};



/**
 * Bind our exported functions to the Lua intepretter, making our functions
 * available to the calling script.
 * @param L The lua intepreter object.
 * @return 1 on success, 0 on failure.
 */
LUALIB_API int luaopen_libhttpd (lua_State *L)
{
    /* Version number from CVS marker. */
    char *version = getVersion();

    luaL_openlib(L, MYNAME, R, 0);
    lua_pushliteral(L,"version");
    lua_pushstring(L, version );
    lua_settable(L,-3);

    return 1;
}

