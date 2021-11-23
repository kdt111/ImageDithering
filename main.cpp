//Standard headers
#include <limits.h>
#include <iostream>
#include <fstream>
#include <string>

//Raylib headers
#include <raylib.h>

//Implementation files
#include "algs/dithering.cpp"

void (*algorithms[])(Image&, bool) = {
	Dithering::Random,
	Dithering::Ordered2x2,
	Dithering::Ordered4x4,
	Dithering::Ordered8x8,
	Dithering::Ordered16x16,
	Dithering::FloydSteinberg};

const char* algorithmNames[] = {
	"Random",
	"Ordered 2x2",
	"Ordered 4x4",
	"Ordered 8x8",
	"Ordered 16x16",
	"Floyd-Steinberg"};

constexpr const int algorithmCount = sizeof(algorithmNames) / sizeof(char*);

const int fontSize = 20;	//Font size for texts
const int padding = 5;		//Padding of GUI panels

Image baseImage; 			//Base image that was loaded
Image displayedImage; 		//Image that is currently displayed
Texture2D texture; 			//Texture created form the displayed image
const char* title = "None"; //Algorithm title
bool imageLoaded = false;   //Is a image loaded (Should processing be enabled)
bool scaleRender = true; 	//Can the scale of the image be changed
float scaleAdd = 0.0f; 		//Additional scale from the scrollwheel
Vector2 moveOffset; 		//Image displaying offset
float executionTime; 		//How long did the last algorithm took to finish
bool showGui = true; 		//Should the GUI be visible (stats and options)
bool namingFile = false; 	//Are we naming the output file
std::string fileNameBuff; 	//output file name
std::string filePath; 		//Input file directory

//------------
// LOGIC
//------------

//Load a file form the provided path and sets it as the baseImage
void LoadBaseFile(char* filePath)
{
	if(IsFileExtension(filePath, ".png"))
	{
		if (imageLoaded)
		{
			UnloadImage(baseImage);
			UnloadImage(displayedImage);
			UnloadTexture(texture);
			::filePath.clear();
			imageLoaded = false;
		}

		baseImage = LoadImage(filePath);
		if (baseImage.data != nullptr)
		{
			displayedImage = ImageCopy(baseImage);
			texture = LoadTextureFromImage(displayedImage);
			::filePath = GetDirectoryPath(filePath);
			imageLoaded = true;
		}
	}
}

//Performs batch processing of images
void DoBatchProcessing(int fileCount, char** paths)
{
	int alg = -1, colored = -1;
	//Find a batch configuration file
	for (int i = 0; i < fileCount; i++)
	{
		if(IsFileExtension(paths[i], ".txt"))
		{
			std::fstream file(paths[i]);
			if(file.is_open())
			{
				file >> alg;
				file >> colored;
				file.close();
				break;
			}
		}
	}

	//If configuration was found do batch processing
	if(alg >= 0 && alg < algorithmCount && colored != -1)
	{
		Image image;
		for (int i = 0; i < fileCount; i++)
		{
			if(IsFileExtension(paths[i], ".png"))
			{
				image = LoadImage(paths[i]);
				if(image.data != nullptr)
				{
					algorithms[alg](image, colored);
					ExportImage(image, TextFormat("%s/%s_processed.png", GetDirectoryPath(paths[i]), GetFileNameWithoutExt(paths[i])));
					UnloadImage(image);
				}
			}
		}
	}
}

//Resets the output file name to the default value
void ResetFileName()
{
	fileNameBuff = "out";
}

//Exports the processed image
void ExportImage()
{
	ExportImage(displayedImage, TextFormat("%s/%s.png", filePath.c_str(), fileNameBuff.c_str()));
}

//Executes the algorithm form the algorithms array
void ExecuteAlgorithm(int algNumber)
{
	if(algNumber < 0 || algNumber >= algorithmCount)
		return;

	UnloadTexture(texture);
	UnloadImage(displayedImage);
	displayedImage = ImageCopy(baseImage);

	float startTime = GetTime();
	algorithms[algNumber](displayedImage, IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT));
	executionTime = GetTime() - startTime;

	title = algorithmNames[algNumber];

	texture = LoadTextureFromImage(displayedImage);
}

//Handles the file dropping on the application screen
void HandleFileDropping()
{
	if (IsFileDropped())
	{
		int fileCount;
		char** files = GetDroppedFiles(&fileCount);
		if(fileCount == 1)
			LoadBaseFile(files[0]);
		else
			DoBatchProcessing(fileCount, files);
		ClearDroppedFiles();
	}
}

//Handles the logic of naming a file
void HandleFileNaming()
{
	if (IsKeyPressed(KEY_ENTER))
	{
		ExportImage();
		namingFile = false;
		ResetFileName();
	}
	else
	{
		if (IsKeyPressed(KEY_BACKSPACE))
		{
			if (fileNameBuff.size() > 0)
				fileNameBuff.resize(fileNameBuff.size() - 1);
		}
		else
		{
			while (true)
			{
				char pressed = GetCharPressed();
				if ((pressed >= 'a' && pressed <= 'z') || (pressed >= 'A' && pressed <= 'Z') || (pressed >= '0' && pressed <= '9') || pressed == '_')
					fileNameBuff.append(1, pressed);
				else
					break;
			}
		}
	}
}

//Main application update loop
void UpdateLoop()
{
	if(namingFile)
	{
		HandleFileNaming();	
		return;
	}

	HandleFileDropping();

	if (imageLoaded)
	{
		//Image transformations
		scaleAdd += GetMouseWheelMove() * 0.1f;
		if (IsMouseButtonDown(0))
		{
			Vector2 delta = GetMouseDelta();
			moveOffset.x += delta.x;
			moveOffset.y += delta.y;
		}

		if (IsKeyPressed(KEY_TAB))
			showGui = !showGui;

		if (IsKeyPressed(KEY_R))
		{
			if (IsKeyDown(KEY_LEFT_SHIFT))
				scaleRender = !scaleRender;
			scaleAdd = 0.0f;
			moveOffset.x = 0.0f;
			moveOffset.y = 0.0f;
		}

		if (IsKeyPressed(KEY_E))
		{
			namingFile = true;
			return;
		}

		//Reload normal image
		if (IsKeyPressed(KEY_N))
		{
			UnloadTexture(texture);
			UnloadImage(displayedImage);

			displayedImage = ImageCopy(baseImage);
			texture = LoadTextureFromImage(displayedImage);

			title = "None";
			executionTime = 0.0f;
		}

		//Dither the image using the selected algorithm
		for(int i = 0; i < algorithmCount; i++)
		{
			if(IsKeyPressed(KEY_ONE + i))
			{
				ExecuteAlgorithm(i);
				break;
			}
		}
	}
}

//------------
// GUI Drawing
//------------

//Draws a array of texts on a panel
int DrawTextPanel(int xPos, int yPos, const char* texts[], int size, bool centered = false)
{
	int panelWidth = INT_MIN;
	int panelHeight = padding + (fontSize + padding) * size;

	for (int i = 0; i < size; i++)
	{
		int currentWidth = MeasureText(texts[i], fontSize) + padding * 2;
		if (currentWidth > panelWidth)
			panelWidth = currentWidth;
	}

	if(centered)
	{
		xPos = (GetScreenWidth() - panelWidth) * 0.5f;
		yPos = (GetScreenHeight() - panelHeight) * 0.5f;
	}
	else
	{
		xPos = xPos + panelWidth > GetScreenWidth() ? GetScreenWidth() - panelWidth : xPos;
		yPos = yPos + panelHeight > GetScreenHeight() ? GetScreenHeight() - panelHeight : yPos;
	}

	DrawRectangle(xPos, yPos, panelWidth, panelHeight, WHITE);
	int currentY = padding + yPos;
	for (int i = 0; i < size; i++)
	{
		DrawText(texts[i], xPos + padding, currentY, fontSize, BLACK);
		currentY += fontSize + padding;
	}
	return currentY;
}

//Draws the GUI
void DrawGUI(float scale)
{
	const char *stats[] = {
		"Stats:",
		TextFormat("Used algorithm: %s", title),
		TextFormat("Dithering time: %.2f ms", executionTime * 1000.0f),
		TextFormat("Image scale: %.2f%%", scale * 100.0f)
	};

	const int statsSize = sizeof(stats) / sizeof(char*);

	const char *options[] = {
		"Options:",
		"[N] Show base image",
		"[1-6] Different in grayscale",
		"[SHIFT + 1-6] Dithering in color",
		"[R] Reset transformations (image scale and offset)",
		"[SHIFT + R] Toggle view to 100% image scale",
		"[LMB + Mouse] Move image",
		"[MouseWheel] Zoom image",
		"[E] Export image",
		"[TAB] Toggle GUI",
		"",
		"Created by Jan Malek"
		};

	const int optionsSize = sizeof(options) / sizeof(char*);

	DrawTextPanel(0, 0, stats, statsSize);
	DrawTextPanel(0, GetScreenHeight(), options, optionsSize);

	if(namingFile)
	{
		const char* texts[] = {
			"Output file name (without extensions):",
			fileNameBuff.c_str()
		};

		DrawTextPanel(0, 0, texts, 2, true);
	}
}

//Main application draw loop
void DrawLoop()
{
	BeginDrawing();
	ClearBackground(WHITE);
	if (imageLoaded)
	{
		float screenWidth = GetScreenWidth();
		float screenHeight = GetScreenHeight();
		float scale;
		if (scaleRender)
		{
			scale = fminf(screenWidth / texture.width, screenHeight / texture.height) + scaleAdd;
			if (scale < 0.01f)
				scale = 0.01f;
		}
		else
			scale = 1.0f;

		float drawWidth = texture.width * scale;
		float drawHeight = texture.height * scale;

		DrawTextureEx(texture, (Vector2){(screenWidth - drawWidth) * 0.5f + moveOffset.x, (screenHeight - drawHeight) * 0.5f + moveOffset.y}, 0.0f, scale, WHITE);
		if (showGui)
			DrawGUI(scale);
	}
	else
	{
		const char *text = "Drop image (.png) here!";
		int xSize = MeasureText(text, fontSize);
		DrawText(text, ((float)GetScreenWidth() - xSize) * 0.5f, ((float)GetScreenHeight() - fontSize) * 0.5f, fontSize, BLACK);
	}
	EndDrawing();
}

//Entry point
int main(int argc, char **argv)
{
	if(argc > 2)
	{
		DoBatchProcessing(argc - 1, argv + 1);
		return 0; //If batch processing is detected, do it and don't start the window
	}

	#ifndef DEBUG
	SetTraceLogLevel(LOG_NONE);
	#endif

	//Raylib configuration
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(800, 600, "Image Dithering");
	SetWindowMinSize(800, 600);

	ResetFileName();

	//Load the base image if passed as an argument
	if(argc == 2)
		LoadBaseFile(argv[1]);

	//Main loop
	while (true)
	{
		UpdateLoop();
		DrawLoop();
		if(WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) break;
	}
	
	//Clean up if image was loaded
	if(imageLoaded)
	{
		UnloadImage(baseImage);
		UnloadImage(displayedImage);
		UnloadTexture(texture);
	}
	CloseWindow();
	return 0;
}


