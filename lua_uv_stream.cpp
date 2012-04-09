
#include "luaopen_uv.h"

int lua_uv_stream_read_cb_closure(lua_State * L) {
	dprintf("lua_uv_stream_read_cb_closure\n");
	lua_pushvalue(L, lua_upvalueindex(1)); // handler
	lua_insert(L, 1);
	lua_pushvalue(L, lua_upvalueindex(2)); // udata
	lua_insert(L, 2);
	lua_call(L, 3, 0);	// pcall?
	//Lua(L).resume(3);
	return 0;
}

void lua_uv_stream_read_cb(uv_stream_t* handle, ssize_t nread, uv_buf_t buf) {
	dprintf("lua_uv_stream_read_cb\n");
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	
	Lua C(lua_newthread(L));
	
	// get the closure:
	lua_pushlightuserdata(C, handle);
	lua_rawget(C, LUA_REGISTRYINDEX);
	
	// push the buffer:
	if (nread > 0) {
		lua_pushlstring(C, buf.base, nread);
		lua_pushinteger(C, nread);
	} else {
		// if nread == -1, signals EOF.
		lua_pushnil(C);
		lua_pushinteger(C, nread);
	}
	
	//lua_call(C, 2, 0);	// pcall?
	C.resume(2);
}

// read cb may fire multiple times.
int lua_uv_stream_read_start(lua_State * L) {
	uv_stream_t * s = lua_generic_object<uv_stream_t>(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	
	// bind ptr to callback function:
	lua_pushlightuserdata(L, s);
	lua_pushvalue(L, 2);	// func
	lua_pushvalue(L, 1);	// stream
	lua_pushcclosure(L, lua_uv_stream_read_cb_closure, 2);	// closure(func, timer)
	//Lua(L).dump("read start"); 
	// registry[s] = closure(func, stream)
	lua_rawset(L, LUA_REGISTRYINDEX);
	
	uv_read_start(s, lua_uv_alloc, lua_uv_stream_read_cb);
	lua_uv_ok(L);
	lua_settop(L, 1);
	return 1;	// return stream
}

int lua_uv_stream_read_stop(lua_State * L) {
	uv_stream_t * s = lua_generic_object<uv_stream_t>(L, 1);
	uv_read_stop(s);
	return 1;	
}

void lua_uv_stream_write_resume(uv_write_t * req, int status) {
	dprintf("lua_uv_stream_write_resume\n");
	lua_State * L = (lua_State *)req->data;
	
	callback_resume_after(L);	
	
	lua_pushinteger(L, status);
	Lua(L).resume(1);	
}
int lua_uv_stream_write(lua_State * L) {
	uv_stream_t * s = lua_generic_object<uv_stream_t>(L, 1);
	size_t sz;
	const char * str = luaL_checklstring(L, 2, &sz);
	
	if (lua_pushthread(L)) {
		// not in a coroutine.
		// uv_write does not provide a blocking version
		luaL_error(L, "stream writing only permitted within a coroutine");
		return 0;	
	} else {
		uv_write_t * req = callback_resume_before<uv_write_t>(L, 1);
		uv_buf_t * buf = lua_uv_buf_init(L, (char *)str, sz);
		uv_write(req, s, buf, 1, lua_uv_stream_write_resume);
		// keep these on the stack to prevent gc:
		return lua_yield(L, 3);	
	}
}

int lua_uv_stream_listen_cb_closure(lua_State * L) {
	dprintf("lua_uv_stream_listen_cb_closure %p\n", L);
	lua_pushvalue(L, lua_upvalueindex(1)); // handler
	lua_pushvalue(L, lua_upvalueindex(2)); // udata
	lua_call(L, 1, 0);	// pcall?
	//return Lua(L).resume(1);
	return 0;
}

void lua_uv_stream_listen_cb(uv_stream_t* handle, int status) {
	dprintf("lua_uv_stream_listen_cb\n");
	uv_loop_t * loop = handle->loop;
	lua_State * L = (lua_State *)loop->data;
	
	Lua C(lua_newthread(L));
	printf("created coro %p\n", (lua_State *)C);
	
	lua_pushlightuserdata(C, handle);
	lua_rawget(C, LUA_REGISTRYINDEX);	//lua_uv_stream_listen_cb_closure
	//lua_call(L, 0, 0);	// pcall?
	printf("resume %d\n", C.resume(0));
}

// listen for connections:
int lua_uv_stream_listen(lua_State * L) {
	uv_stream_t * s = lua_generic_object<uv_stream_t>(L, 1);
	luaL_checktype(L, 2, LUA_TFUNCTION);
	int backlog = luaL_optinteger(L, 3, 128);
	
	// bind ptr to callback function:
	lua_pushlightuserdata(L, s);
	lua_pushvalue(L, 2);	// func
	lua_pushvalue(L, 1);	// stream
	lua_pushcclosure(L, lua_uv_stream_listen_cb_closure, 2);	// closure(func, timer)
	lua_rawset(L, LUA_REGISTRYINDEX);
	
	uv_listen(s, backlog, lua_uv_stream_listen_cb);
	lua_uv_ok(L);
	return 0;
}

int lua_uv_stream_accept(lua_State * L) {
	uv_stream_t * server = lua_generic_object<uv_stream_t>(L, 1);
	uv_stream_t * client = lua_generic_object<uv_stream_t>(L, 2);
	uv_accept(server, client);
	lua_uv_ok(L);
	return 0;
}

void lua_uv_stream_shutdown_resume(uv_shutdown_t * req, int status) {
	dprintf("lua_uv_stream_shutdown_resume\n");
	lua_State * L = (lua_State *)req->data;
	
	callback_resume_after(L);
	
	lua_pushinteger(L, status);
	Lua(L).resume(1);	
}

int lua_uv_stream_shutdown(lua_State * L) {
	uv_stream_t * s = lua_generic_object<uv_stream_t>(L, 1);
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		luaL_error(L, "must be called from a coroutine");
		// not a coroutine; make a blocking call:
		uv_shutdown_t req;
		uv_shutdown(&req, s, NULL);
		lua_uv_ok(L);
		return 0;
	} else {
		uv_shutdown_t * req = callback_resume_before<uv_shutdown_t>(L, 1);
		uv_shutdown(req, s, lua_uv_stream_shutdown_resume);
		return lua_yield(L, 2);	
	}
}

int lua_uv_stream___gc(lua_State * L) {
	dprintf("lua_uv_stream___gc\n");
	lua_uv_handle_close(L);
	return 0;
}

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME);
	
void init_stream_metatable(lua_State * L, int loopidx) {
	// uv_stream_t metatable:
	luaL_newmetatable(L, classname<uv_stream_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_stream_t>); 
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, lua_uv_stream___gc);
	lua_setfield(L, -2, "__gc");
	LUA_UV_CLOSURE(uv_stream, read_start);
	LUA_UV_CLOSURE(uv_stream, read_stop);
	LUA_UV_CLOSURE(uv_stream, write);
	LUA_UV_CLOSURE(uv_stream, listen);
	LUA_UV_CLOSURE(uv_stream, accept);
	LUA_UV_CLOSURE(uv_stream, shutdown);
	lua_super_metatable(L, -1, classname<uv_handle_t>());
	lua_pop(L, 1);
}
//
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