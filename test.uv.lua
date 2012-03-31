print(string.rep("-", 64))
uv = require "uv"

local function demo(str)
	local f = loadstring("return "..str)
	print(str, f())
end

print(string.rep("-", 64))
print("utilities")
print(string.rep("-", 64))

demo "uv.now()"
demo "uv.hrtime()"
demo "uv.uptime()"
demo "uv.loadavg()"
demo "uv.get_process_title()"
demo "uv.get_free_memory()"
demo "uv.get_total_memory()"


local cpus = { uv.cpu_info() }
for i, cpu in ipairs(cpus) do
	print(string.format("cpu %d: model %s speed %d times.user %f times.nice %f times.sys %f times.idle %f times.irq %f", i, cpu.model, cpu.speed, cpu.times.user, cpu.times.nice, cpu.times.sys, cpu.times.idle, cpu.times.irq))
end

print(string.rep("-", 64))
print("filesystem")
print(string.rep("-", 64))

demo "uv.exepath()"
demo "uv.cwd()"
demo [[uv.chdir(uv.cwd() .. "/libuv")]]
demo "uv.cwd()"

-- run file IO as a coroutine for asynchronous yielding behavior:
local filedemo = function()
	local path = uv.cwd() .. "/LICENSE"
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

print(string.rep("-", 64))
print("network")
print(string.rep("-", 64))

local addresses = { uv.interface_addresses() }
for i, a in pairs(addresses) do 
	print(string.format("interface %d: family: %s address: %s interface: %s internal:", i, a.family, a.address, a.interface), a.internal)
end

print(string.rep("-", 64))
print("timer")
print(string.rep("-", 64))

local start = uv.now()
local t = uv.timer(function(timer)
	print("timer callback", uv.now(), timer)
	if uv.now() < start + 1000 then
		timer:start(250)
	else
		timer:close()
		print("timer closed:", timer)
	end
end)
print("t", t)
t:start()
demo "uv.run_once()"

print(string.rep("-", 64))
demo "uv.last_error()"
demo "uv.update_time()"
demo "uv.now()"
demo "uv.run_once()"
demo "uv.run_once()"
-- this will block as long as there is some handle alive (e.g. a timer)
demo "uv.run()"
print(string.rep("-", 64))

--]=]

print("ok")