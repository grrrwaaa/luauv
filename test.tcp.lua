local uv = require "uv"

local host = "127.0.0.1"
local port = 12345

function server()
	local t = uv.tcp()
	t:bind(host, port)		-- blocking
	--local sock = t:getsockname()
	--print("family", sock.family)
	--print("address", sock.address)
	--print("port", sock.port)
	
	-- this may spawn multiple instances
	-- perhaps it would be good for each of them 
	-- to become a new coroutine?
	t:listen(function(t, ...)	-- maps to registry
		print("listener received request", t, ...)
			
		local client = uv.tcp()
		t:accept(client)
		print("accepted session")
		
		-- this may also spawn many instances... 
		client:read_start(function(stream, buffer, nread)
			print("read", buffer, nread)
			if nread > 0 then
				-- echo back!
				stream:write(buffer)	-- yield/resume
			elseif nread < 0 then
				client:close()
			end
		end)
		
		--local sock = client:getsockname()
		--print("family", sock.family)
		--print("address", sock.address)
		--print("port", sock.port)
		
		-- sometime later, finish the session:
		--client:close()
		--print("terminated session")
		
	end)
	-- control gets to here without yielding.
	-- the tcp object stays alive because it is referenced
	-- by the listen callback.
	print("SERVER DONE")
end

function client()
	local c = uv.tcp()	
	
	print"connecting"
	c:connect(host, port)	-- yield/resume
	print("connected")
	
	-- set up read handler before writing
	---[[
	c:read_start(function(stream, buffer, nread)
		print("received back:", buffer, nread)
		--os.exit()
	end)
	--]]
	
	--local peer = c:getpeername()
	--print("family", peer.family)
	--print("address", peer.address)
	--print("port", peer.port)
	
	-- send some:
	for i = 1, 5 do
		print"sending"
		c:write("hello")	-- yield/resume
		print("sent")
	end
	
	c:close()
	
	print("CLIENT DONE")
end

coroutine.wrap(server)()
coroutine.wrap(client)()
coroutine.wrap(client)()

print("RUN")
uv.run()