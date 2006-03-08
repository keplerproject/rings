-- $Id: sample_state.lua,v 1.1 2006/03/08 17:33:49 tomas Exp $

require"rings"

local init_cmd = [[
LUA_PATH = "]]..package.path..[["
require"compat-5.1"
require"stable"]]

local count_cmd = [[
count = stable.get"shared_counter" or 0
stable.set ("shared_counter", count + 1)
return count
]]

S = rings.new () -- new state
assert(S:dostring (init_cmd))
print (S:dostring (count_cmd)) -- true, 0
print (S:dostring (count_cmd)) -- true, 1
S:close ()

S = rings.new () -- another new state
assert (S:dostring (init_cmd))
print (S:dostring (count_cmd)) -- true, 2
S:close ()

print("OK!")
