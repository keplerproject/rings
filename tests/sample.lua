require"rings"

S = rings.new ()

data = { 12, 13, 14, }
print (S:dostring ([[
aux = {}
for i, v in ipairs (arg) do
	table.insert (aux, 1, v)
end
return unpack (aux)]], unpack (data)))

--[[
local data = { 12, 13, 14, 15, }
local cmd = string.format ([[
assert (type(arg) == "table")
assert (arg[1] == %d)
assert (arg[2] == %d)
assert (arg[3] == %d)
assert (arg[4] == %d)
return unpack (arg)]], unpack (data))
local _data = { S:dostring(cmd, data[1], data[2], data[3], data[4]) }
assert (true == table.remove (_data, 1), "Unexpected error: "..tostring(_data[2]))
for i, v in ipairs (data) do
	assert (v == _data[i])
end
--]]

S:close ()

print("OK!")
