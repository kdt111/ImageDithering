WHITE = {r=255, g=255, b=255}
BLACK = {r=0, g=0, b=0}

function GetSaturation(color)
	return (color.r + color.g + color.b) / 3
end

function Execute()
	dim = GetImageSize()
	for x = 0, dim.w - 1 do
		for y = 0, dim.h - 1 do
			color = GetColor(x, y)
			if GetSaturation(color) < 128 then
				SetColor(x, y, BLACK)
			else
				SetColor(x, y, WHITE)
			end
		end
	end
end