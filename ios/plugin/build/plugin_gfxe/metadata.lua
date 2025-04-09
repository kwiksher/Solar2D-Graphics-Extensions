local Library = require "CoronaLibrary"

-- Create library
local lib = Library:new{ name='plugin.gfxe', publisherId='com.kwiksher' }

-- Default functions
lib.init = function() end

return lib
