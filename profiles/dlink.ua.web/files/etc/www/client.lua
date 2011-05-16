#!/usr/local/bin/lua-5.1

dofile("lib/sock.lua");
--socket = require( "libhttpd" );

local sp, name, name1;
sp, name1 = spadd("localhost", 80);
_, name = spadd("192.168.0.90", 8080, sp);

--print("new")
--s = sock:new(socket);
--print("!!!!!!!! open -----------")
--sp[name]:open("localhost", 80);
print("write -----------")
sp[name]:write("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");
print("read -----------")
data, len = sp[name]:read();
print(data);
print("!!!!!!!! refresh -----------")
sp[name]:refresh();
print("write -----------")
sp[name]:write("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");
print("read -----------")
data, len = sp[name]:read();
print(data);
print("!!!!!!!! refresh -----------")
sp[name]:refresh();
--print("write -----------")
--sp[name]:write("GET / HTTP/1.0\r\n");
--print("drain -----------")
--sp[name]:drain();
print("write -----------")
sp[name1]:write("GET / HTTP/1.0\r\nConnection: close\r\n\r\n");
print("read -----------")
data, len = sp[name1]:read();
print(data);

print("close -----------")
sp[name]:close()
sp[name] = nil;
sp[name1]:close()
sp[name1] = nil;


