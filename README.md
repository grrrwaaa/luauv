# LuaUV

A Lua wrapper of some parts of https://github.com/joyent/libuv (the platform abstraction layer used in node.js)

This wrapper aims to be Lua-centric, and coroutine-centric, but is certainly far from comprehensive!

Libuv is callback-oriented, however for those callbacks that will fire only once, luauv will use yield/resume when called from within a coroutine, and be blocking otherwise.

Libuv's event handling loop can be driven manually using uv.run_once(), or can take control of the main thread using uv.run()

### Building

Build on OSX using the Xcode 3 project, or using a lakefile for https://github.com/stevedonovan/Lake

Should be possible to build on Linux using lake, but as yet untested.

### See also

https://github.com/luvit/luvit
https://github.com/bnoordhuis/lua-uv/
https://github.com/ignacio/LuaNode

### License

Released under the same license scheme as Lua (MIT/BSD style)

Copyright 2012 Graham Wakefield. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
