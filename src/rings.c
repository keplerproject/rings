/*
** Rings
** $Id: rings.c,v 1.1.1.1 2005/12/29 13:09:17 tomas Exp $
*/

#include "string.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"
#include "compat-5.1.h"


#define RINGS_TABLENAME "rings"
#define STATE_METATABLE "state metatable"


typedef struct {
	lua_State *L;
} state_data;


int luaopen_rings (lua_State *L);


/*
** Get a State object from the first call stack position.
*/
static state_data *getstate (lua_State *L) {
	state_data *s = (state_data *)luaL_checkudata (L, 1, STATE_METATABLE);
	luaL_argcheck (L, s != NULL, 1, "not a Lua State");
	luaL_argcheck (L, s->L, 1, "already closed state");
	return s;
}


/*
**
*/
static int state_tostring (lua_State *L) {
	state_data *s = (state_data *)luaL_checkudata (L, 1, STATE_METATABLE);
	lua_pushfstring (L, "Lua State (%p)", s);
	return 1;
}


/*
** Copies values from State src to State dst.
*/
static void copy_values (lua_State *dst, lua_State *src, int i, int top) {
	for (; i <= top; i++) {
		switch (lua_type (src, i)) {
			case LUA_TNUMBER:
				lua_pushnumber (dst, lua_tonumber (src, i));
				break;
			case LUA_TBOOLEAN:
				lua_pushboolean (dst, lua_toboolean (src, i));
				break;
			case LUA_TSTRING: {
				const char *string = lua_tostring (src, i);
				size_t length = lua_strlen (src, i);
				lua_pushlstring (dst, string, length);
				break;
			}
			case LUA_TUSERDATA:
			case LUA_TLIGHTUSERDATA: {
				lua_pushlightuserdata (dst, lua_touserdata (src, i));
				break;
			}
			case LUA_TNIL:
			default:
				lua_pushnil (dst);
				break;
		}
	}
}


/*
**
*/
static void buildargtable (lua_State *L, int n) {
	int t, i;
	lua_pushliteral (L, "arg");
	lua_newtable (L);
	t = lua_gettop (L);
	for (i = 1; i <= n; i++) {
		lua_pushnumber (L, i);
		lua_pushvalue (L, -n+(i-4));
		lua_rawset (L, t);
	}
	lua_settable (L, LUA_GLOBALSINDEX);
	lua_pop (L, n);
}


/*
** Executes a string of code from State src into State dst.
** The arguments from src are stored into table arg; the return values are
** pushed onto the stack.
** p is the index of the first argument on the stack.
*/
static int dostring (lua_State *dst, lua_State *src, const char *str, int p) {
	int status = luaL_loadbuffer (dst, str, strlen(str), str); /* Compile string */
	if (status == 0) { /* parse OK? */
		int base, arg_top = lua_gettop (src);
		copy_values (dst, src, p, arg_top); /* Push arguments to dst stack */
		buildargtable (dst, arg_top-p+1);
		base = lua_gettop (dst);
		status = lua_pcall (dst, 0, LUA_MULTRET, 0);
		if (status == 0) { /* run OK? */
			int ret_top = lua_gettop (dst);
			lua_pushboolean (src, 1); /* Push status = OK */
			copy_values (src, dst, base, ret_top); /* Return values to src */
			lua_pop (dst, ret_top-base+1);
			return 1+(ret_top-base+1); /* Return true (success) plus return values */
		}
	}
	lua_pushboolean (src, 0); /* Push status = ERR */
	lua_pushstring (src, lua_tostring (dst, -1));
	lua_pop (dst, 1); /* pops result from new state */
	return 2;
}


/*
** Executes a string of Lua code in the master state.
*/
static int master_dostring (lua_State *S) {
	lua_State *M = (lua_State *)lua_touserdata (S, lua_upvalueindex (1));
	const char *str = luaL_checkstring (S, 1);
	return dostring (M, S, str, 2);
}


/*
** Executes a string of Lua code in a given slave state.
*/
static int slave_dostring (lua_State *M) {
	state_data *s = getstate (M); /* S == s->L */
	const char *str = luaL_checkstring (M, 2);
	return dostring (s->L, M, str, 3);
}


/*
** Creates a new Lua State and returns an userdata that represents it.
*/
static int state_new (lua_State *L) {
	state_data *s = (state_data *)lua_newuserdata (L, sizeof (state_data));
	s->L = NULL;
	luaL_getmetatable (L, STATE_METATABLE);
	lua_setmetatable (L, -2);
	s->L = lua_open ();

	/* load base and loadlib libraries */
	luaopen_base (s->L);
	lua_settop (s->L, 0);
	luaopen_string (s->L);
	lua_settop (s->L, 0);
	luaopen_math (s->L);
	lua_settop (s->L, 0);
	luaopen_io (s->L);
	lua_settop (s->L, 0);
	luaopen_table (s->L);
	lua_settop (s->L, 0);
	luaopen_debug (s->L);
	lua_settop (s->L, 0);
	luaopen_loadlib (s->L);
	lua_settop (s->L, 0);

	/* define dostring function (which runs strings on the master state) */
	lua_pushliteral (s->L, "dostring");
	lua_pushlightuserdata (s->L, L);
	lua_pushcclosure (s->L, master_dostring, 1);
	lua_settable (s->L, LUA_GLOBALSINDEX);

	return 1;
}


/*
** Closes a Lua State.
** Returns `true' in case of success; `nil' when the state was already closed.
*/
static int slave_close (lua_State *L) {
	state_data *s = (state_data *)luaL_checkudata (L, 1, STATE_METATABLE);
	luaL_argcheck (L, s != NULL, 1, "not a Lua State");
	if (s->L == NULL)
		return 0;
	lua_close (s->L);
	s->L = NULL;
	lua_pushboolean (L, 1);
	return 1;
}


/*
**
*/
static int state_createmetatable (lua_State *L) {
	/* State methods */
	struct luaL_reg methods[] = {
		{"close", slave_close},
		{"dostring", slave_dostring},
		{NULL, NULL},
	};
	/* State metatable */
	if (!luaL_newmetatable (L, STATE_METATABLE)) {
		return 0;
	}
	/* define methods */
	luaL_openlib (L, NULL, methods, 0);
	/* define metamethods */
	lua_pushliteral (L, "__gc");
	lua_pushcfunction (L, slave_close);
	lua_settable (L, -3);

	lua_pushliteral (L, "__index");
	lua_pushvalue (L, -2);
	lua_settable (L, -3);

	lua_pushliteral (L, "__tostring");
	lua_pushcfunction (L, state_tostring);
	lua_settable (L, -3);

	lua_pushliteral (L, "__metatable");
	lua_pushliteral (L, "You're not allowed to get the metatable of a Lua State");
	lua_settable (L, -3);
	return 1;
}


/*
** Opens library.
*/
int luaopen_rings (lua_State *L) {
	/* Library functions */
	struct luaL_reg rings[] = {
		{"new", state_new},
		{NULL, NULL},
	};
	if (!state_createmetatable (L))
		return 0;
	lua_pop (L, 1);
	/* define library functions */
	luaL_openlib (L, RINGS_TABLENAME, rings, 0);

	return 1;
}
