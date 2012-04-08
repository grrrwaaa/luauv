local uv = require "uv"



local stdout = uv.tty(1)

print(stdout)
--stdout:write("bybye\n")

local stdin = uv.tty(0)
print(stdin)
--print(stdin:get_win_size())

stdin:read_start(function(tty, buffer, nread)
	print("read", buffer, nread)
	stdout:write(buffer)
	print("pregc")
	collectgarbage()
	collectgarbage()
	collectgarbage()
	print("postgc")
	
end)

print("run")
uv.run()

--[[

create a server object, give it a connection callback
tell the server to listen on a host/port with a cb1
	makes a Tcp
		Tcp:bind(host, port)
		Tcp responds to "listening" <- cb1
						"listen" <- 
							create client Tcp
							self:accept(client)
							client:readStart()
							client <- "connection"
						

Server responds to:
	



net.createServer = function(connectionCallback)
  local s = Server:new(connectionCallback)
  return s
end

function http.createServer(host, port, onConnection)
  local server
  server = net.createServer(function(client)
    if err then
      return server:emit("error", err)
    end

    -- Accept the client and build request and response objects
    local request = Request:new(client)
    local response = Response:new(client)

    -- Convert tcp stream to HTTP stream
    local current_field
    local parser
    local headers
    parser = HttpParser.new("request", {
      onMessageBegin = function ()
        headers = {}
        request.headers = headers
      end,
      onUrl = function (url)
        request.url = url
      end,
      onHeaderField = function (field)
        current_field = field
      end,
      onHeaderValue = function (value)
        headers[current_field:lower()] = value
      end,
      onHeadersComplete = function (info)

        request.method = info.method
        request.upgrade = info.upgrade

        request.version_major = info.version_major
        request.version_minor = info.version_minor

        -- Give upgrade requests access to the raw client if they want it
        if info.upgrade then
          request.client = client
        end

        -- Handle 100-continue requests
        if request.headers.expect and info.version_major == 1 and info.version_minor == 1 and request.headers.expect:lower() == "100-continue" then
          if server.handlers and server.handlers.check_continue then
            server:emit("check_continue", request, response)
          else
            response:writeContinue()
            onConnection(request, response)
          end
        else
          onConnection(request, response)
        end

      end,
      onBody = function (chunk)
        request:emit('data', chunk, #chunk)
      end,
      onMessageComplete = function ()
        request:emit('end')
      end
    })


    client:on("data", function (chunk)

      -- Ignore empty chunks
      if #chunk == 0 then return end

      -- Once we're in "upgrade" mode, the protocol is no longer HTTP and we
      -- shouldn't send data to the HTTP parser
      if request.upgrade then
        request:emit("data", chunk)
        return
      end

      -- Parse the chunk of HTTP, this will syncronously emit several of the
      -- above events and return how many bytes were parsed.
      local nparsed = parser:execute(chunk, 0, #chunk)

      -- If it wasn't all parsed then there was an error parsing
      if nparsed < #chunk then
        -- If the error was caused by non-http protocol like in websockets
        -- then that's ok, just emit the rest directly to the request object
        if request.upgrade then
          chunk = chunk:sub(nparsed + 1)
          request:emit("data", chunk)
        else
          request:emit("error", "parse error")
        end
      end

    end)

    client:on("end", function ()
      parser:finish()
    end)

    client:on("error", function (err)
      request:emit("error", err)
      -- N.B. must close(), or https://github.com/joyent/libuv/blob/master/src/unix/stream.c#L586
      -- kills the appication
      client:close()
      parser:finish()
    end)

  end)

  server:listen(port, host)

  return server
end


http.createServer(function (req, res)
  local body = "Hello world\n"
  res:writeHead(200, {
    ["Content-Type"] = "text/plain",
    ["Content-Length"] = #body
  })
  res:finish(body)
end):listen(8080)

print("Server listening at http://localhost:8080/")
--]]