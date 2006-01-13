#!/usr/local/bin/lua50

---------------------------------------------------------------------
-- checks for a value and throw an error if it is invalid.
---------------------------------------------------------------------
function assert2 (expected, value, msg)
	if not msg then
		msg = ''
	else
		msg = msg..'\n'
	end
	return assert (value == expected,
		msg.."wrong value (["..tostring(value).."] instead of "..
		tostring(expected)..")")
end

---------------------------------------------------------------------
-- object test.
---------------------------------------------------------------------
local objmethods = { "close", "dostring", }
function test_object (obj)
	-- checking object type.
	assert2 (true, type(obj) == "userdata" or type(obj) == "table", "incorrect object type")
	-- trying to get metatable.
	assert2 ("You're not allowed to get the metatable of a Lua State",
		getmetatable(obj), "error permitting access to object's metatable")
	-- trying to set metatable.
	assert2 (false, pcall (setmetatable, S, {}))
	-- checking existence of object's methods.
	for i = 1, table.getn (objmethods) do
		local method = obj[objmethods[i]]
		assert2 ("function", type(method))
		assert2 (false, pcall (method), "no 'self' parameter accepted")
	end
	return obj
end

---------------------------------------------------------------------
---------------------------------------------------------------------
require"rings"

S = test_object (rings.new())

-- How to handle errors on another Lua State?

assert2 (false, S:dostring"bla()")
assert2 (false, S:dostring"bla(")
assert2 (true, S:dostring"print'Hello World!'")
-- Checking returning value
local ok, _x = S:dostring"return x"
assert2 (true, ok, "Error while returning a value ("..tostring(_x)..")")
assert2 (nil, _x, "Unexpected initialized variable (x = "..tostring(_x)..")")
-- setting a value
assert2 (nil, x, "I need an unitialized varible to do the test!")
S:dostring"x = 1"
assert2 (nil, x, "Changing original Lua State instead of the new one!")
-- obtaining a value from the new state
local ok, _x = S:dostring"return x"
assert2 (true, ok, "Error while returning a value ("..tostring(_x)..")")
assert2 (1, _x, "Unexpected initialized variable (x = "..tostring(_x)..")")

-- executing code in the master state from the new state
global = 2
local ok, _x = S:dostring[[
	local ok, _x = dostring"return global"
	if not ok then
		error(_x)
	else
		return _x
	end
]]
assert2 (true, ok, "Unexpected error: ".._x)
assert2 (global, _x, "Unexpected error: ".._x)

-- new state obtaining data from the master state by using dostring
f1 = function () return "funcao 1" end
f2 = function () return "funcao 2" end
f3 = function () return "funcao 3" end
data = {
	key1 = { f1, f2, f3, },
	key2 = { f3, f1, f2, },
}
local ok, k, i, f = S:dostring[[
	assert (loadfile ("/usr/local/share/lua/5.0/compat-5.1.lua"))()
	require"math"
	require"os"
	math.randomseed(os.date"%s")
	local key = "key"..math.random(2)
	local i = math.random(3)
	local ok, f = dostring("return data."..key.."["..i.."]()")
	return key, i, f
]]
assert2 (true, ok, "Unexpected error: "..k)
assert2 ("string", type(k), string.format ("Wrong #1 return value (expected string, got "..type(k)..")"))
assert2 ("number", type(i), string.format ("Wrong #2 return value (expected number, got "..type(i)..")"))
assert2 ("string", type(f), string.format ("Wrong #3 return value (expected string, got "..type(f)..")"))
assert2 (f, data[k][i](), "Wrong #3 return value")

-- Passing arguments and returning values
local data = { 12, 13, 14, 15, }
local cmd = string.format ([[
assert (type(arg) == "table")
assert (arg[1] == %d)
assert (arg[2] == %d)
assert (arg[3] == %d)
assert (arg[4] == %d)
return unpack (arg)]], unpack (data))
local _data = { S:dostring(cmd, data[1], data[2], data[3], data[4]) }
assert2 (true, table.remove (_data, 1), "Unexpected error: "..tostring(_data[2]))
for i, v in ipairs (data) do
	assert2 (v, _data[i])
end

-- Transfering userdata
local ok, f1, f2 = S:dostring([[ return arg[1], io.stdout ]], io.stdout)
local _f1 = string.gsub (tostring(f1), "%D", "")
local _f2 = string.gsub (tostring(f2), "%D", "")
assert (_f1 ~= _f2, "Same file objects (io.stdout) in different states")
local _stdout = string.gsub (tostring(io.stdout), "%D", "")
assert (_f1 == _stdout, "Lightuserdata has changed its value when transfered to another state")

-- Checking cache
local chunk = [[return tostring(debug.getinfo(1,'f').func)]]
local ok, f1 = S:dostring(chunk)
local ok, f2 = S:dostring(chunk)
local ok, f3 = S:dostring([[return tostring (debug.getinfo(1,'f').func)]])
assert (f1 == f2, "Cache is not working")
assert (f1 ~= f3, "Function `dostring' is producing the same function for different strings")
S:dostring"collectgarbage(); collectgarbage()"
local ok, f4 = S:dostring(chunk)
assert (f4 ~= f1, "Cache is not being collected")
local ok, f5 = S:dostring(chunk)
assert (f4 == f5, "Cache is not working")

-- Closing new state
S:close ()
assert2 (false, pcall (S.dostring, S, "print[[This won't work!]]"))
collectgarbage()
collectgarbage()

print"Ok!"
