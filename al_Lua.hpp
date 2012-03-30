#ifndef INC_AL_LUA_HPP
#define INC_AL_LUA_HPP

/*	Copyright (C) 2009. AlloSphere Research Group, Media Arts & Technology, UCSB.
	Copyright (C) 2006-2008. The Regents of the University of California (REGENTS).
	All Rights Reserved.

	Permission to use, copy, modify, distribute, and distribute modified versions
	of this software and its documentation without fee and without a signed
	licensing agreement, is hereby granted, provided that the above copyright
	notice, the list of contributors, this paragraph and the following two paragraphs
	appear in all copies, modifications, and distributions.

	IN NO EVENT SHALL REGENTS BE LIABLE TO ANY PARTY FOR DIRECT, INDIRECT,
	SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST PROFITS, ARISING
	OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, EVEN IF REGENTS HAS
	BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	REGENTS SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
	THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
	PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF ANY, PROVIDED
	HEREUNDER IS PROVIDED "AS IS". REGENTS HAS  NO OBLIGATION TO PROVIDE
	MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


	File description:
	Utilities for working with Lua

	File author(s):
	Graham Wakefield, 2009, grrrwaaa@gmail.com
*/

#include "lua.hpp"
#include <string>

///! A wrapper around the lua_State:
class Lua {
public:

	Lua() : L(0), mOwner(true) {
		L = lua_open();
		luaL_openlibs(L);
	}
	
	Lua(lua_State * L) : L(L), mOwner(false) {}

	///! destructor calls
	~Lua() {
		if (L && mOwner) {
			lua_close(L);
			L = 0;
		}
	}

	///! allow the Lua object to be used in place of a lua_State *
	operator lua_State *() { return L; }
	operator const lua_State *() { return L; }
	
	///! push a value onto the Lua stack:
	template<typename T>
	Lua& push(T v);
	///! push nil onto the Lua stack:
	Lua& push() { lua_pushnil(L); return *this; }
	
	///! get value from Lua stack:
	///! indices from 1 .. top()
	///! negative indices count backwards from -1 == top()
	template<typename T>
	T to(int idx=-1);
	
	///! get number of items on the stack:
	int top() { return lua_gettop(L); }
	
	///! remove N elements from top of stack:
	Lua& pop(int n=1) { lua_pop(L, n); return *this; }
	
	///! set value to be a named global
	template<typename T>
	Lua& setglobal(const std::string& name, T value);
	
	///! set stack top value to be a named global
	/// pops value from stack
	Lua& setglobal(const std::string& name);
	
	Lua& setfield(int idx, const std::string& name);
	Lua& getfield(int idx, const std::string& name);

	///! call function with N args
	/// function should be at stack index (top - n)
	/// catches errors and prints a traceback to stdout
	/// returns 0 if no errors
	int pcall(int n=0);
	
	///! resume as a coroutine with n arguments on the stack:
	/// catches errors and prints a traceback to stdout
	/// returns 0 if no errors
	int resume(int n=0);

	///! runs a string of code
	/// catches errors and prints to stdout
	/// returns 0 if no errors
	int dostring(const std::string& code);

	///! loads and runs code from a file
	/// catches errors and prints to stdout
	/// returns 0 if no errors
	int dofile(const std::string& path);
	
	template<typename T, int (T::*M)(lua_State *)>
	struct Bind {
		static int method(lua_State * L) {
			T * self = (T *)lua_touserdata(L, lua_upvalueindex(1));
			printf("self %p\n", self);
			((self)->*(M))(L);
			return 0;
		}
	};
	
	template <typename T>
	Lua& pushmethod(T * self, int (T::*M)(lua_State *)) {
		lua_pushlightuserdata(L, self);
		printf("self %p, %p", self, &Bind<T, M>::method);
		lua_pushcclosure(L, &Bind<T, M>::method, 1);
		// test it:
		return *this;
	}
	
	void dump(const char * msg="Lua:");
	
	int lerror(int err);
	
	bool mainthread();

protected:
	lua_State * L;
	bool mOwner;
};

template<> inline Lua& Lua::push(const std::string str) { lua_pushstring(L, str.c_str()); return *this; }
template<> inline Lua& Lua::push(const char * str) { lua_pushstring(L, str); return *this; }

template<> inline Lua& Lua::push(float v) { lua_pushnumber(L, v); return *this; }
template<> inline Lua& Lua::push(double v) { lua_pushnumber(L, v); return *this; }
template<> inline Lua& Lua::push(uint16_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(int16_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(uint32_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(int32_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(uint64_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(int64_t v) { lua_pushinteger(L, v); return *this; }
template<> inline Lua& Lua::push(bool v) { lua_pushboolean(L, v); return *this; }
template<> inline Lua& Lua::push(void * v) { lua_pushlightuserdata(L, v); return *this; }

template<> inline std::string Lua::to(int idx) { return lua_tostring(L, idx); }
template<> inline const char * Lua::to(int idx) { return lua_tostring(L, idx); }
template<> inline double Lua::to(int idx) { return lua_tonumber(L, idx); }
template<> inline float Lua::to(int idx) { return lua_tonumber(L, idx); }
template<> inline uint16_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline int16_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline uint32_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline int32_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline uint64_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline int64_t Lua::to(int idx) { return lua_tointeger(L, idx); }
template<> inline bool Lua::to(int idx) { return lua_toboolean(L, idx); }
template<> inline void * Lua::to(int idx) { return lua_touserdata(L, idx); }

inline Lua& Lua::setglobal(const std::string& name) { lua_setglobal(L, name.c_str()); return *this; }

template<typename T>
inline Lua& Lua::setglobal(const std::string& name, T value) {
	return push(value).setglobal(name); 
}


inline Lua& Lua::setfield(int idx, const std::string& name) {
	lua_setfield(L, idx, name.c_str()); return *this;
}

inline Lua& Lua::getfield(int idx, const std::string& name) {
	lua_getfield(L, idx, name.c_str()); return *this;
}

inline int Lua::lerror(int err) {
	if (err) {
		fprintf(stderr, "error: %s\n", lua_tostring(L, -1));
		lua_pop(L, 1);
	}
	return err;
}


inline int Lua::pcall(int nargs) {
	int debug_idx = -nargs-3;
	// put debug.traceback just below the function & args
	{
		lua_getglobal(L, "debug");
		lua_pushliteral(L, "traceback");
		lua_gettable(L, -2);
		lua_insert(L, debug_idx);
		lua_pop(L, 1); // pop debug table
	}
	int res = lerror(lua_pcall(L, nargs, LUA_MULTRET, -nargs-2));
	lua_remove(L, -(res+1)); // remove debug function from stack
	return res;
}

inline int Lua::resume(int nargs) {
	int result = lua_status(L);
	switch(result) {
		case 0:
		case LUA_YIELD:	// == 1
			result = lua_resume(L, nargs); 
			break;
		default:
			break;
	}
	switch (result) {
		case 0:			// coroutine completed or dead
		case LUA_YIELD:	// coroutine paused
			break;
		case LUA_ERRRUN:
			fprintf(stderr, "runtime error: %s\n", lua_tostring(L, -1));
			break;
		case LUA_ERRSYNTAX:
			fprintf(stderr, "syntax error: %s\n", lua_tostring(L, -1));
			break;
		case LUA_ERRMEM:
			fprintf(stderr, "memory error: %s\n", lua_tostring(L, -1));
			break;
		default:
			fprintf(stderr, "unknown error: %s\n", lua_tostring(L, -1));
	}
	return result;
}

inline int Lua::dostring(const std::string& code) {
	return lerror(luaL_loadstring(L, code.c_str())) || pcall(0);
}
inline int Lua::dofile(const std::string& path) {
	return lerror(luaL_loadfile(L, path.c_str())) || pcall(0);
}

inline void Lua::dump(const char * msg) {
	printf("lua_State (%p) %s\n", L, msg);
	int top = lua_gettop(L);
	for (int i=1; i<=top; i++) {
		switch(lua_type(L, i)) {
			case LUA_TNIL:
				printf("%i (-%i): nil\n", i, top+1-i); break;
			case LUA_TBOOLEAN:
				printf("%i (-%i): boolean (%s)\n", i, top+1-i, lua_toboolean(L, i) ? "true" : "false"); break;
			case LUA_TLIGHTUSERDATA:
				printf("%i (-%i): lightuserdata (%p)\n", i, top+1-i, lua_topointer(L, i)); break;
			case LUA_TNUMBER:
				printf("%i (-%i): number (%f)\n", i, top+1-i, lua_tonumber(L, i)); break;
			case LUA_TSTRING:
				printf("%i (-%i): string (%s)\n", i, top+1-i, lua_tostring(L, i)); break;
			case LUA_TUSERDATA:
//				printf("%i (-%i): userdata (%p)\n", i, top+1-i, lua_topointer(L, i)); break;
				lua_getglobal(L, "tostring");
				lua_pushvalue(L, i);
				lua_call(L, 1, 1);
				printf("%i (-%i): %s\n", i, top+1-i, lua_tostring(L, -1));
				lua_pop(L, 1);
				break;
			default:{
				printf("%i (-%i): %s (%p)\n", i, top+1-i, lua_typename(L, lua_type(L, i)), lua_topointer(L, i));
			}
		}
	}
}

inline bool Lua::mainthread() {
	bool res = lua_pushthread(L);
	lua_pop(L, 1);
	return res;
}

#endif
