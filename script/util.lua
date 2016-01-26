local util = {}

function util.dump(t, indent)
	if not indent then indent = 0 end
	for k, v in pairs(t) do
		fmt = string.rep("  ", indent) .. k .. ": "
		if type(v) == "table" then
			print(fmt)
			util.dump(v, indent + 1)
		elseif type(v) == "boolean" then
			print(fmt .. tostring(v))
		else
			print(fmt .. v)
		end
	end
end

return util