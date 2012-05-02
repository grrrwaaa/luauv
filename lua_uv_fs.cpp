#include "luaopen_uv.h"
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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

void lua_uv_fs_close_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	uv_fs_req_cleanup(req);
	callback_resume_after(L);
	Lua(L).resume(0);
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
		uv_fs_close(loop, &req, file, NULL);
		lua_uv_ok(L);
		return 0;
	} else {
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 1);
		uv_fs_close(loop, req, file, lua_uv_fs_close_resume);
		return lua_yield(L, 2);
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
		uv_fs_req_cleanup(req);	
		return 3;
	}
	// create new userdata containing file:
	*(uv_file *)lua_newuserdata(L, sizeof(uv_file *)) = (uv_file)result;
	luaL_getmetatable(L, classname<uv_file>());
	lua_setmetatable(L, -2);
	uv_fs_req_cleanup(req);
	return 1;
}

void lua_uv_fs_open_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	callback_resume_after(L);
	
	Lua(L).resume(lua_uv_fs_open_result(L, req));
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
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 0);
		uv_fs_open(loop, req, path, flags, mode, lua_uv_fs_open_resume);
		return lua_yield(L, 1);
	}
}

void lua_uv_fs_read_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	uv_buf_t * buf = (uv_buf_t *)lua_touserdata(L, 2);
	
	callback_resume_after(L);
	
	lua_pushstring(L, buf->base);
	uv_fs_req_cleanup(req);
	
	Lua(L).resume(1);
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
		uv_fs_read(loop, &req, file, buf->base, size, offset, NULL);
		lua_uv_ok(L);
		lua_pushstring(L, buf->base);
		return 1;
	} else {
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 1);
		uv_fs_read(loop, req, file, buf->base, size, offset, lua_uv_fs_read_resume);
		return lua_yield(L, 2);
	}
}

int lua_uv_fs_write_result(lua_State * L, uv_fs_t * req) {
	ssize_t result = req->result;
	if (req->result < 0) {
		lua_pushboolean(L, 0);
		lua_pushstring(L, "fs_write error");
		lua_pushinteger(L, req->result);
		uv_fs_req_cleanup(req);
		return 3;
	}
	lua_pushboolean(L, 1);
	lua_pushinteger(L, result);
	uv_fs_req_cleanup(req);
	return 2;
}

void lua_uv_fs_write_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	
	callback_resume_after(L);
	
	Lua(L).resume(lua_uv_fs_write_result(L, req));
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
		uv_fs_write(loop, &req, file, (void *)buf, size, offset, NULL);
		lua_uv_ok(L);
		return lua_uv_fs_write_result(L, &req);
	} else {
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 1);
		uv_fs_write(loop, req, file, (void *)buf, size, offset, lua_uv_fs_write_resume);
		return lua_yield(L, 2);
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
	uv_fs_req_cleanup(req);
	return results;
}

void lua_uv_fs_readdir_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	
	callback_resume_after(L);
	
	Lua(L).resume(lua_uv_fs_readdir_result(L, req));
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
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 1);
		uv_fs_readdir(loop, req, path, flags, lua_uv_fs_readdir_resume);
		return lua_yield(L, 2);
	}
}

int lua_uv_fs_stat_result(lua_State * lua, uv_fs_t* req) {
	Lua L((lua_State *)lua);
	struct stat* s;
	if (req->result < 0) {
		lua_pushnil(L);
		lua_pushstring(L, "fs_stat error");
		lua_pushinteger(L, req->result);
		uv_fs_req_cleanup(req);
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
	uv_fs_req_cleanup(req);
	return 1;
}

void lua_uv_fs_stat_resume(uv_fs_t* req) {
	lua_State * L = (lua_State *)req->data;
	
	callback_resume_after(L);

	Lua(L).resume(lua_uv_fs_stat_result(L, req));	
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
		uv_fs_t * req = callback_resume_before<uv_fs_t>(L, 1);
		uv_fs_stat(loop, req, path, lua_uv_fs_stat_resume);
		return lua_yield(L, 2);
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

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME);

void init_fs_metatable(lua_State * L, int loopidx) {
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
}
