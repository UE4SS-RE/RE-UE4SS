local jsb = require "jsbProfi"

-- jsb.simpleBenchMulti("test123", 50, print, "testing")

-- jsb.simpleBench("looptest", ForEachUObject, function(Object, ChunkIndex, ObjectIndex)
--     print(string.format("Chunk: %X | Object: %X | Name: %s\n", ChunkIndex, ObjectIndex, Object:GetFullName()))
--   end)

-- jsb.profile(15, "testing vm_hook")

-- ForEachUObject( function(Object, ChunkIndex, ObjectIndex)
--   print(string.format("Chunk: %X | Object: %X | Name: %s\n", ChunkIndex, ObjectIndex, Object:GetFullName()))
-- end)

-- for i = 1, 50 do print("TEST"..i) end