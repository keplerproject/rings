/*
** Rings: Multiple Lua States
** $Id: rings.c,v 1.24 2008/06/30 17:52:31 carregal Exp $
** See Copyright Notice in license.html
*/

#include "string.h"

#include "lua.h"
#include "lualib.h"
#include "lauxlib.h"


#define RINGS_STATE     "rings state"
#define RINGS_TABLENAME "rings"
#define RINGS_ENV       "rings environment"
#define STATE_METATABLE "rings state metatable"
#define RINGS_CACHE     "rings cache"


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


/* The code for copying nested tables comes from the lua-llthreads
 * project. [1]
 *
 * [1] https://github.com/Neopallium/lua-llthreads */

#define MAX_COPY_DEPTH  30

struct copy_state {
  lua_State  *src;
  lua_State  *dst;
  int  has_cache;
  int  cache_idx;
  int  is_arg;
};

static int
copy_table_from_cache(struct copy_state *state, int idx)
{
  void  *ptr;

  /* convert table to pointer for lookup in cache */
  ptr = (void *) lua_topointer(state->src, idx);
  if (ptr == NULL) return 0; /* can't convert pointer */

  /* check if we need to create the cache */
  if (!state->has_cache) {
    lua_newtable(state->dst);
    lua_replace(state->dst, state->cache_idx);
    state->has_cache = 1;
  }

  lua_pushlightuserdata(state->dst, ptr);
  lua_rawget(state->dst, state->cache_idx);
  if (lua_isnil(state->dst, -1)) {
    /* not in cache */
    lua_pop(state->dst, 1);
    /* create new table and add to cache */
    lua_newtable(state->dst);
    lua_pushlightuserdata(state->dst, ptr);
    lua_pushvalue(state->dst, -2);
    lua_rawset(state->dst, state->cache_idx);
    return 0;
  }

  /* found table in cache */
  return 1;
}

static int
copy_one_value(struct copy_state *state, int depth, int idx)
{
  /* Maximum recursive depth */
  if (++depth > MAX_COPY_DEPTH) {
    return luaL_error(state->src, "Hit maximum copy depth (%d > %d).",
                      depth, MAX_COPY_DEPTH);
  }

  /* only support string/number/boolean/nil/table/lightuserdata */
  switch (lua_type(state->src, idx)) {
    case LUA_TNUMBER:
      lua_pushnumber(state->dst, lua_tonumber(state->src, idx));
      break;
    case LUA_TBOOLEAN:
      lua_pushboolean(state->dst, lua_toboolean(state->src, idx));
      break;
    case LUA_TSTRING: {
      size_t length;
      const char *string = lua_tolstring(state->src, idx, &length);
      lua_pushlstring(state->dst, string, length);
      break;
    }
    case LUA_TLIGHTUSERDATA: {
      lua_pushlightuserdata(state->dst, lua_touserdata(state->src, idx));
      break;
    }
    case LUA_TNIL:
      lua_pushnil(state->dst);
      break;
    case LUA_TTABLE:
      /* make sure there is room on the new state for 3 values
       * (table,key,value) */
      if (!lua_checkstack(state->dst, 3)) {
        return luaL_error(state->src, "To stack overflow!");
      }
      /* make room on from stack for key/value pairs */
      luaL_checkstack(state->src, 2, "From stack overflow");

      /* check cache for table */
      if (copy_table_from_cache(state, idx)) {
        /* found in cache; don't need to copy table */
        break;
      }

      lua_pushnil(state->src);
      while (lua_next(state->src, idx) != 0) {
        /* key is at (top - 1), value at (top), but we need to normalize
         * these to positive indices */
        int kv_pos = lua_gettop(state->src);
        /* copy key */
        copy_one_value(state, depth, kv_pos - 1);
        /* copy value */
        copy_one_value(state, depth, kv_pos);
        /* Copied key and value are now at -2 and -1 in dest */
        lua_settable(state->dst, -3);
        /* Pop value for next iteration */
        lua_pop(state->src, 1);
      }
      break;

    case LUA_TFUNCTION:
    case LUA_TUSERDATA:
    case LUA_TTHREAD:
    default:
      if (state->is_arg) {
        return luaL_argerror
          (state->src, idx, "function/userdata/thread types unsupported");
      } else {
        /* convert unsupported types to an error string */
        lua_pushfstring(state->dst, "Unsupported value: %s: %p",
          lua_typename(state->src, lua_type(state->src, idx)),
          lua_topointer(state->src, idx));
      }
  }

  return 1;
}

/*
** Copies values from State src to State dst.
*/
static void
copy_values(lua_State *dst, lua_State *src, int i, int top, int is_arg)
{
  struct copy_state  state;
  luaL_checkstack(dst, top - i + 1, "To stack overflow!");

  /* setup copy state */
  state.src = src;
  state.dst = dst;
  state.is_arg = is_arg;
  state.has_cache = 0;
  lua_pushnil(dst);
  state.cache_idx = lua_gettop(dst);

  for (; i <= top; i++) {
    copy_one_value(&state, 0, i);
  }

  /* remove cache table */
  lua_remove(dst, state.cache_idx);
}


/*
** Obtains a function which is the compiled string in the given state.
** It also caches the resulting function to optimize future uses.
** Leaves the compiled function on top of the stack or the error message
** produced by luaL_loadbuffer.
*/
static int compile_string (lua_State *L, void *cache, const char *str) {
  if(cache == NULL) {
    lua_pushliteral(L, RINGS_CACHE);
  } else {
    lua_pushlightuserdata(L, cache);
  }
  lua_gettable (L, LUA_REGISTRYINDEX); /* push cache table */
  lua_pushstring (L, str);
  lua_gettable (L, -2); /* cache[str] */
  if (!lua_isfunction (L, -1)) {
    int status;
    lua_pop (L, 1); /* remove cache[str] (= nil) from top of the stack */
    status = luaL_loadbuffer (L, str, strlen(str), str); /* Compile */
    if (status != 0) { /* error? */
      lua_remove (L, -2); /* removes cache table; leaves the error message */
      return status;
    }
    /* Sets environment */
    lua_pushliteral(L, RINGS_ENV);
    lua_gettable(L, LUA_REGISTRYINDEX);
    if(cache == NULL) {
      lua_pushliteral(L, RINGS_CACHE);
    } else {
      lua_pushlightuserdata(L, cache);
    }
    lua_gettable(L, -2);
    if(!lua_isnil(L, -1)) {
      lua_setfenv(L, -3);
      lua_pop(L, 1);
    } else lua_pop(L, 2);
    /* Stores the produced function at cache[str] */
    lua_pushstring (L, str);
    lua_pushvalue (L, -2);
    lua_settable (L, -4); /* cache[str] = func */
  }
  lua_remove (L, -2); /* removes cache table; leaves the function */
  return 0;
}


/*
** Executes a string of code from State src into State dst.
** idx is the index of the string of code.
*/
static int dostring (lua_State *dst, lua_State *src, void *cache, int idx) {
  int base;
  const char *str = luaL_checkstring (src, idx);
  lua_pushliteral(dst, "rings_traceback");
  lua_gettable(dst, LUA_REGISTRYINDEX);
  base = lua_gettop (dst);
  idx++; /* ignore first argument (string of code) */
  if (compile_string (dst, cache, str) == 0) { /* Compile OK? => push function */
    int arg_top = lua_gettop (src);
    copy_values (dst, src, idx, arg_top, 1); /* Push arguments to dst stack */
    if (lua_pcall (dst, arg_top-idx+1, LUA_MULTRET, base) == 0) { /* run OK? */
      int ret_top = lua_gettop (dst);
      lua_pushboolean (src, 1); /* Push status = OK */
      copy_values (src, dst, base+1, ret_top, 0); /* Return values to src */
      lua_pop (dst, ret_top - base+1);
      return 1+(ret_top-base); /* Return true (success) plus return values */
    }
  }
  lua_pushboolean (src, 0); /* Push status = ERR */
  lua_pushstring (src, lua_tostring (dst, -1));
  lua_pop (dst, 2); /* pops debug.traceback and result from dst state */
  return 2;
}


/*
** Executes a string of Lua code in the master state.
*/
static int master_dostring (lua_State *S) {
  lua_State *M;
  void *C;
  lua_pushliteral(S, RINGS_STATE);
  lua_gettable(S, LUA_REGISTRYINDEX);
  M = (lua_State *)lua_touserdata (S, -1);
  lua_pop(S, 1);
  C = lua_touserdata (S, lua_upvalueindex (1));
  return dostring (M, S, C, 1);
}


/*
** Executes a string of Lua code in a given slave state.
*/
static int slave_dostring (lua_State *M) {
  state_data *s = getstate (M); /* S == s->L */
  lua_pushliteral(s->L, RINGS_STATE);
  lua_pushlightuserdata(s->L, M);
  lua_settable(s->L, LUA_REGISTRYINDEX);
  return dostring (s->L, M, NULL, 2);
}


/*
** Creates a weak table in the registry.
*/
static void create_cache (lua_State *L) {
  lua_newtable (L);
  lua_newtable (L); /* cache metatable */
  lua_pushliteral (L, "__mode");
  lua_pushliteral (L, "v");
  lua_settable (L, -3); /* metatable.__mode = "v" */
  lua_setmetatable (L, -2);
  lua_settable (L, LUA_REGISTRYINDEX);
}


/*
** Creates a new Lua State and returns an userdata that represents it.
*/
static int state_new (lua_State *L) {
  state_data *s;
  if(lua_gettop(L) == 0) {
    lua_getglobal(L, "_M");
    if(lua_isnil(L, 1)) {
      lua_settop(L, 0);
      lua_getglobal(L, "_G");
      if(lua_isnil(L, 1)) {
	lua_settop(L, 0);
        lua_newtable(L);
      }
    }
  }
  s = (state_data *)lua_newuserdata (L, sizeof (state_data));
  if(s == NULL) {
    lua_pushliteral(L, "rings: could not create state data"); 
    lua_error(L);
  }
  s->L = NULL;
  luaL_getmetatable (L, STATE_METATABLE);
  lua_setmetatable (L, -2);
  s->L = lua_open ();
  if(s->L == NULL) {
    lua_pushliteral(L, "rings: could not create new state");
    lua_error(L);
  }

  /* Initialize environment */

  lua_pushliteral(L, RINGS_ENV);
  lua_gettable(L, LUA_REGISTRYINDEX);
  lua_pushlightuserdata(L, s->L);
  lua_pushvalue(L, 1);
  lua_settable(L, -3);
  lua_pop(L, 1);
 
   /* load base libraries */
  luaL_openlibs(s->L);

  /* define dostring function (which runs strings on the master state) */
  lua_pushliteral (s->L, "remotedostring");
  lua_pushlightuserdata (s->L, s->L);
  lua_pushcclosure (s->L, master_dostring, 1);
  lua_settable (s->L, LUA_GLOBALSINDEX);

  /* fetches debug.traceback to registry */
  lua_getglobal(s->L, "debug");
  lua_pushliteral(s->L, "traceback");
  lua_gettable(s->L, -2);
  lua_pushliteral(s->L, "rings_traceback");
  lua_pushvalue(s->L, -2);
  lua_settable(s->L, LUA_REGISTRYINDEX);

  /* Create caches */
  lua_pushlightuserdata(L, s->L);
  create_cache (L);
  lua_pushliteral(s->L, RINGS_CACHE);
  create_cache (s->L);
  lua_pushliteral(s->L, RINGS_ENV);
  create_cache (s->L);

  return 1;
}


/*
** Closes a Lua State.
** Returns `true' in case of success; `nil' when the state was already closed.
*/
static int slave_close (lua_State *L) {
	state_data *s = (state_data *)luaL_checkudata (L, 1, STATE_METATABLE);
	luaL_argcheck (L, s != NULL, 1, "not a Lua State");
	if (s->L != NULL) {
	  lua_pushliteral(L, RINGS_ENV);
	  lua_gettable(L, LUA_REGISTRYINDEX);
	  lua_pushlightuserdata(L, s->L);
	  lua_pushnil(L);
	  lua_settable(L, -3);
	  lua_close (s->L);
	  s->L = NULL;
	}
	return 0;
}


/*
** Creates the metatable for the state on top of the stack.
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
	luaL_register(L, NULL, methods);
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
**
*/
static void set_info (lua_State *L) {
	lua_pushliteral (L, "_COPYRIGHT");
	lua_pushliteral (L, "Copyright (C) 2006 Kepler Project");
	lua_settable (L, -3);
	lua_pushliteral (L, "_DESCRIPTION");
	lua_pushliteral (L, "Rings: Multiple Lua States");
	lua_settable (L, -3);    lua_pushliteral (L, "_VERSION");
	lua_pushliteral (L, "Rings 1.2.2");
	lua_settable (L, -3);
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
	luaL_register(L, RINGS_TABLENAME, rings);
    lua_pushliteral(L, RINGS_ENV);
	lua_newtable (L);
	lua_settable (L, LUA_REGISTRYINDEX);
	set_info (L);

	/* fetches debug.traceback to registry */
	lua_getglobal(L, "debug");
	if(!lua_isnil(L, -1)) {
	  lua_pushliteral(L, "traceback");
	  lua_gettable(L, -2);
	  lua_pushliteral(L, "rings_traceback");
	  lua_pushvalue(L, -2);
	  lua_settable(L, LUA_REGISTRYINDEX);
	  lua_pop(L, 2);
	} else {
	  lua_pop(L, 1);
	}

	return 1;
}
