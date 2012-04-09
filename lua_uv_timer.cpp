#include "luaopen_uv.h"

int lua_uv_timer(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	uv_timer_t * timer = (uv_timer_t *)lua_newuserdata(L, sizeof(uv_timer_t));
	uv_timer_init(loop, timer);
	luaL_getmetatable(L, classname<uv_timer_t>());
	lua_setmetatable(L, -2);
	lua_uv_ok(L);

	return 1;
}

int lua_uv_timer_cb_closure(lua_State * L) {
	lua_pushvalue(L, lua_upvalueindex(1)); // handler
	lua_pushvalue(L, lua_upvalueindex(2)); // udata
	lua_call(L, 1, 0);	// pcall?
	return 0;
}
void lua_uv_timer_cb(uv_timer_t* handle, int status) {
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	
	Lua C(lua_newthread(L));
	
	lua_pushlightuserdata(C, handle);
	lua_rawget(C, LUA_REGISTRYINDEX);
	C.resume(0);
	//lua_call(L, 0, 0);	// pcall?
}

int lua_uv_timer_start(lua_State * L) {
	uv_timer_t * timer = lua_generic_object<uv_timer_t>(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	int64_t timeout = luaL_optinteger(L, 3, 0);
	int64_t repeat = luaL_optinteger(L, 4, 0);
	
	// bind ptr to callback function:
	lua_pushlightuserdata(L, timer);
	lua_pushvalue(L, 2);	// func
	lua_pushvalue(L, 1);	// timer
	lua_pushcclosure(L, lua_uv_timer_cb_closure, 2);	// closure(func, timer)
	// warning: this binding will be overwritten by handle:close()
	lua_rawset(L, LUA_REGISTRYINDEX);	// reg[timer] = closure
	
	uv_timer_start(timer, lua_uv_timer_cb, timeout, repeat);
	lua_uv_ok(L);
	lua_settop(L, 1);
	return 1;	// return timer
}

int lua_uv_timer_stop(lua_State * L) {
	uv_timer_t * t = lua_generic_object<uv_timer_t>(L, 1);
	uv_timer_stop(t);
	lua_settop(L, 1);
	return 1;	// return timer
}

int lua_uv_timer_again(lua_State * L) {
	uv_timer_t * t = lua_generic_object<uv_timer_t>(L, 1);
	uv_timer_again(t);
	lua_uv_ok(L);
	lua_settop(L, 1);
	return 1;	// return timer
}

int lua_uv_timer_repeat(lua_State * L) {
	uv_timer_t * t = lua_generic_object<uv_timer_t>(L, 1);
	if (lua_isnumber(L, 2)) {
		uv_timer_set_repeat(t, lua_tonumber(L, 2));
		lua_settop(L, 1);
		return 1;	// return timer
	} else {
		lua_pushnumber(L, uv_timer_get_repeat(t));
		return 1;
	}
}

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME);
	
void init_timer_metatable(lua_State * L, int loopidx) {
	// define uv_timer metatable
	luaL_newmetatable(L, classname<uv_timer_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_timer_t>);
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, lua_uv_handle_close);
	lua_setfield(L, -2, "__gc");
	LUA_UV_CLOSURE(uv_timer, start);
	LUA_UV_CLOSURE(uv_timer, stop);
	LUA_UV_CLOSURE(uv_timer, repeat);
	lua_super_metatable(L, -1, classname<uv_handle_t>());
	lua_pop(L, 1);
}