-- $Id: sample.lua,v 1.3 2006/07/25 14:09:45 tomas Exp $

require"rings"

S = rings.new ()

data = { 12, 13, 14, }
print (S:dostring ([[
aux = {}
for i, v in ipairs (...) do
	table.insert (aux, 1, v)
end
return unpack (aux)]], unpack (data)))

S:close ()

print("OK!")
