#include "luaopen_uv.h"

int lua_uv_handle_is_active(lua_State * L) {
	uv_handle_t* handle = lua_generic_object<uv_handle_t>(L, 1);
	lua_pushboolean(L, uv_is_active(handle));
	return 1;
}

void lua_uv_close_handle_cb(uv_handle_t* handle) {
	//printf("closing handle %p", handle);
	// where does L come from?
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	// remove the reference and let gc continue normally:
	lua_pushlightuserdata(L, handle);
	lua_pushnil(L);
	lua_rawset(L, LUA_REGISTRYINDEX);	// reg[handle] = nil
}

int lua_uv_handle_close(lua_State * L) {
	uv_handle_t * handle = lua_generic_object<uv_handle_t>(L, 1);
//	if (uv_is_active(handle)) {
//		luaL_error(L, "cannot close active handle %p", handle);
//	}	
	printf("closing handle (active %d)\n", uv_is_active(handle));
	// remove metatable:
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	// push a reference to abort gc:
	lua_pushlightuserdata(L, handle);
	lua_pushvalue(L, 1);
	lua_rawset(L, LUA_REGISTRYINDEX);	// reg[handle] = udata
	// request callback for memory release:
	uv_close(handle, lua_uv_close_handle_cb);
	return 0;
}

int lua_uv_handle___gc(lua_State * L) {
	printf("__gc handle\n");
	lua_uv_handle_close(L);
	return 0;
}

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME); 
	
void init_handle_metatable(lua_State * L, int loopidx) {
	// define uv_handle_t metatable
	luaL_newmetatable(L, classname<uv_handle_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_handle_t>);
	lua_setfield(L, -2, "__tostring");
	LUA_UV_CLOSURE(uv_handle, __gc);
	LUA_UV_CLOSURE(uv_handle, close);
	LUA_UV_CLOSURE(uv_handle, is_active);
	lua_pop(L, 1);
}
