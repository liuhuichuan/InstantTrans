-- KEYS[1]: 限流 key
-- ARGV[1]: 限制次数
-- ARGV[2]: 窗口期（毫秒）

local key = KEYS[1]
local limit = tonumber(ARGV[1])
local window = tonumber(ARGV[2])

local current = tonumber(redis.call("GET", key) or "0")
if current + 1 > limit then
    return 0
else
    current = redis.call("INCR", key)
    if current == 1 then
        redis.call("PEXPIRE", key, window)
    end
    return 1
end
