----------------------------------------------------------------------------
-- Stable: State persistent table.
--
-- Copyright (c) 2004-2006 Kepler Project
-- $Id: stable.lua,v 1.5 2006/07/25 14:09:45 tomas Exp $
----------------------------------------------------------------------------

local remotedostring = assert (remotedostring, "There is no `remotedostring'.  Probably not in a slave state")
-- creating persistent table at master state.
assert (remotedostring[[_state_persistent_table_ = _state_persistent_table_ or {}]])

module"stable"

----------------------------------------------------------------------------
_COPYRIGHT = "Copyright (C) 2004-2006 Kepler Project"
_DESCRIPTION = "State persistent table"
_VERSION = "Stable 1.0"

----------------------------------------------------------------------------
function get (i)
	local ok, value = remotedostring ("return _state_persistent_table_[...]", i)
	return value
end

----------------------------------------------------------------------------
function set (i, v)
	remotedostring ("_state_persistent_table_[select(1,...)] = select(2,...)", i, v)
end
