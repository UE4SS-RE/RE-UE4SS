local jsb = {}

-- options
local new_log_eachtime = false
local log_file_name = "jsb.log"

function jsb.IntegratedbasicSerialize(s)
  if s == nil then
    return 'nil'
  else
    if type(s) == "number" or type(s) == "boolean" then
      return tostring(s)
    elseif type(s) == "function" or type(s) == "table" or type(s) == "userdata" then
      return ""
    elseif type(s) == "string" then
      return string.format('%q',s)
    end
  end
end

local function tbl_ibs(tbl)
  local result = jsb.IntegratedbasicSerialize(tbl)
  if string.find(tostring(result), 'table:') then
    result = ''
  end
  return result
end

function jsb.tbl(tbl, name, space, indent, recirc)
  local fstr = string.format
  local internal_call = recirc ~= nil
  recirc = recirc or {}
  space = space or '  '
  indent = indent or ""
  name = name or ""
  if type(tbl) == "table" then
    recirc[tbl] = space
    local result = {}
    if internal_call then
      result[#result+1] = fstr('%s{\n', name)
    else
      result[#result+1] = fstr('%s%s = {\n', indent, name)
    end
    for key,data in pairs(tbl) do
      -- key
      if type(key) == "number" then
        result[#result+1] = fstr('%s%s[%s] = ', indent, space, tostring(key))
      else
        result[#result+1] = fstr('%s%s[%s] = ', indent, space, tbl_ibs(key))
      end
      -- data
      if type(data) == "number" or type(data) == "boolean" then
        result[#result+1] = fstr('%s,\n', tostring(data))
      elseif type(data) == "string" then
        result[#result+1] = fstr('%s,\n', tbl_ibs(data))
      elseif type(data) == "table" then
        recirc[data] = fstr('\n%s[%s]', space, tbl_ibs(key))
        result[#result+1] = fstr('%s\n', jsb.tbl(data, '', fstr('%s', space), fstr('%s  ', indent), recirc))
      elseif type(data) == "boolean" then
        result[#result+1] = fstr("%s\n",tostring(data))
      else
        result[#result+1] = "nil,\n"
      end
    end
    if not internal_call then result[#result+1] = fstr("%s}", indent) else result[#result+1] = fstr("%s},", indent) end
    return table.concat(result)
  end
end

function jsb.write(data, file, method)
    local File = io.open(file, method or "a")
    if not File then return end
    File:write(data):close()
end

table.keys = function(t)
  local keys = {}
  local fmt = string.format
  for k in pairs (t or {}) do
    local type_obj = type(t[k])
    keys[#keys+1] = fmt("%s : (%s)", k, type_obj)
    if type_obj == "string" or type_obj == "number" or type_obj == "boolean" then keys[#keys+1] = " " .. tostring(t[k]) end
    keys[#keys+1] = "\n"
  end
  return table.concat(keys)
end

-- simple bench
  -- 2 simple func that can stand inline of a call (former a single call and the latter can be made to run the call n times)
  -- will output the call performance to log file as time to execute in seconds
  -- see example UEHelpers line 72 
  function jsb.simpleBench(name, func, ...)
    if not name or type(name) ~= "string" then return print("Error in bench args!") end
    local data_return
    local start = os.clock()
    data_return = {func(...)}
    local fin = os.clock() - start
    jsb.write(string.format("%s: Func %s took: %g\n", os.date(), name or "unk", fin), log_file_name, "a")
    if not data_return[1] then return end
    return table.unpack(data_return)
  end

  function jsb.simpleBenchMulti(name, repeats, func, ...)
    if (not name or type(name) ~= "string") or (not repeats or type(repeats) ~= "number") then return print("Error in bench args!") end
    local data_return
    local start = os.clock()
    for i = 1, repeats or 1 do data_return = {func(...)} end
    local fin = os.clock() - start
    jsb.write(string.format("%s: Func %s took: %g and %g average per call\n", os.date(), name or "unk", fin, fin/(repeats or 1)), log_file_name, "a")
    if not data_return[1] then return end
    return table.unpack(data_return)
  end
--

jsb.write(string.format("\n\n%s: New test session\n\n", os.date()), log_file_name, new_log_eachtime and "w" or "a")

-- profiler

local jsb_profi = {}
jsb.profi = {}

jsb_profi.clock = os.clock

-- function labels
local _func_labels = {}
-- function definitions
local _defined_funcs = {}
-- time of last call
local _time_called = {}
-- total execution time
local _time_elapsed = {}
-- number of calls
local _number_of_calls = {}
-- list of internal profiler functions
local _internal_funcs = {}
local running
local non_lua, line_calls, started = {}, 0, 0
local call_tree = {}
local taken = 0
local fstr = string.format
local real_world_adjust = 0.4582 -- adjust results knowing the process itself will inflate execution
local rwa = true -- enable performance report adjustment for overhead

--- This is an internal function.
function jsb_profi.hooker(event, line, info)
  info = info or debug.getinfo(2, 'fnS')
  local f = info.func
  -- ignore the profiler
  if _internal_funcs[f] or info.what ~= "Lua" then
    return
  end
  -- get the function name if available
  if info.name then
    _func_labels[f] = info.name
  end
  -- find the line definition
  if not _defined_funcs[f] then
    _defined_funcs[f] = info.short_src..":"..info.linedefined
    _number_of_calls[f] = 0
    _time_elapsed[f] = 0
  end
  if _time_called[f] then
    local dt = jsb_profi.clock() - _time_called[f]
    _time_elapsed[f] = _time_elapsed[f] + dt
    _time_called[f] = nil
  end
  if event == "tail call" then
    local prev = debug.getinfo(3, 'fnS')
    jsb_profi.hooker("return", line, prev)
    jsb_profi.hooker("call", line, info)
  elseif event == 'call' then
    _time_called[f] = jsb_profi.clock()
  else
    _number_of_calls[f] = _number_of_calls[f] + 1
  end
end

local missed_events = {}
local no_record = {}

jsb_profi.evlog = function(fmt, ...) missed_events[#missed_events+1] = fstr(fmt,...) end
jsb_profi.wlog = function(fmt, ...) non_lua[#non_lua+1] = fstr(fmt,...) end
local _log = function(fmt, ...) jsb.write(string.format("%s: %s\n", os.date(), string.format(fmt, ...)), log_file_name) end

function jsb_profi.call_tree(alert_only)
  local output = {}
  local alert = {}
  local prv_time
  output[#output+1] = fstr("Call Tree :: %s\n\n", os.date())
  for call = 1,#call_tree do
    if not prv_time then
      output[#output+1] = fstr("First call: %s :: %s :: %s\n",tostring(call_tree[call][1]),tostring(call_tree[call][3]),tostring(call_tree[call][4]))
      prv_time = call_tree[call][2]
    elseif call_tree[call][2]-prv_time < 0.14 then
      output[#output+1] = fstr("%d th call: %s :: %s :: %s\n",call,tostring(call_tree[call][1]),tostring(call_tree[call][3]),tostring(call_tree[call][4]))
      output[#output+1] = fstr("                                                             %f since last call\n",call_tree[call][2]-prv_time)
      prv_time = call_tree[call][2]
    else
      output[#output+1] = fstr("%d th call: %s :: %s :: %s\n",call,tostring(call_tree[call][1]),tostring(call_tree[call][3]),tostring(call_tree[call][4]))
      output[#output+1] = fstr("                                                  ALERT      %f since last call\n",call_tree[call][2]-prv_time)
      alert[#alert+1] = fstr("%d th call: %s :: %s :: %s\n",call,tostring(call_tree[call][1]),tostring(call_tree[call][3]),tostring(call_tree[call][4]))
      alert[#alert+1] = fstr("                                                  ALERT      %f since last call\n",call_tree[call][2]-prv_time)
      prv_time = call_tree[call][2]
    end
  end
  if not alert_only then jsb.write(table.concat(output), "CallTree.log", "w") end
  jsb.write(table.concat(alert), "CallTreeAlert.log", "w")
end

local lastSeen = {}

function jsb_profi.vm_hook(event, line, info)
  line_calls = line_calls + 1
  info = info or debug.getinfo(2, 'fnSl')
  local f = info.func
  -- ignore the profiler itself or non lua code
  if _internal_funcs[f] then
    return
  elseif info.what == "C" or info.what == "?" then
    jsb_profi.wlog("%s", info.what)
    return
  end
  if info.name and no_record[info.name] then
    return
  elseif info.name then
    call_tree[#call_tree+1] = {info.name, os.clock(), info.short_src, info.linedefined}
  end
  -- get the function name if available
  if not _func_labels[f] and info.name then
    _func_labels[f] = info.name
  end
  -- find the line definition
  if not _defined_funcs[f] then
    _defined_funcs[f] = fstr("%s:%s",info.short_src, info.linedefined)
    _number_of_calls[f] = 0
    _time_elapsed[f] = 0
  end

  if _time_called[f] then
    local dt = jsb_profi.clock() - _time_called[f]
    _time_elapsed[f] = _time_elapsed[f] + dt
    _time_called[f] = nil
  end

  if event == "tail call" then
    local prev = debug.getinfo(3, 'fnS')
    jsb_profi.hooker("return", line, prev)
    jsb_profi.hooker("call", line, info)
    call_tree[#call_tree+1] = {prev.name,os.clock(),prev.short_src,prev.linedefined}
  elseif event == 'call' then
    _time_called[f] = jsb_profi.clock()
    if info.name then
      if not lastSeen[info.name] then
        lastSeen[info.name] = os.clock()
      else
        local t = os.clock() - lastSeen[info.name]
        if t > 10.4 and t < 13 then
          _log("Function match :: %s",tostring(info.name))
        end
        lastSeen[info.name] = nil
      end
    end
  else
    -- profile.evlog("%s",event)
    _number_of_calls[f] = _number_of_calls[f] + 1
  end
end

--- Starts collecting data.
function jsb_profi.start_collect(name)
  if not running then
    non_lua, line_calls = {}, 0
    call_tree = {}
    started = os.clock()
    running = true
    jsb.write(string.format("%s: Profiler started for session: %s\n", os.date(), name or "logging"), log_file_name)
    debug.sethook(jsb_profi.vm_hook, "cr")
  else
    jsb.write(string.format("%s: Error! Profiler already started!\n", os.date()), log_file_name)
  end
end

--- Stops collecting data.
function jsb_profi.stop()
  debug.sethook()
  jsb_profi.call_tree(true)
  taken = jsb_profi.clock() - started
  for f in pairs(_time_called) do
    local dt = jsb_profi.clock() - _time_called[f]
    _time_elapsed[f] = _time_elapsed[f] + dt
    _time_called[f] = nil
  end
  -- merge closures
  local lookup = {}
  for f, d in pairs(_defined_funcs) do
    local id = (_func_labels[f] or '?')..d
    local f2 = lookup[id]
    if f2 then
      _number_of_calls[f2] = _number_of_calls[f2] + (_number_of_calls[f] or 0)
      _time_elapsed[f2] = _time_elapsed[f2] + (_time_elapsed[f] or 0)
      _defined_funcs[f], _func_labels[f] = nil, nil
      _number_of_calls[f], _time_elapsed[f] = nil, nil
    else
      lookup[id] = f
    end
  end
  local missed = {}
  missed[#missed+1] = "jsbProfi stopped. Time took: "
  missed[#missed+1] = taken

  local mnl = {}
  missed[#missed+1] = " non_lua: "
  for i = 1,#non_lua do
    if not mnl[non_lua[i]] then
      mnl[non_lua[i]] = true
      missed[#missed+1] = non_lua[i]
      missed[#missed+1] = " "
    end
  end
  _log(table.concat(missed))
  collectgarbage('collect')
end

--- Resets all collected data.
function jsb_profi.reset()
  local data = {_time_elapsed, _number_of_calls}
  for i = 1, 2 do
    for f in pairs(data[i] or {}) do
      data[i][f] = 0
    end
  end
  for f in pairs(_time_called or {}) do
    _time_called[f] = nil
  end
  collectgarbage('collect')
end

--- This is an internal function.
function jsb_profi.comp(a, b)
  local dt = _time_elapsed[b] - _time_elapsed[a]
  if dt == 0 then
    return _number_of_calls[b] < _number_of_calls[a]
  end
  return dt < 0
end

--- Iterates all functions that have been called since the profile was started.
function jsb_profi.query(limit)
  local t = {}
  for f, n in pairs(_number_of_calls) do
    if n > 0 then
      t[#t + 1] = f
    end
  end
  table.sort(t, jsb_profi.comp)
  if limit then
    while #t > limit do
      table.remove(t)
    end
  end
  for i, f in ipairs(t) do
    local dt = 0
    if _time_called[f] then
      dt = jsb_profi.clock() - _time_called[f]
    end
    t[i] = { i, _func_labels[f] or '?', _number_of_calls[f], string.format("%0.4f (%0.4f)",((_time_elapsed[f] + dt)/_number_of_calls[f])*(rwa and real_world_adjust or 1), (_time_elapsed[f] + dt)*(rwa and real_world_adjust or 1)), _defined_funcs[f] }
  end
  return t
end

local cols = { 3, 29, 11, 24, 32 }

--- Generates a text report.
-- @tparam[opt] number limit Maximum number of rows
function jsb_profi.report()
  local line_out = {}
  local report = jsb_profi.query(50)
  for i, row in ipairs(report) do
    for j = 1, 5 do
      local s = row[j]
      local l2 = cols[j]
      s = tostring(s)
      local l1 = s:len()
      if l1 < l2 then
        s = s..(' '):rep(l2-l1)
      elseif l1 > l2 then
        s = s:sub(l1 - l2 + 1, l1)
      end
      row[j] = s
    end
    line_out[i] = table.concat(row, ' | ')
  end

  local row = " +-----+-------------------------------+-------------+--------------------------+----------------------------------+ \n"
  local col = " | #   | FunctionName                  | nCalls      | TimePerCall (total)      | CodeLine                         | \n"
  local header = row .. col .. row
  if #line_out > 0 then
    header = header .. ' | ' .. table.concat(line_out, ' | \n | ') .. ' | \n'
  end
  return '\n' .. header .. row
end

function jsb_profi.stop_collect()
  if not running then return end
  _log("Profiler stopped!")
  running = false
  jsb_profi.stop()
  local rep = jsb_profi.report()
  _log(rep)
  jsb_profi.reset()
end

-- user call to start lua vm profiler
-- n is seconds to hook all lua execution
-- name the session with a string for ease of differentiation in reporting
-- can only be active in one state at a time, no async or multi sessions (can be made into a class if this is required)
-- will automatically report the trace results to the log file
function jsb.profile(n, name)
  jsb_profi.reset()
  jsb_profi.start_collect(name)
  -- only auto stop if implicit
  if n then ExecuteWithDelay(n * 1000, jsb_profi.stop_collect) end
end

-- can manually call to stop and report
jsb.stop_trace = jsb_profi.start_collect

-- store all internal profiler functions
  for _, v in pairs(jsb_profi) do
    if type(v) == "function" then
      _internal_funcs[v] = true
    end
  end
--

return jsb