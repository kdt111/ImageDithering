//Standard headers
#include <limits.h>
#include <string>
#include <fstream>
#include <math.h>

//Raylib headers
#include <raylib.h>
#include <extras/raygui.h>

//tinyfiledialogs header
#include <tinyfiledialogs.h>

#include "dithering.h"

void (*algorithms[])(Image&, bool) = {
	Dithering::Random,
	Dithering::Ordered2x2,
	Dithering::Ordered4x4,
	Dithering::Ordered8x8,
	Dithering::Ordered16x16,
	Dithering::FloydSteinberg};

const char *algorithmNames[] = {
	"Random",
	"Ordered 2x2",
	"Ordered 4x4",
	"Ordered 8x8",
	"Ordered 16x16",
	"Floyd-Steinberg"};

const char *toggleButtonText = "None\nRandom\nOrdered 2x2\nOrdered 4x4\nOrdered 8x8\nOrdered 16x16\nFloyd-Steinberg";

constexpr const int algorithmCount = sizeof(algorithmNames) / sizeof(char *);

const int fontSize = 20;		//Font size for texts
const int padding = 5;			//Padding of GUI panels
const float scaleMin = 0.01f;	//Min scale of the image (1%)

Image baseImage; 				//Base image that was loaded
Image displayedImage; 			//Image that is currently displayed
Texture2D texture; 				//Texture created form the displayed image
bool processColored = false;	//Should the image be processed in color
int selectedAlgorithm = 0;		//Selected display algorithm
bool executeAlgorithm = false;	//Should the algorithm be executed on the next frame
bool imageLoaded = false;   	//Is a image loaded (Should processing be enabled)
bool scaleRender = true; 		//Can the scale of the image be changed
float scaleAdd = 0.0f; 			//Additional scale from the scrollwheel
Vector2 moveOffset; 			//Image displaying offset
float executionTime; 			//How long did the last algorithm took to finish
std::string filePath; 			//Input file directory
std::string applicationPath;	//Path to base folder of the application
char saveFileName[100];			//Save file name buffer
bool inSaveDialog = false;		//Is the save dialog active

//File filters
static const char *filterPaterns[] = {"*.png"};
static constexpr int filterPaternSize = sizeof(filterPaterns) / sizeof(char *);

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
			::filePath = filePath;
			imageLoaded = true;
		}
		else
			tinyfd_messageBox("Loading error!", "File cant be loaded as an image", "ok", "error", 1);
	}
	else
		tinyfd_messageBox("Loading error!", "Only .png files are supported", "ok", "error", 1);
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
	algorithms[algNumber](displayedImage, processColored);
	executionTime = GetTime() - startTime;

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

//Main application update loop
void UpdateLoop()
{
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

		//Execute selected algorithm
		if (executeAlgorithm)
		{
			//If 0 is seleted then reload the base image
			if (selectedAlgorithm == 0)
			{
				UnloadTexture(texture);
				UnloadImage(displayedImage);
				displayedImage = ImageCopy(baseImage);
				texture = LoadTextureFromImage(displayedImage);
				executionTime = 0.0f;
			}
			else
				ExecuteAlgorithm(selectedAlgorithm - 1);
			executeAlgorithm = false;
		}
	}
}

//Native dialogs for exporting the algorithm resoult
void SaveDialog()
{
	std::string startPath = GetDirectoryPath(filePath.c_str());
	startPath += "/out.png";

	char* savePath = tinyfd_saveFileDialog("Export the file...", startPath.c_str(), filterPaternSize, filterPaterns, nullptr);
	if(savePath != nullptr)
	{
		if(ExportImage(displayedImage, savePath))
			tinyfd_messageBox("Export status", "File exported", "ok", "info", 1);
		else
			tinyfd_messageBox("Export status", "File export error", "ok", "error", 1);
	}		
}

//Native dialog for opening a file
void LoadDialog()
{
	char *openPath = tinyfd_openFileDialog("Open image...", filePath.empty() ? applicationPath.c_str() : filePath.c_str(), filterPaternSize, filterPaterns, nullptr, 0);
	if (openPath != nullptr)
		LoadBaseFile(openPath);
}

//------------
// GUI Drawing
//------------

//Retruns the render size of the longest text in the array
int GeMaxTextSize(const char** texts, int size)
{
	int max = INT_MIN;
	for(int i = 0; i < size; i++)
	{
		int current = MeasureText(texts[i], fontSize);
		if(current > max)
			max = current;
	}
	return max;
}

//Draws the GUI
void DrawGUI(float scale)
{
	// Start cooridnates of the GUI
	const float startX = 20;
	const float startY = 20;

	// Button dimensions
	const float buttonHeight = fontSize + padding * 2;
	const float buttonWidth = GeMaxTextSize(algorithmNames, algorithmCount) + padding * 8;

	// Current drawind positions
	float currentY = startY;
	float currentX = startX;

	if(GuiButton((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, "Open image"))
		LoadDialog();
	
	currentY += buttonHeight + padding;

	//Draw rest if the image is loaded
	if(imageLoaded)
	{
		// Draw algorithm selection
		int initialSelected = selectedAlgorithm;
		selectedAlgorithm = GuiToggleGroup((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, toggleButtonText, selectedAlgorithm);

		// Move the drawing point
		currentX += buttonWidth + padding;
		currentY = startY;

		// Draw colored selection
		bool initialColored = processColored;
		processColored = GuiToggle((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, "Colored", processColored);
		currentY += buttonHeight + padding;

		// Process the image if paramers were changed
		if (initialSelected != selectedAlgorithm || initialColored != processColored)
			executeAlgorithm = true;

		// Draw scale render controll
		scaleRender = GuiToggle((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, TextFormat("%s zoom", scaleRender ? "Lock" : "Unlock"), scaleRender);
		currentY += buttonHeight + padding;

		// Draw reset controll
		if (GuiButton((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, "Reset scale/offset"))
		{
			scaleAdd = 0.0f;
			moveOffset.x = 0.0f;
			moveOffset.y = 0.0f;
		}
		currentY += buttonHeight + padding;

		// Draw export button
		if (GuiButton((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, "Export") && !inSaveDialog)
			SaveDialog();
		currentX += buttonWidth + padding;

		// Draw the stats
		currentY = startY;
		GuiLabel((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, TextFormat("Execution time: %.2f ms", executionTime * 1000.0f));
		currentY += buttonHeight + padding;
		GuiLabel((Rectangle){currentX, currentY, buttonWidth, buttonHeight}, TextFormat("Image zoom: %.2f%%", scale * 100.0f));
	}
}

//Main application draw loop
void DrawLoop()
{
	BeginDrawing();
	ClearBackground(WHITE);
	float scale = 1.0f;
	if (imageLoaded)
	{
		float screenWidth = GetScreenWidth();
		float screenHeight = GetScreenHeight();
		//Calculate the image scale, so it takes as much of the window as possible
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
	}
	else
	{
		//Display the instruction text
		const char *text = "Drop image (.png) here!";
		int xSize = MeasureText(text, fontSize);
		DrawText(text, ((float)GetScreenWidth() - xSize) * 0.5f, ((float)GetScreenHeight() - fontSize) * 0.5f, fontSize, BLACK);
	}
	DrawGUI(scale);
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

	//All dialogs should be graphical
	tinyfd_assumeGraphicDisplay = 1;
	//Set the base open file location to the application folder
	applicationPath = argv[0];//GetDirectoryPath(argv[0]);

	//Raylib configuration
	SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
	InitWindow(800, 600, "Image Dithering");
	SetWindowMinSize(800, 600);

	GuiSetStyle(DEFAULT, TEXT_SIZE, fontSize); //Set gui text size
	GuiSetStyle(TOGGLE, GROUP_PADDING, padding); //Set toggle group padding

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
}
