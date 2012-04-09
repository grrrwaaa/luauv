local uv = require "uv"
local format = string.format


local simpleresponse = [=[
HTTP/1.0 200 OK
Server: Lua
Content-Type: text/html
]=]

local STATUS_CODES = {
  [100] = 'Continue',
  [101] = 'Switching Protocols',
  [102] = 'Processing',                 -- RFC 2518, obsoleted by RFC 4918
  [200] = 'OK',
  [201] = 'Created',
  [202] = 'Accepted',
  [203] = 'Non-Authoritative Information',
  [204] = 'No Content',
  [205] = 'Reset Content',
  [206] = 'Partial Content',
  [207] = 'Multi-Status',               -- RFC 4918
  [300] = 'Multiple Choices',
  [301] = 'Moved Permanently',
  [302] = 'Moved Temporarily',
  [303] = 'See Other',
  [304] = 'Not Modified',
  [305] = 'Use Proxy',
  [307] = 'Temporary Redirect',
  [400] = 'Bad Request',
  [401] = 'Unauthorized',
  [402] = 'Payment Required',
  [403] = 'Forbidden',
  [404] = 'Not Found',
  [405] = 'Method Not Allowed',
  [406] = 'Not Acceptable',
  [407] = 'Proxy Authentication Required',
  [408] = 'Request Time-out',
  [409] = 'Conflict',
  [410] = 'Gone',
  [411] = 'Length Required',
  [412] = 'Precondition Failed',
  [413] = 'Request Entity Too Large',
  [414] = 'Request-URI Too Large',
  [415] = 'Unsupported Media Type',
  [416] = 'Requested Range Not Satisfiable',
  [417] = 'Expectation Failed',
  [418] = 'I\'m a teapot',              -- RFC 2324
  [422] = 'Unprocessable Entity',       -- RFC 4918
  [423] = 'Locked',                     -- RFC 4918
  [424] = 'Failed Dependency',          -- RFC 4918
  [425] = 'Unordered Collection',       -- RFC 4918
  [426] = 'Upgrade Required',           -- RFC 2817
  [500] = 'Internal Server Error',
  [501] = 'Not Implemented',
  [502] = 'Bad Gateway',
  [503] = 'Service Unavailable',
  [504] = 'Gateway Time-out',
  [505] = 'HTTP Version not supported',
  [506] = 'Variant Also Negotiates',    -- RFC 2295
  [507] = 'Insufficient Storage',       -- RFC 4918
  [509] = 'Bandwidth Limit Exceeded',
  [510] = 'Not Extended'                -- RFC 2774
}

local setheader = function(self, name, value) 
	self.headers[name] = value
end
local addheader = function(self, name, value) 
	table.insert(self.headers, { name, value })
end

local sendheaders = function(self, code)
	code = code or 200
	local reason = STATUS_CODES[code]
	assert(reason, "invalid response code") 
	local head = {
		format("HTTP/1.1 %d %s ", code, reason),
	}
	
	if self.body == nil then
		-- RFC 2616, 10.2.5:
		-- The 204 response MUST NOT include a message-body, and thus is always
		-- terminated by the first empty line after the header fields.
		-- RFC 2616, 10.3.5:
		-- The 304 response MUST NOT contain a message-body, and thus is always
		-- terminated by the first empty line after the header fields.
		-- RFC 2616, 10.1 Informational 1xx:
		-- This class of status code indicates a provisional response,
		-- consisting only of the Status-Line and optional headers, and is
		-- terminated by an empty line.
		if code == 204 or code == 304 or (code >= 100 and code < 200) then
			self.body = nil
		else
			self.body = {}
		end
	end
	
	-- auto-generate values:
	self.headers.Server = self.headers.Server or "Lua"
	-- This should be RFC 1123 date format
    -- IE: Tue, 15 Nov 1994 08:12:31 GMT
    self.headers.Date = self.headers.Date or os.date("!%a, %d %b %Y %H:%M:%S GMT")
	
	if self.body then
		self.headers["Content-Type"] = self.headers["Content-Type"] or "text/html"
		if not self.headers["Content-Length"] then
			self.headers["Transfer-Encoding"] = "chunked"
		end
	end
	
	for field, value in pairs(self.headers) do
		head[#head+1] = format("%s: %s", tostring(field), tostring(value))
	end
	head = table.concat(head, "\r\n") .. "\r\n"
	
	print("sending", head)
	
	self.socket:write(head)
end

local continue = function(self)
	self.socket:write("HTTP/1.1 100 Continue\r\n\r\n")
end

local write = function(self, str)
	-- chunked:
	self.socket:write(format("%x\r\n%s\r\n", #str, str))
end

local finish = function(self, str)
	-- chunked:
	self.socket:write("0\r\n\r\n")
	-- done with session:
	self.socket:shutdown()
	self.socket:close()
end


local host = "127.0.0.1"
local port = 8080

-- Request and Response extend stream.

function server()
	local s = uv.tcp()
	s:bind("0.0.0.0", 8080)
	
	s:listen(function()	
		print("listener received request", s)
			
		local client = uv.tcp()
		s:accept(client)
		print("accepted session", s, client)
		
		-- create request/response objects
		local request = { socket = client }
		local response = { 
			socket = client,
			headers = {},
		}
		
		client:read_start(function(self, chunk, size)
			print("received data", size)
			if size < 0 then
				-- EOF: 
				print("session ended", size)
				self:close()
				return
			elseif size == 0 then
				return
			end
			
			-- handle upgrade mode?
			
			-- parse HTTP chunk
			print("chunk", chunk)
			
			
			print("send", simpleresponse)
			self:write(simpleresponse)
	
			--sendheaders(response)
			print("sent headers")
			--write(response, "<html><body>Hello!</body></html>")
			print("sent body")
			--finish(response)
			print("finished")
			
		end)
		
	end)
	
			
	

	
	print("server configured")
end


coroutine.wrap(server)()

uv.run()

print("done")