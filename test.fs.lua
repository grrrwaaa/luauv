uv = require "uv"

local function demo(str)
	local f = loadstring("return "..str)
	print(str, f())
end

demo "uv.exepath()"
demo "uv.cwd()"
demo [[uv.chdir(uv.cwd() .. "/libuv")]]
demo "uv.cwd()"
demo [[uv.chdir(uv.cwd() .. "/..")]]
demo "uv.cwd()"

-- run file IO as a coroutine for asynchronous yielding behavior:
local filedemo = function()
	local path = uv.cwd() .. "/README.md"
	local stat = uv.fs_stat(path)
	print("stat", path)
	for k, v in pairs(stat) do print(path, k, v) end
	local size = stat.size
	print("opening file", path)
	local f = assert(uv.fs_open(path, "r"))
	print("opened file", f)
	local buf = f:read(stat.size)
	print("read file", buf)
end
filedemo()
coroutine.wrap(filedemo)()