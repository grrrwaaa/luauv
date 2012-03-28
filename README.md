# LuaUV

A Lua wrapper of https://github.com/joyent/libuv

This wrapper aims to be Lua-centric, and coroutine-centric.

Libuv is callback-oriented, however for those callbacks that will fire only once, luauv will use yield/resume when called from within a coroutine, and be blocking otherwise.

Libuv's event handling loop can be driven manually using uv.run_once(), or can take control of the main thread using uv.run()

### See also

https://github.com/luvit/luvit
https://github.com/bnoordhuis/lua-uv/
https://github.com/ignacio/LuaNode