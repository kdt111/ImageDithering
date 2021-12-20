WHITE = {r=255, g=255, b=255}
BLACK = {r=0, g=0, b=0}

function Execute()
	dim = GetImageSize()
	for x = 0, dim.w - 1 do
		for y = 0, dim.h - 1 do
			color = GetColor(x, y)
			if color.r > 127 then
				color.r = 255
			else
				color.r = 0
			end
			if color.g > 127 then
				color.g = 255
			else
				color.g = 0
			end
			if color.b > 127 then
				color.b = 255
			else
				color.b = 0
			end
			SetColor(x, y, color)
		end
	end
end