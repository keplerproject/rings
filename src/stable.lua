----------------------------------------------------------------------------
-- Stable: State persistent table.
--
-- Copyright (c) 2004-2005 Kepler Project
-- $Id: stable.lua,v 1.1 2006/01/13 17:58:30 tomas Exp $
----------------------------------------------------------------------------

local dostring = assert (dostring, "There is no `dostring'.  Probably not in a slave state")
-- creating persistent table at master state.
assert (dostring[[persistent_table = persistent_table or {}]])

module"stable"

----------------------------------------------------------------------------
_COPYRIGHT = "Copyright (C) 2003-2006 Kepler Project"
_DESCRIPTION = "State persistent table"
_VERSION = "Stable 1.0"

----------------------------------------------------------------------------
function get (i)
	local ok, value = dostring ("return persistent_table[arg[1]]", i)
	return value
end

----------------------------------------------------------------------------
function set (i, v)
	dostring ("persistent_table[arg[1]] = arg[2]", i, v)
end
