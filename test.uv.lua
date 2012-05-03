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
print("network")
print(string.rep("-", 64))

local addresses = { uv.interface_addresses() }
for i, a in pairs(addresses) do 
	print(string.format("interface %d: family: %s address: %s interface: %s internal:", i, a.family, a.address, a.interface), a.internal)
end

print(string.rep("-", 64))
print("timer")
print(string.rep("-", 64))


--]=]

print("ok")