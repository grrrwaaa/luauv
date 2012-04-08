/*
LuaUV

Copyright 2012 Graham Wakefield. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "luaopen_uv.h"

#include <string.h>

int lua_uv_loop___tostring(lua_State * L) {
	lua_pushfstring(L, "uv_loop (%p)", lua_uv_loop(L));
	return 1;
}

int lua_uv_loop___gc(lua_State * L) {
	printf("lua_uv_loop___gc\n");
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	uv_loop_delete(lua_uv_loop(L));
	return 1;
}

#pragma mark buffers

uv_buf_t lua_uv_alloc(uv_handle_t *, size_t suggested_size) {
	uv_buf_t buf;
	buf.base = (char *)malloc(suggested_size);
	buf.len = suggested_size;
	return buf;
}

/*
	expects an integer size on the top of the stack
	
	allocates a userdata whose contents are 
	uv_buf_t, char[len]
	uv_buf_t.base points to char[len]
	
	Since this memory is managed by Lua, it is user responsibility to keep
	it referenced on the stack by any callbacks that use it
*/
uv_buf_t * lua_uv_buf_init(lua_State * L, size_t len) {
	uv_buf_t * buf = (uv_buf_t *)lua_newuserdata(L, sizeof(uv_buf_t) + len);
	buf->base = (char *)(buf + sizeof(uv_buf_t));
	buf->len = len;
	
	luaL_getmetatable(L, classname<uv_buf_t>());
	lua_setmetatable(L, -2);
	return buf;
}

uv_buf_t * lua_uv_buf_init(lua_State * L, char * data, size_t len) {
	uv_buf_t * buf = lua_uv_buf_init(L, len);
	memcpy(buf->base, data, len);
	return buf;
}

int lua_uv_buf___gc(lua_State * L) {
	printf("buf gc\n");
	return 0;
}

#pragma mark loops


//int lua_uv_loop_new(lua_State * L) {
//	lua_pushlightuserdata(L, uv_loop_new());
//	return 1;
//}
//
//int lua_uv_loop_delete(lua_State * L) {
//	uv_loop_t * v = (uv_loop_t *)lua_touserdata(L, 1);
//	uv_loop_delete(v);
//	return 0;
//}

int lua_uv_run(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	lua_pushinteger(L, uv_run(v));
	return 1;
}

int lua_uv_run_once(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	lua_pushinteger(L, uv_run_once(v));
	return 1;
}

int lua_uv_now(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	lua_pushinteger(L, uv_now(v));
	return 1;
}

int lua_uv_update_time(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	uv_update_time(v);
	return 0;
}

int lua_uv_unref(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	uv_unref(v);
	return 0;
}

#pragma mark errors

bool lua_uv_check(lua_State * L, uv_err_t err) {
	bool ok = err.code == 0;
	lua_pushboolean(L, ok);
	lua_pushstring(L, uv_strerror(err));
	return ok;
}

void lua_uv_ok(lua_State * L, uv_err_t err) {
	if (err.code) {
		lua_pushinteger(L, err.code);
		lua_pushfstring(L, "uv error (%d): %s", err.code, uv_strerror(err) );
		lua_error(L);
	}
}

void lua_uv_ok(lua_State * L) {
	lua_uv_ok(L, uv_last_error(lua_uv_loop(L)));
}

int lua_uv_last_error(lua_State * L) {
	uv_loop_t * v = lua_uv_loop(L);
	uv_err_t err = uv_last_error(v);
	lua_pushinteger(L, err.code);
	lua_pushstring(L, uv_strerror(err));
	lua_pushstring(L, uv_err_name(err));
	return 3;
}

#pragma mark utils

int lua_uv_exepath(lua_State * L) {
	size_t s(4096);
	char path[s + 1];
	uv_exepath(path, &s);
	lua_pushlstring(L, path, s);
	return 1;
}

int lua_uv_cwd(lua_State * L) {
	size_t s(4096);
	char path[s + 1];
	lua_uv_ok(L, uv_cwd(path, s));
	lua_pushstring(L, path);
	return 1;
}

int lua_uv_chdir(lua_State * L) {
	lua_uv_ok(L, uv_chdir(luaL_checkstring(L, 1)));
	return 0;
}

int lua_uv_get_free_memory(lua_State * L) {
	lua_pushinteger(L, uv_get_free_memory());
	return 1;
}
int lua_uv_get_total_memory(lua_State * L) {
	lua_pushinteger(L, uv_get_total_memory());
	return 1;
}

/*
	In UNIX computing, the system load is a measure of the amount of work that a computer system performs. The load average represents the average system load over a period of time. It conventionally appears in the form of three numbers which represent the system load during the last one-, five-, and fifteen-minute periods.
*/
int lua_uv_loadavg(lua_State* L) {
	double avg[3];
	uv_loadavg(avg);
	lua_pushnumber(L, avg[0]);
	lua_pushnumber(L, avg[1]);
	lua_pushnumber(L, avg[2]);
	return 3;
}

int lua_uv_uptime(lua_State* L) {
	double uptime;
	uv_uptime(&uptime);
	lua_pushnumber(L, uptime);
	return 1;
}

int lua_uv_cpu_info(lua_State* L) {
	uv_cpu_info_t* cpu_infos;
	int count;
	uv_cpu_info(&cpu_infos, &count);
	
	
	for (int i = 0; i < count; i++) {
		lua_newtable(L);
		lua_pushstring(L, (cpu_infos[i]).model);
		lua_setfield(L, -2, "model");
		lua_pushnumber(L, (cpu_infos[i]).speed);
		lua_setfield(L, -2, "speed");
		
		lua_newtable(L);
			lua_pushnumber(L, (cpu_infos[i]).cpu_times.user);
			lua_setfield(L, -2, "user");
			lua_pushnumber(L, (cpu_infos[i]).cpu_times.nice);
			lua_setfield(L, -2, "nice");
			lua_pushnumber(L, (cpu_infos[i]).cpu_times.sys);
			lua_setfield(L, -2, "sys");
			lua_pushnumber(L, (cpu_infos[i]).cpu_times.idle);
			lua_setfield(L, -2, "idle");
			lua_pushnumber(L, (cpu_infos[i]).cpu_times.irq);
			lua_setfield(L, -2, "irq");
		lua_setfield(L, -2, "times");
	}
	uv_free_cpu_info(cpu_infos, count);
	return count;
}

int lua_uv_interface_addresses(lua_State * L) {
	int count;
	uv_interface_address_t * addresses;
	char address[INET6_ADDRSTRLEN] = "0.0.0.0";
	lua_uv_check(L, uv_interface_addresses(&addresses, &count));

	for (int i=0; i<count; i++) {
		lua_newtable(L);
		lua_pushstring(L, addresses[i].name);
		lua_setfield(L, -2, "interface");
		lua_pushboolean(L, addresses[i].is_internal);
		lua_setfield(L, -2, "internal");
		
		if (addresses[i].address.address4.sin_family == AF_INET) {
			lua_pushstring(L, "IPv4"); lua_setfield(L, -2, "family");
			uv_ip4_name(&addresses[i].address.address4, address, sizeof(address));
		} else if (addresses[i].address.address4.sin_family == AF_INET6) {
			lua_pushstring(L, "IPv6"); lua_setfield(L, -2, "family");
			uv_ip6_name(&addresses[i].address.address6, address, sizeof(address));
		}
		lua_pushstring(L, address); lua_setfield(L, -2, "address");
	}
	uv_free_interface_addresses(addresses, count);
	return count;
}

int lua_uv_hrtime(lua_State * L) {
	lua_pushnumber(L, uv_hrtime());
	return 1;
}

int lua_uv_get_process_title(lua_State * L) {
	char buf[1024];
	lua_uv_check(L, uv_get_process_title(buf, 1024));
	lua_pushstring(L, buf);
	return 1;
}

int lua_uv_loop_new(lua_State * L) {
	struct luaL_reg loop_methods[] = {
		// utilities
		{ "exepath", lua_uv_exepath },
		{ "cwd", lua_uv_cwd },
		{ "chdir", lua_uv_chdir },
		{ "interface_addresses", lua_uv_interface_addresses },
		{ "get_free_memory", lua_uv_get_free_memory },
		{ "get_total_memory", lua_uv_get_total_memory },
		{ "loadavg", lua_uv_loadavg },
		{ "uptime", lua_uv_uptime },
		{ "cpu_info", lua_uv_cpu_info },
		{ "hrtime", lua_uv_hrtime },
		{ "get_process_title", lua_uv_get_process_title },
		
		{ NULL, NULL },
	};

	// create a new uv_loop_t for this lua_State
	uv_loop_t * loop = uv_loop_new();
	
	// cache the lua_State in the loop
	// (assumes that the state that calls require "uv" is the root state
	// or otherwise safe to use for general callbacks)
	loop->data = L;	
	
	// box it in a userdatum:
	*(uv_loop_t **)lua_newuserdata(L, sizeof(uv_loop_t *)) = loop;
	int loopidx = lua_gettop(L);
	
	// bind loop as an upvalue of all the methods:
	#define LUA_UV_CLOSURE(PREFIX, NAME) \
		lua_pushvalue(L, loopidx); \
		lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
		lua_setfield(L, -2, #NAME); 
	
	// define uv_loop metatable
	luaL_newmetatable(L, classname<uv_loop_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	LUA_UV_CLOSURE(uv_loop, __tostring);
	LUA_UV_CLOSURE(uv_loop, __gc);
	
	LUA_UV_CLOSURE(uv, now);
	LUA_UV_CLOSURE(uv, update_time);
	LUA_UV_CLOSURE(uv, run);
	LUA_UV_CLOSURE(uv, run_once);
	
	LUA_UV_CLOSURE(uv, unref);

	LUA_UV_CLOSURE(uv, last_error);
	
	LUA_UV_CLOSURE(uv, timer);
	LUA_UV_CLOSURE(uv, tcp);
	LUA_UV_CLOSURE(uv, tty);
	
	LUA_UV_CLOSURE(uv, fs_open);
	LUA_UV_CLOSURE(uv, fs_readdir);
	LUA_UV_CLOSURE(uv, fs_stat);
	
	// install the non-closure methods:
	luaL_register(L, NULL, loop_methods);
	
	// set metatable on the main loop:
	lua_setmetatable(L, loopidx);	
	
	return 1;
}

uv_loop_t * lua_uv_main_loop(lua_State * L) {
	lua_getfield(L, LUA_REGISTRYINDEX, "uv_main_loop");
	uv_loop_t * loop = (*(uv_loop_t **)luaL_checkudata(L, -1, classname<uv_loop_t>()));
	return loop;
}

int lua_uv_request___gc(lua_State * L) {
	printf("request __gc\n");
	return 0;
}	

int lua_test_gc(lua_State * L) {
	printf("test gc\n");
	return 0;
}

int lua_test_cb(lua_State * L) {
	// nothing spesh
	return 0;
}

// The uv module returns a uv_loop userdatum, which has all the main
// libuv functions within it.
// This ensures that multiple independent lua_States have distinct loops,
// that the loop is properly closed when the module is garbage collected,
// and that the uv_loop_t is available as an upvalue to all functions.
extern "C" int luaopen_uv(lua_State * L) {
	
	// a quick test:
	luaL_newmetatable(L, "TEST");
	lua_pushcfunction(L, lua_test_gc); lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	
	lua_pushlightuserdata(L, L);
	lua_newuserdata(L, 8);
	luaL_getmetatable(L, "TEST");
	lua_setmetatable(L, -2);
	lua_pushcclosure(L, lua_test_cb, 1);
	lua_settable(L, LUA_REGISTRYINDEX);
	
	
	
	
	
	lua_uv_loop_new(L);
	int loopidx = lua_gettop(L);
	
	// stick this one in the registry:
	lua_pushvalue(L, loopidx);
	lua_setfield(L, LUA_REGISTRYINDEX, "uv_main_loop");
	
	// define uv_buf metatable
	luaL_newmetatable(L, classname<uv_buf_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_buf_t>);
	lua_setfield(L, -2, "__tostring");
	LUA_UV_CLOSURE(uv_buf, __gc);
	lua_pop(L, 1);
	
	// order is important:
	init_handle_metatable(L, loopidx);
	init_stream_metatable(L, loopidx);
	init_timer_metatable(L, loopidx);
	init_tcp_metatable(L, loopidx);
	init_tty_metatable(L, loopidx);
	init_fs_metatable(L, loopidx);
	
	luaL_newmetatable(L, "uv.request");
	lua_pushcfunction(L, lua_uv_request___gc);
	lua_setfield(L, -2, "__gc");
	lua_pop(L, 1);
	
	return 1;
}
