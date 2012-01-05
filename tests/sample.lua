-- $Id: sample.lua,v 1.4 2008/05/30 18:44:05 carregal Exp $

require"rings"

S = rings.new ()

data = { 12, 13, 14, }
print (S:dostring ([[
aux = {}
local input = ...
for i, v in ipairs(input)  do
	table.insert (aux, 1, v)
end
return unpack (aux)]], data))

data = { {12, 13}, {14, 15} }

print (S:dostring ([[
aux = {}
local input = ...
for i0, v0 in ipairs(input)  do
   for i, v in ipairs(v0) do
      table.insert (aux, 1, v)
   end
end
return unpack (aux)]], data))

S:close ()

print("OK!")
