uv = require "uv"

local function demo(str)
	local f = loadstring("return "..str)
	print(str, f())
end

local start = uv.now()

local function oneshotcb(timer)
	print("oneshotcb callback", uv.now(), timer)
	if uv.now() < start + 1000 then
		--timer:start(250)
	else
		timer:close()
		print("timer closed:", timer)
	end
end

local t = uv.timer()
-- start immediately:
t:start(oneshotcb, 0)

demo "uv.run_once()"

local function repeatcb(timer)
	print("repeatcb callback", uv.now(), timer)
	if uv.now() > start + 1000 then
		timer:close()
		print("timer closed:", timer)
	end
end
-- start immediately & recur every 250 ms:
t:start(repeatcb, 0, 250)

print(string.rep("-", 64))
demo "uv.last_error()"
demo "uv.update_time()"
demo "uv.now()"
demo "uv.run_once()"
demo "uv.run_once()"
-- this will block as long as there is some handle alive (e.g. a timer)
demo "uv.run()"
print(string.rep("-", 64))