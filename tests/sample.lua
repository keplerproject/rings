-- $Id: sample.lua,v 1.2 2006/03/08 17:33:49 tomas Exp $

require"rings"

S = rings.new ()

data = { 12, 13, 14, }
print (S:dostring ([[
aux = {}
for i, v in ipairs (arg) do
	table.insert (aux, 1, v)
end
return unpack (aux)]], unpack (data)))

S:close ()

print("OK!")
