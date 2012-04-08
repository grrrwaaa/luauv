#include "luaopen_uv.h"

#if defined(__MINGW32__) || defined(_MSC_VER)
	#include <inet_net_pton.h>
	#include <inet_ntop.h>
	#define uv_inet_pton ares_inet_pton
	#define uv_inet_ntop ares_inet_ntop
#else /* __POSIX__ */
	#include <arpa/inet.h>
	#define uv_inet_pton inet_pton
	#define uv_inet_ntop inet_ntop
#endif

int lua_uv_tcp(lua_State * L) {
	uv_loop_t * loop = lua_uv_loop(L);
	uv_tcp_t * t = (uv_tcp_t *)lua_newuserdata(L, sizeof(uv_tcp_t));
	uv_tcp_init(loop, t);
	
	luaL_getmetatable(L, classname<uv_tcp_t>());
	lua_setmetatable(L, -2);
	lua_uv_ok(L);
    return 1;
}

int lua_uv_tcp_nodelay(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	int enable = lua_toboolean(L, 2);
	int delay = lua_tointeger(L, 3);
	uv_tcp_keepalive(t, enable, delay);
	lua_uv_ok(L);
	return 0;
}	

int lua_uv_tcp_keepalive(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	int enable = lua_toboolean(L, 2);
	uv_tcp_nodelay(t, enable);
	lua_uv_ok(L);
	return 0;
}	

int lua_uv_tcp_bind(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	const char * host = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	struct sockaddr_in addr = uv_ip4_addr(host, port);
	uv_tcp_bind(t, addr);
	lua_uv_ok(L);
	return 0;
}	

int lua_uv_tcp_bind6(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	const char * host = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	struct sockaddr_in6 addr = uv_ip6_addr(host, port);
	uv_tcp_bind6(t, addr);
	lua_uv_ok(L);
	return 0;
}	

int lua_uv_tcp_getsockname(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	
	struct sockaddr_storage name;
	int namelen = sizeof(name);
	
	uv_tcp_getsockname(t, (struct sockaddr*)(&name), &namelen);
	lua_uv_ok(L);
	
	lua_newtable(L);
	if (name.ss_family == AF_INET) {
		lua_pushstring(L, "IPv4"); 
		lua_setfield(L, -2, "family");
	
		struct sockaddr_in* addr = (struct sockaddr_in*)&name;
		char ip[INET_ADDRSTRLEN];
		uv_inet_ntop(AF_INET, &(addr->sin_addr), ip, INET6_ADDRSTRLEN);
		lua_pushstring(L, ip);
		lua_setfield(L, -2, "address");
		lua_pushnumber(L, ntohs(addr->sin_port));
		lua_setfield(L, -2, "port");
	} else if (name.ss_family == AF_INET6) {
		lua_pushstring(L, "IPv6"); 
		lua_setfield(L, -2, "family");
	
		struct sockaddr_in6* addr = (struct sockaddr_in6*)&name;
		char ip[INET6_ADDRSTRLEN];
		uv_inet_ntop(AF_INET6, &(addr->sin6_addr), ip, INET6_ADDRSTRLEN);
		lua_pushstring(L, ip);
		lua_setfield(L, -2, "address");
		lua_pushnumber(L, ntohs(addr->sin6_port));
		lua_setfield(L, -2, "port");
	}
	
	return 1;
}

int lua_uv_tcp_getpeername(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	
	struct sockaddr_storage name;
	int namelen = sizeof(name);
	
	uv_tcp_getpeername(t, (struct sockaddr*)(&name), &namelen);
	lua_uv_ok(L);
	
	lua_newtable(L);
	if (name.ss_family == AF_INET) {
		lua_pushstring(L, "IPv4"); 
		lua_setfield(L, -2, "family");
	
		struct sockaddr_in* addr = (struct sockaddr_in*)&name;
		char ip[INET_ADDRSTRLEN];
		uv_inet_ntop(AF_INET, &(addr->sin_addr), ip, INET6_ADDRSTRLEN);
		lua_pushstring(L, ip);
		lua_setfield(L, -2, "address");
		lua_pushnumber(L, ntohs(addr->sin_port));
		lua_setfield(L, -2, "port");
	} else if (name.ss_family == AF_INET6) {
		lua_pushstring(L, "IPv6"); 
		lua_setfield(L, -2, "family");
	
		struct sockaddr_in6* addr = (struct sockaddr_in6*)&name;
		char ip[INET6_ADDRSTRLEN];
		uv_inet_ntop(AF_INET6, &(addr->sin6_addr), ip, INET6_ADDRSTRLEN);
		lua_pushstring(L, ip);
		lua_setfield(L, -2, "address");
		lua_pushnumber(L, ntohs(addr->sin6_port));
		lua_setfield(L, -2, "port");
	}
	
	return 1;
}

void lua_uv_tcp_connect_cb(uv_connect_t * req, int status) {
	printf("lua_uv_tcp_connect_cb\n");
	lua_State * L = (lua_State *)req->data;
	release_request(L, req);
	lua_pushinteger(L, status);
	Lua(L).resume(1);
}
int lua_uv_tcp_connect(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	const char * host = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	struct sockaddr_in addr = uv_ip4_addr(host, port);
		
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_connect_t req;
		uv_tcp_connect(&req, t, addr, NULL);
		lua_uv_ok(L);
		return 0;
	} else {
		// generate a request:
		uv_connect_t * req = make_request<uv_connect_t>(L);
		uv_tcp_connect(req, t, addr, lua_uv_tcp_connect_cb);
		// keeps the uv_buf on the stack until the write is completed:
		return lua_yield(L, 1);	
	}
	return 1;	
}
int lua_uv_tcp_connect6(lua_State * L) {
	uv_tcp_t * t = lua_generic_object<uv_tcp_t>(L, 1);
	const char * host = luaL_checkstring(L, 2);
	int port = luaL_checkinteger(L, 3);
	struct sockaddr_in6 addr = uv_ip6_addr(host, port);
		
	// are we in a coroutine?
	if (lua_pushthread(L)) {
		// not a coroutine; make a blocking call:
		uv_connect_t req;
		uv_tcp_connect6(&req, t, addr, NULL);
		lua_uv_ok(L);
		return 0;
	} else {
		// generate a request:
		uv_connect_t * req = make_request<uv_connect_t>(L);
		uv_tcp_connect6(req, t, addr, lua_uv_tcp_connect_cb);
		// keeps the uv_buf on the stack until the write is completed:
		return lua_yield(L, 1);	
	}
	return 1;	
}

int lua_uv_tcp___gc(lua_State * L) {
	printf("lua_uv_tcp___gc\n");
	lua_uv_handle_close(L);
	return 0;
}

// bind loop as an upvalue of all the methods:
#define LUA_UV_CLOSURE(PREFIX, NAME) \
	lua_pushvalue(L, loopidx); \
	lua_pushcclosure(L, lua_##PREFIX##_##NAME, 1); \
	lua_setfield(L, -2, #NAME);

void init_tcp_metatable(lua_State * L, int loopidx) {
	luaL_newmetatable(L, classname<uv_tcp_t>());
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index");
	lua_pushcfunction(L, lua_generic___tostring<uv_tcp_t>); 
	lua_setfield(L, -2, "__tostring");
	lua_pushcfunction(L, lua_uv_tcp___gc);
	lua_setfield(L, -2, "__gc");
	LUA_UV_CLOSURE(uv_tcp, nodelay);
	LUA_UV_CLOSURE(uv_tcp, keepalive);
	LUA_UV_CLOSURE(uv_tcp, bind);
	LUA_UV_CLOSURE(uv_tcp, bind6);
	LUA_UV_CLOSURE(uv_tcp, getsockname);
	LUA_UV_CLOSURE(uv_tcp, getpeername);
	LUA_UV_CLOSURE(uv_tcp, connect);
	LUA_UV_CLOSURE(uv_tcp, connect6);
	lua_super_metatable(L, -1, classname<uv_stream_t>());
	lua_pop(L, 1);
}
