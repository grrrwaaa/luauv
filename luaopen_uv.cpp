/*
LuaUV

Copyright 2012 Graham Wakefield. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

extern "C" {
	#include "lua.h"
	#include "lauxlib.h"
	#include "uv.h"
}

#include <string.h>
#include <fcntl.h>

// lightweight C++ wrapper of Lua API:
#include "al_Lua.hpp"

// using type traits to attach names to types:
template<typename T> const char * classname();
template<> const char * classname<uv_loop_t>() { return "uv_loop"; }
template<> const char * classname<uv_timer_t>() { return "uv_timer"; }
template<> const char * classname<uv_file>() { return "uv_file"; }


template<typename T>
T * lua_generic_object(lua_State * L, int idx=1) {
	T * v = (T *)luaL_checkudata(L, idx, classname<T>());
	if (v==0) luaL_error(L, "missing object");
	return v;
}

template<typename T>
int lua_generic___tostring(lua_State * L) {
	T * t = lua_generic_object<T>(L, 1);
	lua_pushfstring(L, "%s(%p)", classname<T>(), t);
	return 1;
}

// grab the loop from the closure:
#define lua_uv_loop(L) (*(uv_loop_t **)luaL_checkudata(L, lua_upvalueindex(1), classname<uv_loop_t>()))

int lua_uv_loop___tostring(lua_State * L) {
	lua_pushfstring(L, "uv_loop (%p)", lua_uv_loop(L));
	return 1;
}

int lua_uv_loop___gc(lua_State * L) {
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	uv_loop_delete(lua_uv_loop(L));
	return 1;
}

#pragma mark types


uv_handle_t * lua_uv_handle_t(lua_State * L, int idx=1) {
	uv_handle_t * v = (uv_handle_t *)lua_touserdata(L, idx);
	if (v==0) luaL_error(L, "missing uv_handle_t");
	return v;
}

int lua_uv_is_active(lua_State * L) {
	uv_handle_t* handle = lua_uv_handle_t(L, 1);
	lua_pushboolean(L, uv_is_active(handle));
	return 1;
}

void lua_uv_close_handle_cb(uv_handle_t* handle) {
	printf("closing handle %p", handle);
	// where does L come from?
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	// remove the reference and let gc continue normally:
	lua_pushlightuserdata(L, handle);
	lua_pushnil(L);
	lua_rawset(L, LUA_REGISTRYINDEX);
	//lua_settable(L, LUA_REGISTRYINDEX);
}

int lua_uv_close_handle(lua_State * L, int idx=1) {
	uv_handle_t * handle = lua_uv_handle_t(L, idx);
	// remove metatable:
	lua_pushnil(L);
	lua_setmetatable(L, idx);
	// push a reference to abort gc:
	lua_pushlightuserdata(L, handle);
	lua_pushvalue(L, idx);
	lua_rawset(L, LUA_REGISTRYINDEX);
	//lua_settable(L, LUA_REGISTRYINDEX);
	// request callback for memory release:
	uv_close(handle, lua_uv_close_handle_cb);
	return 0;
}

#pragma mark requests

//	A reusable template for one-shot requests.
//	Creating a request will allocate memory from Lua
//	The coroutine and userdata are stashed as upvalues of a function in the registry against the request pointer, to prevent garbage collection of any of them while the request is pending.
//	Releasing the request removes the registry link, allowing the closure & upvalues to be collected.

// a dummy function used just to prevent gc of its upvalues
int lua_uv_request_closure(lua_State * L) { return 0; }

// expects the coroutine to already be on the top of the stack:
template<typename T>
T * make_request(lua_State * L) {
	//lua_pushthread(L);	// already there
	T * req = (T *)lua_newuserdata(L, sizeof(T));
	// cache to prevent gc
	// making a closure as a way to store two values against the ptr
	lua_pushcclosure(L, lua_uv_request_closure, 2);
	lua_pushlightuserdata(L, req);
	lua_insert(L, -2);
	// registry[req] = closure(coro, udata):
	lua_rawset(L, LUA_REGISTRYINDEX);	
	// request needs to know the lua_State ptr:
	req->data = L;	
	return req;
}

template<typename T>
void release_request(lua_State * L, T * req) {
	lua_pushlightuserdata(L, req);
	lua_pushnil(L);
	lua_rawset(L, LUA_REGISTRYINDEX);	// registry[req] = nil
}


#pragma mark buffers

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
	buf->base = (char *)(buf + 1);
	buf->len = len;
	
	// TODO: set uv_buf_t metatable
	return buf;
}

int lua_uv_buf_init(lua_State * L) {
	size_t len = luaL_checkinteger(L, -1);
	lua_uv_buf_init(L, len);
	return 1;
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

#pragma mark timers

/*	
	Timer can fire multiple times, so requires a handler function
		function needs to be locked against __gc while timer exists
	uv_timer_t allocated as a Lua userdatum, 
		needs to be locked against __gc while a timer is pending.
	
	uv_timer_t callback can retrieve lua_State from loop,
	Useful to pass the timer userdatum to the handler, for e.g. rescheduling
		reg[uv_timer_t] = udata
		reg[udata] = handler
	
	Another way is to use a C closure:
		reg[uv_timer_t] = closure(handler, udata)
	
*/

int lua_uv_timer_cb_closure(lua_State * L) {
	lua_pushvalue(L, lua_upvalueindex(1)); // handler
	lua_pushvalue(L, lua_upvalueindex(2)); // udata
	lua_call(L, 1, 0);	// pcall?
	return 0;
}

int lua_uv_timer(lua_State * L) {
	luaL_checktype(L, 1, LUA_TFUNCTION);

	uv_loop_t * loop = lua_uv_loop(L);
	uv_timer_t * timer = (uv_timer_t *)lua_newuserdata(L, sizeof(uv_timer_t));
	int udata = lua_gettop(L);
	uv_timer_init(loop, timer);
	luaL_getmetatable(L, "uv_timer");
	lua_setmetatable(L, -2);
	lua_uv_ok(L);
	
	// bind ptr to callback function:
	lua_pushlightuserdata(L, timer);
	lua_pushvalue(L, 1);
	lua_pushvalue(L, udata);
	lua_pushcclosure(L, lua_uv_timer_cb_closure, 2);
	lua_rawset(L, LUA_REGISTRYINDEX);

	return 1;
}

int lua_uv_timer___tostring(lua_State * L) {
	return lua_generic___tostring<uv_timer_t>(L);
}

int lua_uv_timer___gc(lua_State * L) {
	// is it necessary to call uv_timer_stop?
	// close handle (asynchronously):
	lua_uv_close_handle(L, 1);
	return 0;
}

int lua_uv_timer_close(lua_State * L) {
	lua_uv_close_handle(L, 1);
	return 1;
}

void lua_uv_timer_cb(uv_timer_t* handle, int status) {
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	lua_pushlightuserdata(L, handle);
	lua_rawget(L, LUA_REGISTRYINDEX);
	lua_call(L, 0, 0);	// pcall?
}

int lua_uv_timer_start(lua_State * L) {
	uv_timer_t * timer = lua_generic_object<uv_timer_t>(L, 1);
	int64_t timeout = luaL_optinteger(L, 2, 0);
	int64_t repeat = luaL_optinteger(L, 3, 0);
	int result = uv_timer_start(timer, lua_uv_timer_cb, timeout, repeat);
	lua_pushinteger(L, result);
	return 1;
}

int lua_uv_timer_stop(lua_State * L) {
	uv_timer_t * t = lua_generic_object<uv_timer_t>(L, 1);
	printf("stop %d\n", uv_timer_stop(t));
	return 1;
}

#pragma mark filesystem

int lua_uv_string_to_flags(lua_State* L, const char* string) {
	if (strcmp(string, "r") == 0) return O_RDONLY;
	if (strcmp(string, "r+") == 0) return O_RDWR;
	if (strcmp(string, "w") == 0) return O_CREAT | O_TRUNC | O_WRONLY;
	if (strcmp(string, "w+") == 0) return O_CREAT | O_TRUNC | O_RDWR;
	if (strcmp(string, "a") == 0) return O_APPEND | O_CREAT | O_WRONLY;
	if (strcmp(string, "a+") == 0) return O_APPEND | O_CREAT | O_RDWR;
	return luaL_error(L, "Unknown file open flag'%s'", string);
}

uv_file lua_uv_check_file(lua_State * L, int idx=-1) {
	return *(uv_file *)luaL_checkudata(L, idx, classname<uv_file>());
}

int lua_uv_fs___tostring(lua_State * L) {
	uv_file f = lua_uv_check_file(L, 1);
	lua_pushfstring(L, "uv.file(%d)", f);
	return 1;
}

void lua_uv_fs_close_cb(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	int results = 0;
	uv_fs_req_cleanup(req);
	release_request(L, req);
	Lua(L).resume(results);
}

int lua_uv_fs_close(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	uv_file file = lua_uv_check_file(L, 1);
	
	// unset metatable on f:
	lua_pushnil(L);
	lua_setmetatable(L, 1);
	
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_fs_t req;
		int res = uv_fs_close(loop, &req, file, NULL);
		if (res < 0) luaL_error(L, "fs_close (%d)", res);
		lua_uv_ok(L);
		return 0;
	} else {
		// generate a request:
		uv_fs_t * req = make_request<uv_fs_t>(L);
		uv_fs_close(loop, req, file, lua_uv_fs_close_cb);
		return lua_yield(L, 0);
	}
}

int lua_uv_fs___gc(lua_State * L) {
	return lua_uv_fs_close(L);
}

int lua_uv_fs_open_result(lua_State * L, uv_fs_t* req) {
	ssize_t result = req->result;
	if (req->result < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "fs_open error");
		lua_pushinteger(L, req->result);
		return 3;
	}
	// create new userdata containing file:
	*(uv_file *)lua_newuserdata(L, sizeof(uv_file *)) = (uv_file)result;
	luaL_getmetatable(L, "uv_file");
	lua_setmetatable(L, -2);
	return 1;
}

void lua_uv_fs_open_cb(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	int results = lua_uv_fs_open_result(L, req);
	uv_fs_req_cleanup(req);
	release_request(L, req);
	Lua(L).resume(results);
}

int lua_uv_fs_open(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	const char * path = luaL_checkstring(L, 1);
	int flags = lua_uv_string_to_flags(L, luaL_optstring(L, 2, "r"));
	int mode = strtoul(luaL_optstring(L, 3, "0666"), NULL, 8);
	
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_fs_t req;
		uv_fs_open(loop, &req, path, flags, mode, NULL);
		return lua_uv_fs_open_result(L, &req);
	} else {
		// generate a request:
		uv_fs_t * req = make_request<uv_fs_t>(L);
		uv_fs_open(loop, req, path, flags, mode, lua_uv_fs_open_cb);
		return lua_yield(L, 0);
	}
}

void lua_uv_fs_read_cb(uv_fs_t* req) {
	Lua L((lua_State *)req->data);
	uv_buf_t * buf = (uv_buf_t *)lua_touserdata(L, 1);
	lua_pushstring(L, buf->base);
	
	uv_fs_req_cleanup(req);
	delete req;
	
	L.resume(1);
}

int lua_uv_fs_read(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	uv_file file = lua_uv_check_file(L, 1);
	size_t size = (size_t)luaL_optinteger(L, 2, 1024);
	size_t offset = (size_t)luaL_optinteger(L, 3, -1);
	
	// create a buffer & store on the stack:
	uv_buf_t * buf = lua_uv_buf_init(L, size);
	
	// are we on the main thread?
	if (Lua(L).mainthread()) {
		// on main thread; do it immediately:
		uv_fs_t req;
		int res = uv_fs_read(loop, &req, file, buf->base, size, offset, NULL);
		if (res < 0) luaL_error(L, "fs_read (%d)", res);
		lua_uv_ok(L);
		lua_pushstring(L, buf->base);
		return 1;
	} else {
		// in a coroutine, do it asynchronously
		uv_fs_t * req = new uv_fs_t;
		req->data = L;
		int res = uv_fs_read(loop, req, file, buf->base, size, offset, lua_uv_fs_read_cb);
		if (res < 0) luaL_error(L, "fs_read (%d)", res);
		lua_uv_ok(L);
		// buf is on the stack to prevent gc:
		return lua_yield(L, 1);
	}
}

int lua_uv_fs_write_result(lua_State * L, uv_fs_t * req) {
	ssize_t result = req->result;
	if (req->result < 0) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "fs_write error");
		lua_pushinteger(L, req->result);
		return 3;
	}
	lua_pushboolean(L, 1);
	return 1;
}

void lua_uv_fs_write_cb(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	int results = lua_uv_fs_write_result(L, req);
	uv_fs_req_cleanup(req);
	release_request(L, req);
	Lua(L).resume(results);		
}

int lua_uv_fs_write(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	uv_file file = lua_uv_check_file(L, 1);
	const char * buf = luaL_checkstring(L, 2);
	size_t size = strlen(buf);
	size_t offset = (size_t)luaL_optinteger(L, 3, -1);
	
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// on main thread; do it immediately:
		uv_fs_t req;
		int res = uv_fs_write(loop, &req, file, (void *)buf, size, offset, lua_uv_fs_write_cb);
		lua_uv_ok(L);
		return lua_uv_fs_write_result(L, &req);
	} else {
		// generate a request:
		uv_fs_t * req = make_request<uv_fs_t>(L);
		uv_fs_write(loop, req, file, (void *)buf, size, offset, lua_uv_fs_write_cb);
		return lua_yield(L, 0);
	}
}

int lua_uv_fs_readdir_result(lua_State * lua, uv_fs_t* req) {
	Lua L((lua_State *)lua);
	int results = req->result;
	char * buf = (char *)req->ptr;
	while (results--) {
		size_t len = strlen(buf);
		lua_pushlstring(L, buf, len);
		buf += len + 1;
    }
	return req->result;
}

void lua_uv_fs_readdir_cb(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	int results = lua_uv_fs_readdir_result(L, req);
	uv_fs_req_cleanup(req);
	release_request(L, req);
	Lua(L).resume(results);
}

int lua_uv_fs_readdir(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	const char * path = luaL_checkstring(L, 1);
	int flags = luaL_optinteger(L, 2, 0);
	
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_fs_t req;
		uv_fs_readdir(loop, &req, path, flags, NULL);
		return lua_uv_fs_readdir_result(L, &req);
	} else {
		// in a coroutine, do it asynchronously
		uv_fs_t * req = make_request<uv_fs_t>(L);
		uv_fs_readdir(loop, req, path, flags, lua_uv_fs_readdir_cb);
		return lua_yield(L, 0);
	}
}

int lua_uv_fs_stat_result(lua_State * lua, uv_fs_t* req) {
	Lua L((lua_State *)lua);
	struct stat* s;
	if (req->result < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "fs_stat error");
		lua_pushinteger(L, req->result);
		return 3;
	}
	s = (struct stat *)req->ptr;
	lua_newtable(L);
	#if _WIN32
		L.push((double)s->st_atime); L.setfield(-2, "accessed");
		L.push((double)s->st_mtime); L.setfield(-2, "modified");
	#elif !defined(_POSIX_C_SOURCE) || defined(_DARWIN_C_SOURCE)
		L.push((double)s->st_atimespec.tv_sec); L.setfield(-2, "accessed");
		L.push((double)s->st_mtimespec.tv_sec); L.setfield(-2, "modified");
	#else
		L.push((double)s->st_atim.tv_sec); L.setfield(-2, "accessed");
		L.push((double)s->st_mtim.tv_sec); L.setfield(-2, "modified");
	#endif
	L.push(s->st_size); L.setfield(-2, "size");
	return 1;
}

void lua_uv_fs_stat_cb(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	int results = lua_uv_fs_stat_result(L, req);
	uv_fs_req_cleanup(req);
	release_request(L, req);
	Lua(L).resume(results);	
}

/*
	returns table:
		{ accessed = <seconds>, modified = <seconds>, size = <bytes> }
	or error code: <integer>
*/
int lua_uv_fs_stat(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	const char * path = luaL_checkstring(L, 1);
	
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_fs_t req;
		uv_fs_stat(loop, &req, path, NULL);
		return lua_uv_fs_stat_result(L, &req);
	} else {
		// generate a request:
		uv_fs_t * req = make_request<uv_fs_t>(L);
		uv_fs_stat(loop, req, path, lua_uv_fs_stat_cb);
		return lua_yield(L, 0);
	}
}

int lua_uv_fs_event_do(lua_State * L, uv_fs_event_t* handle, const char* filename, int events, int status) {
	return 0;
}

void lua_uv_fs_event_cb(uv_fs_event_t* req, const char* filename, int events, int status) {
	Lua L((lua_State *)req->data);
	if (status != 0) {
		luaL_error(L, "fs_event status error: %d (%s)", status, filename);
	}

	if (filename) {
		L.push(filename);
	} else {
		L.push();
	}

	switch (events) {
		case UV_RENAME: L.push("rename"); break;
		case UV_CHANGE: L.push("change"); break;
		default: L.push(); break;
    }
	
	L.resume(2);
}

int lua_uv_fs_event_init(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	const char * path = luaL_checkstring(L, 1);
	int flags = luaL_optinteger(L, 2, 0);
	
	// are we on the main thread?
	if (Lua(L).mainthread()) {
		return luaL_error(L, "must be called within a coroutine");
	} else {
		// in a coroutine, do it asynchronously
		uv_fs_event_t * req = new uv_fs_event_t;
		req->data = L;
		int res = uv_fs_event_init(loop, req, path, lua_uv_fs_event_cb, flags);
		if (res < 0) luaL_error(L, "fs_event_init (%d)", res);
		return lua_yield(L, 0);
	}
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


//struct uv_interface_address_s {
//  char* name;
//  int is_internal;
//  union {
//    struct sockaddr_in address4;
//    struct sockaddr_in6 address6;
//  } address;
//};

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
	
	LUA_UV_CLOSURE(uv, fs_open);
	LUA_UV_CLOSURE(uv, fs_readdir);
	LUA_UV_CLOSURE(uv, fs_stat);
	
	// install the non-closure methods:
	luaL_register(L, NULL, loop_methods);
	
	// set metatable on the main loop:
	lua_setmetatable(L, loopidx);	
	
	return 1;
}

// The uv module returns a uv_loop userdatum, which has all the main
// libuv functions within it.
// This ensures that multiple independent lua_States have distinct loops,
// that the loop is properly closed when the module is garbage collected,
// and that the uv_loop_t is available as an upvalue to all functions.
extern "C" int luaopen_uv(lua_State * L) {
	lua_uv_loop_new(L);
	int loopidx = lua_gettop(L);
	
	// define uv_timer metatable
	luaL_newmetatable(L, classname<uv_timer_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	LUA_UV_CLOSURE(uv_timer, __tostring);
	LUA_UV_CLOSURE(uv_timer, __gc);
	LUA_UV_CLOSURE(uv_timer, start);
	LUA_UV_CLOSURE(uv_timer, stop);
	LUA_UV_CLOSURE(uv_timer, close);
	lua_pop(L, 1);
	
	// define uv_file metatable
	luaL_newmetatable(L, classname<uv_file>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	LUA_UV_CLOSURE(uv_fs, __tostring);
	LUA_UV_CLOSURE(uv_fs, __gc);
	LUA_UV_CLOSURE(uv_fs, close);
	LUA_UV_CLOSURE(uv_fs, read);
	LUA_UV_CLOSURE(uv_fs, write);
	lua_pop(L, 1);
	
	return 1;
}
