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

const char *algorithmNames[] = {
	"Random",
	"Ordered 2x2 Bayer matrix",
	"Ordered 4x4 Bayer matrix",
	"Ordered 8x8 Bayer matrix",
	"Ordered 16x16 Bayer matrix",
	"Floyd-Steinberg"};

constexpr const int algorithmCount = sizeof(algorithmNames) / sizeof(char*);

const int fontSize = 20;		//Font size for texts
const int padding = 5;			//Padding of GUI panels
const float scaleMin = 0.01f;	//Min scale of the image (1%)

Image baseImage; 				//Base image that was loaded
Image displayedImage; 			//Image that is currently displayed
Texture2D texture; 				//Texture created form the displayed image
const char* title = "None"; 	//Algorithm title
bool imageLoaded = false;   	//Is a image loaded (Should processing be enabled)
bool scaleRender = true; 		//Can the scale of the image be changed
float scaleAdd = 0.0f; 			//Additional scale from the scrollwheel
Vector2 moveOffset; 			//Image displaying offset
float executionTime; 			//How long did the last algorithm took to finish
bool showGui = true; 			//Should the GUI be visible (stats and options)
bool namingFile = false; 		//Are we naming the output file
std::string fileNameBuff; 		//output file name
std::string filePath; 			//Input file directory

//------------
// LOGIC
//------------

//Load a file form the provided path and sets it as the baseImage
void LoadBaseFile(char* filePath)
{
	//Only .png images are supported
	if(IsFileExtension(filePath, ".png"))
	{
		//Unload previous image data if it was loaded
		if (imageLoaded)
		{
			UnloadImage(baseImage);
			UnloadImage(displayedImage);
			UnloadTexture(texture);
			::filePath.clear();
			imageLoaded = false;
		}

		//Load the new image
		baseImage = LoadImage(filePath);
		//Don't do anything if the image wasn't loaded
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
				//Get the data from the file
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
		//Dither every image from the passed files
		for (int i = 0; i < fileCount; i++)
		{
			if(IsFileExtension(paths[i], ".png"))
			{
				image = LoadImage(paths[i]);
				if(image.data != nullptr)
				{
					algorithms[alg](image, colored);
					//Export the image to the same location with the _processed suffix
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
	//Reset the file name to the default one
	fileNameBuff = "out";
}

//Exports the processed image
void ExportImage()
{
	//Construct the path using the loaded file directory and the provided file name
	ExportImage(displayedImage, TextFormat("%s/%s.png", filePath.c_str(), fileNameBuff.c_str()));
}

//Executes the algorithm form the algorithms array
void ExecuteAlgorithm(int algNumber)
{
	if(algNumber < 0 || algNumber >= algorithmCount)
		return;

	//Unload old data
	UnloadTexture(texture);
	UnloadImage(displayedImage);
	displayedImage = ImageCopy(baseImage);

	//Measure the time and execute the algorithm
	float startTime = GetTime();
	bool colored = IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT);
	algorithms[algNumber](displayedImage, colored);
	executionTime = GetTime() - startTime;

	//Set the display name algorithm name
	title = algorithmNames[algNumber];

	//Load the resoult to the display texture
	texture = LoadTextureFromImage(displayedImage);
}

//Handles the file dropping on the application screen
void HandleFileDropping()
{
	if (IsFileDropped())
	{
		int fileCount;
		char** files = GetDroppedFiles(&fileCount);
		//If one file is dropped then load it, if multiple files were dropped start batch processing
		if(fileCount == 1)
			LoadBaseFile(files[0]);
		else
			DoBatchProcessing(fileCount, files);
		
		//Clear the dropped files, so they won't trigger this function again
		ClearDroppedFiles();
	}
}

//Handles the logic of naming a file
void HandleFileNaming()
{
	//File is exported when the enter key is pressed and the file name is not empty
	if (IsKeyPressed(KEY_ENTER) && fileNameBuff.size() > 0)
	{
		ExportImage();
		namingFile = false;
		ResetFileName();
	}
	else
	{
		//Remove last character if backspace was pressed
		if (IsKeyPressed(KEY_BACKSPACE))
		{
			if (fileNameBuff.size() > 0)
				fileNameBuff.resize(fileNameBuff.size() - 1);
		}
		else
		{
			//Clear the buffer of the pressed keys since the last event pooling
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
		//Image transformations (scaling and offsetting)
		scaleAdd += GetMouseWheelMove() * 0.1f;
		if (IsMouseButtonDown(0))
		{
			Vector2 delta = GetMouseDelta();
			moveOffset.x += delta.x;
			moveOffset.y += delta.y;
		}

		//Toggle gui visibility
		if (IsKeyPressed(KEY_TAB))
			showGui = !showGui;

		//Reset transformations
		if (IsKeyPressed(KEY_R))
		{
			//If shift is also pressed then lock the scaling
			if (IsKeyDown(KEY_LEFT_SHIFT))
				scaleRender = !scaleRender;
			scaleAdd = 0.0f;
			moveOffset.x = 0.0f;
			moveOffset.y = 0.0f;
		}

		//Start file exporting process
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
			//Check for the pressed key by calculating it's offset from the '1' key
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
	//Initialized panel width and compute panel height
	int panelWidth = INT_MIN;
	int panelHeight = padding + (fontSize + padding) * size;

	//Get the panel's width (size of the longest text)
	for (int i = 0; i < size; i++)
	{
		int currentWidth = MeasureText(texts[i], fontSize);
		if (currentWidth > panelWidth)
			panelWidth = currentWidth;
	}

	//Add padding to the panel width
	panelWidth += padding * 2;

	//If the panel should be centered then addjust it's position
	if (centered)
	{
		xPos = (GetScreenWidth() - panelWidth) * 0.5f;
		yPos = (GetScreenHeight() - panelHeight) * 0.5f;
	}
	else
	{
		//Clamp the panel's position so it's on the screen
		xPos = xPos + panelWidth > GetScreenWidth() ? GetScreenWidth() - panelWidth : xPos;
		yPos = yPos + panelHeight > GetScreenHeight() ? GetScreenHeight() - panelHeight : yPos;
	}

	//Draw background
	DrawRectangle(xPos, yPos, panelWidth, panelHeight, WHITE);
	int currentY = padding + yPos;
	//Draw texts
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
		//Calculate the image scale, so it takes as much of the window as possible
		float scale;
		if (scaleRender)
		{
			scale = fminf(screenWidth / texture.width, screenHeight / texture.height) + scaleAdd;
			//Clamp the scale
			scale = fmax(scale, scaleMin);
		}
		//Or lock the scale to one if required
		else
			scale = 1.0f;

		//Calculate the display size
		float drawWidth = texture.width * scale;
		float drawHeight = texture.height * scale;

		//Draw the image with the proper scale and offset
		DrawTextureEx(texture, (Vector2){(screenWidth - drawWidth) * 0.5f + moveOffset.x, (screenHeight - drawHeight) * 0.5f + moveOffset.y}, 0.0f, scale, WHITE);
		if (showGui)
			DrawGUI(scale);
	}
	else
	{
		//Display the instruction text
		const char *text = "Drop image (.png) here!";
		int xSize = MeasureText(text, fontSize);
		DrawText(text, ((float)GetScreenWidth() - xSize) * 0.5f, ((float)GetScreenHeight() - fontSize) * 0.5f, fontSize, BLACK);
	}
	EndDrawing();
}

//Entry point
int main(int argc, char **argv)
{
	//Bach processing can start only when the program has more than 2 arguments (.exe file path, one batch file, one image)
	if(argc > 2)
	{
		DoBatchProcessing(argc - 1, argv + 1);
		return 0; //If batch processing is detected, do it and don't start the window
	}

	//Disable logging when the application is not in debug mode
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


