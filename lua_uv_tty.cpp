
#include "luaopen_uv.h"

int lua_uv_tty(lua_State * L) {
	uv_file fd = luaL_checkint(L, 1);
	int readable = lua_toboolean(L, 2);

	uv_loop_t * loop = lua_uv_loop(L);
	uv_tty_t * tty = (uv_tty_t *)lua_newuserdata(L, sizeof(uv_tty_t));
	uv_tty_init(loop, tty, fd, readable);
	
	luaL_getmetatable(L, classname<uv_tty_t>());
	lua_setmetatable(L, -2);
	lua_uv_ok(L);

    return 1;
}

int lua_uv_tty_set_mode(lua_State * L) {
	uv_tty_t * tty = lua_generic_object<uv_tty_t>(L, 1);
	int mode = luaL_checkinteger(L, 2);
	uv_tty_set_mode(tty, mode);
	lua_uv_ok(L);
	return 0;
}

int lua_uv_tty_reset_mode(lua_State * L) {
	uv_tty_reset_mode();
	return 0;
}

///! Returns character size of TTY shell window
int lua_uv_tty_get_win_size(lua_State * L) {
	uv_tty_t * tty = lua_generic_object<uv_tty_t>(L, 1);
	
	int w, h;
	uv_tty_get_winsize(tty, &w, &h);
	lua_uv_ok(L);
	lua_pushinteger(L, w);
	lua_pushinteger(L, h);
	return 2;
}

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME);

void init_tty_metatable(lua_State * L, int loopidx) {
	// uv_tty_t metatable:
	luaL_newmetatable(L, classname<uv_tty_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_tty_t>); 
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, lua_uv_handle_close);
	lua_setfield(L, -2, "__gc");
	LUA_UV_CLOSURE(uv_tty, set_mode);
	LUA_UV_CLOSURE(uv_tty, reset_mode);
	LUA_UV_CLOSURE(uv_tty, get_win_size);
	// uv_tty_t inherits uv_stream_t:
	lua_super_metatable(L, -1, classname<uv_stream_t>());
	lua_pop(L, 1);
}

//extern "C" int luaopen_uv_tty(lua_State * L) {
//	// don't hold the event loop open for them
//	uv_loop_t * loop = lua_uv_main_loop(L);
//	int loopidx = lua_gettop(L);
//	
//	init_stream_metatable(L, loopidx);
//	init_tty_metatable(L, loopidx);
//		
//	struct luaL_reg lib[] = {
//		{ NULL, NULL }
//	};
//	lua_newtable(L);
//	luaL_register(L, "uv.tty", lib);
//	
//	
//	
//	
//	LUA_UV_CLOSURE(uv_tty, new);
//	
//	// create stdin, stdout:
//	// Tty:new(0)
//	// uv_unref(loop);
//	// uv_unref(loop);
//	
//	return 1;
//}