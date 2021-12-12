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
bool imageLoaded = false;   	//Is a image loaded (Should processing be enabled)

bool scaleRender = true; 		//Can the scale of the image be changed
float scaleAdd = 0.0f; 			//Additional scale from the scrollwheel
Vector2 moveOffset; 			//Image displaying offset

std::string filePath; 			//Input file directory
std::string applicationPath;	//Path to base folder of the application

//File filters
static const char *filterPaterns[] = {"*.png", "*.bmp", "*.tga", "*.jpg"};
static constexpr int filterPaternSize = sizeof(filterPaterns) / sizeof(char *);
static const char *filterDescription = "Image files (*.png, *.bmp, *.tga, *.jpg)";

//------------
// LOGIC
//------------

//Load a file form the provided path and sets it as the baseImage
void LoadBaseFile(char* filePath)
{
	if (imageLoaded)
	{
		UnloadImage(baseImage);
		UnloadImage(displayedImage);
		UnloadTexture(texture);
		::filePath.clear();
		imageLoaded = false;
	}

	// Load the new image
	baseImage = LoadImage(filePath);
	// Don't do anything if the image wasn't loaded
	if (baseImage.data != nullptr)
	{
		//Format the image to the required pixel format
		//ImageFormat(&baseImage, PIXELFORMAT_UNCOMPRESSED_R8G8B8);
		displayedImage = ImageCopy(baseImage);
		texture = LoadTextureFromImage(displayedImage);
		::filePath = filePath;
		imageLoaded = true;
	}
	else
		tinyfd_messageBox("Loading error!", "File cant be loaded as an image", "ok", "error", 1);
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
			if(IsFileExtension(paths[i], ".txt"))
				continue;

			image = LoadImage(paths[i]);
			if (image.data != nullptr)
			{
				algorithms[alg](image, colored);
				// Export the image to the same location with the _processed suffix
				ExportImage(image, TextFormat("%s/%s_processed%s", GetDirectoryPath(paths[i]), GetFileNameWithoutExt(paths[i]), GetFileExtension(paths[i])));
				UnloadImage(image);
			}
		}
	}
}

//Executes the algorithm form the algorithms array
void ExecuteAlgorithm(int algNumber, bool colored)
{
	if(algNumber < 0 || algNumber >= algorithmCount)
		return;

	//Unload old data
	UnloadTexture(texture);
	UnloadImage(displayedImage);
	displayedImage = ImageCopy(baseImage);

	//Execute the algorithm
	algorithms[algNumber](displayedImage, colored);

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
	}
}

//Native dialogs for exporting the algorithm resoult
void SaveDialog()
{
	std::string startPath = GetDirectoryPath(filePath.c_str());
	startPath += "/out";
	startPath += GetFileExtension(filePath.c_str());

	char* savePath = tinyfd_saveFileDialog("Export the file...", startPath.c_str(), filterPaternSize, filterPaterns, filterDescription);
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
	char *openPath = tinyfd_openFileDialog("Open image...", filePath.empty() ? applicationPath.c_str() : filePath.c_str(), filterPaternSize, filterPaterns, filterDescription, 0);
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
void DrawGUI()
{
	//GUI state
	static int selectedAlgorithm;
	static bool processColored;
	static bool showAlgorithmSelection;
	static bool showOptions;

	// Start cooridnates of the GUI
	const float startX = 20;
	const float startY = 20;

	// Button dimensions
	const float buttonHeight = fontSize + padding * 2;
	const float buttonWidth = GeMaxTextSize(algorithmNames, algorithmCount) + padding * 8;

	Rectangle drawRect = {startX, startY, buttonWidth, buttonHeight};

	if(GuiButton(drawRect, "Open image"))
		LoadDialog();
	drawRect.y += buttonHeight + padding;

	//Draw rest if the image is loaded
	if(imageLoaded)
	{
		// Draw export button
		if (GuiButton(drawRect, "Export image"))
			SaveDialog();
		drawRect.x += buttonWidth + padding;
		drawRect.y = startY;

		// Draw algorithm selection
		showAlgorithmSelection = GuiToggle(drawRect, TextFormat("%s algorithms", showAlgorithmSelection ? "Hide" : "Show"), showAlgorithmSelection);
		drawRect.y += buttonHeight + padding;
		int initialSelected = selectedAlgorithm;
		if(showAlgorithmSelection)
			selectedAlgorithm = GuiToggleGroup(drawRect, toggleButtonText, selectedAlgorithm);
		drawRect.x += buttonWidth + padding;
		drawRect.y = startY;

		// Draw colored selection
		showOptions = GuiToggle(drawRect, TextFormat("%s options", showOptions ? "Hide" : "Show"), showOptions);
		drawRect.y += buttonHeight + padding;
		bool initialColored = processColored;
		if(showOptions)
		{
			processColored = GuiToggle(drawRect, "Colored", processColored);
			drawRect.y += buttonHeight + padding;

			// Draw scale render controll
			scaleRender = GuiToggle(drawRect, TextFormat("%s zoom", scaleRender ? "Lock" : "Unlock"), scaleRender);
			drawRect.y += buttonHeight + padding;

			// Draw reset controll
			if (GuiButton(drawRect, "Reset scale/offset"))
			{
				scaleAdd = 0.0f;
				moveOffset.x = 0.0f;
				moveOffset.y = 0.0f;
			}
		}

		// Process the image if paramers were changed
		if (initialSelected != selectedAlgorithm || initialColored != processColored)
		{
			// If 0 is seleted then reload the base image
			if (selectedAlgorithm == 0)
			{
				UnloadTexture(texture);
				UnloadImage(displayedImage);
				displayedImage = ImageCopy(baseImage);
				texture = LoadTextureFromImage(displayedImage);
			}
			else
				ExecuteAlgorithm(selectedAlgorithm - 1, processColored);
		}
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
	}
	else
	{
		//Display the instruction text
		const char *text = "Drop image here!";
		int xSize = MeasureText(text, fontSize);
		DrawText(text, ((float)GetScreenWidth() - xSize) * 0.5f, ((float)GetScreenHeight() - fontSize) * 0.5f, fontSize, BLACK);
	}
	DrawGUI();
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
