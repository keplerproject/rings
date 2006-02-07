----------------------------------------------------------------------------
-- Stable: State persistent table.
--
-- Copyright (c) 2004-2006 Kepler Project
-- $Id: stable.lua,v 1.3 2006/02/07 01:48:56 uid20013 Exp $
----------------------------------------------------------------------------

local remotedostring = assert (remotedostring, "There is no `remotedostring'.  Probably not in a slave state")
-- creating persistent table at master state.
assert (remotedostring[[persistent_table = persistent_table or {}]])

module"stable"

----------------------------------------------------------------------------
_COPYRIGHT = "Copyright (C) 2004-2006 Kepler Project"
_DESCRIPTION = "State persistent table"
_VERSION = "Stable 1.0"

----------------------------------------------------------------------------
function get (i)
	local ok, value = remotedostring ("return persistent_table[arg[1]]", i)
	return value
end

----------------------------------------------------------------------------
function set (i, v)
	remotedostring ("persistent_table[arg[1]] = arg[2]", i, v)
end
