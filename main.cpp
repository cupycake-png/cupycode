#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <string>
#include <vector>
#include <chrono>
#include <filesystem>

#include <stdio.h>
#include <raylib.h>

// TODO: Remove need for tinyfiledialogs.h
#include "tinyfiledialogs.h"
#include "LanguageServerProcess.h"
#include "LSPTypes.h"

using json = nlohmann::json;

// TODO: Make text font size dynamic

void drawTextCentreAligned(const char* text, Vector2 centrePos, Font font, Color colour){
	Vector2 size = MeasureTextEx(font, text, font.baseSize, 2);

	DrawTextEx(font, text, Vector2{((float)GetScreenWidth() - size.x + centrePos.x)/2, ((float)GetScreenHeight() - size.y - centrePos.y)/2}, (float)font.baseSize, 2, colour);
}

std::vector<std::string> stringSplit(std::string str){
	std::vector<std::string> retSplits;
	
	std::string currSplit = "";
	for(char c : str){
		if(c == '\n'){
			retSplits.push_back(currSplit);
			currSplit = "";

		}else{
			currSplit += c;
		}

	}

	retSplits.push_back(currSplit);

	return retSplits;
}

std::string reconstructSplitString(std::vector<std::string> splitString){
	std::string retString = "";

	for(std::string split : splitString){
		retString += split + '\n';
	}

	retString.pop_back();

	return retString;
}

int main(){
	Color bgColour = {30, 23, 41, 255};
	Color cursorColour = {255, 109, 194, 200};

	SetConfigFlags(FLAG_WINDOW_RESIZABLE);
	SetTraceLogLevel(LOG_ERROR);

	InitWindow(1280, 800, "cupycode");

	Font consolaFont = LoadFontEx("CONSOLA.TTF", 35, 0, 250);

	SetTargetFPS(30);

	LanguageServerProcess LS;

	// TEMPORARY: Spawning OLS process (really should use settings to detect if using OLS or external LS)
	if(!LS.start("OLS.exe")){
		// TODO: Turn into actual error popup
		printf("[ERR] Failed to start language server\n");
	}

	InitialiseRequest initialiseMsg(1);

	LS.send(initialiseMsg.toJson());

	std::string currentFileName = "UNNAMED";
	std::vector<std::string> lines = {""};
	int lineIndex = 0;
	int colIndex = 0;
	float scrollX = 0.0f;
	float scrollY = 0.0f;
	int keyPauseMs = 100;
	// TODO: Make better system for this plaz... 
	std::chrono::time_point<std::chrono::system_clock> lastBackspacePress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> backspacePressCheck;
	std::chrono::time_point<std::chrono::system_clock> lastEnterPress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> enterPressCheck;
	std::chrono::time_point<std::chrono::system_clock> lastUpPress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> upPressCheck;
	std::chrono::time_point<std::chrono::system_clock> lastDownPress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> downPressCheck;
	std::chrono::time_point<std::chrono::system_clock> lastLeftPress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> leftPressCheck;
	std::chrono::time_point<std::chrono::system_clock> lastRightPress = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock> rightPressCheck;

	bool showEditor = false;
	while(!WindowShouldClose()){
		std::string response;
		LS.getNextMessage(response);

		if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)){
			showEditor = true;
		
		}else if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)){
			char* openFilePath = tinyfd_openFileDialog("Select File", nullptr, 0, nullptr, nullptr, 0);

			if(openFilePath != nullptr){
				std::filesystem::path path(openFilePath);

				currentFileName = path.filename().string();

				char* contents = LoadFileText(openFilePath);
				std::string fileContents(contents); 
				UnloadFileText(contents);

				lines = stringSplit(fileContents);
		
				if(!showEditor){
					showEditor = true;
				}
			}
		
		}else if(showEditor && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)){
			char* saveFilePath;
			
			if(currentFileName == "UNNAMED"){
				saveFilePath = tinyfd_saveFileDialog("Save File As", nullptr, 0, nullptr, nullptr); 
			}

			if(saveFilePath != nullptr){
				std::filesystem::path path(saveFilePath);

				currentFileName = path.filename().string();

				std::string content = reconstructSplitString(lines);

				if(!SaveFileText(saveFilePath, (char*)content.c_str())){
					printf("Error saving file %s with content %s\n", path, content);
					// TODO: Implement better way to handle errors pls
				}
			}
		}

		BeginDrawing();

		ClearBackground(bgColour);

		if(showEditor){
			if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_B)){
				showEditor = false;
			}

			int key = GetCharPressed();

			while(key > 0){
				if((key >= 32) && (key <= 126)){					
					lines[lineIndex].insert(lines[lineIndex].begin() + colIndex, (char)key);
					colIndex++;
				}

				key = GetCharPressed();
			}

			if(IsKeyPressed(KEY_TAB)){
				lines[lineIndex].insert(lines[lineIndex].begin() + colIndex, 4, ' ');
				colIndex += 4;
			}

			if(IsKeyDown(KEY_BACKSPACE)){
				backspacePressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(backspacePressCheck - lastBackspacePress).count() > keyPauseMs){
					lastBackspacePress = std::chrono::system_clock::now();

					if(colIndex == 0){
						if(lineIndex > 0){
							colIndex = lines[lineIndex - 1].length();
							lines[lineIndex - 1] += lines[lineIndex];
							lines.erase(lines.begin() + lineIndex);
							lineIndex--;
						}

					}else{
						lines[lineIndex].erase(colIndex - 1, 1);
						colIndex--;
					}
				}

			}else if(IsKeyDown(KEY_ENTER)){
				enterPressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(enterPressCheck - lastEnterPress).count() > keyPauseMs){
					lastEnterPress = std::chrono::system_clock::now();

					std::string currentLine = lines[lineIndex];
					std::string newLine = currentLine.substr(colIndex);
					lines[lineIndex] = currentLine.substr(0, colIndex);
					lines.insert(lines.begin() + lineIndex + 1, newLine);
					lineIndex++;
					colIndex = 0;
				}

			}else if(IsKeyDown(KEY_UP)){
				upPressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(upPressCheck - lastUpPress).count() > keyPauseMs){
					lastUpPress = std::chrono::system_clock::now();

					if(lineIndex > 0){
						lineIndex--;

						if(colIndex > lines[lineIndex].length()){
							colIndex = lines[lineIndex].length();
						}
					}
				}

			}else if(IsKeyDown(KEY_DOWN)){
				downPressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(downPressCheck - lastDownPress).count() > keyPauseMs){
					lastDownPress = std::chrono::system_clock::now();

					if(lineIndex < lines.size()-1){
						lineIndex++;

						if(colIndex > lines[lineIndex].length()){
							colIndex = lines[lineIndex].length();
						}
					}
				}

			}else if(IsKeyDown(KEY_LEFT)){
				leftPressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(leftPressCheck - lastLeftPress).count() > keyPauseMs){
					lastLeftPress = std::chrono::system_clock::now();

					if(colIndex <= 0 && lineIndex > 0){
						lineIndex--;
						colIndex = lines[lineIndex].length();
					}else{
						if(colIndex > 0){
							colIndex--;
						}
					}
				}

			}else if(IsKeyDown(KEY_RIGHT)){
				rightPressCheck = std::chrono::system_clock::now();

				if(std::chrono::duration_cast<std::chrono::milliseconds>(rightPressCheck - lastRightPress).count() > keyPauseMs){
					lastRightPress = std::chrono::system_clock::now();

					if(colIndex >= lines[lineIndex].length()){
						if(lineIndex < lines.size() - 1){
							lineIndex++;
							colIndex = 0;
						}
					
					}else{
						colIndex++;
					}
				}
			}

			// Draw cursor

			std::string currentLine = lines[lineIndex];

			std::string textBeforeCursor = currentLine.substr(0, colIndex);
			Vector2 cursorTextSize = MeasureTextEx(consolaFont, textBeforeCursor.c_str(), consolaFont.baseSize, 2);
			float cursorXPos = cursorTextSize.x;
			float cursorYPos = lineIndex * 50;

			Vector2 placeholderSize = MeasureTextEx(consolaFont, " ", consolaFont.baseSize, 2);
			float cursorWidth = placeholderSize.x;
			float cursorHeight = placeholderSize.y;

			if(cursorXPos - scrollX > GetScreenWidth() - 20){
				scrollX = cursorXPos - (GetScreenWidth() - 20);
			
			}else if(cursorXPos - scrollX < 0){
				scrollX = cursorXPos - 20;
				
				if(scrollX < 0){
					scrollX = 0;
				}
			}

			if(cursorYPos - scrollY > GetScreenHeight() - 50){
				scrollY = cursorYPos - (GetScreenHeight() - 50);
			
			}else if(cursorYPos - scrollY < 0){
				scrollY = cursorYPos;

				if(scrollY < 0){
					scrollY = 0;
				}
			}

			DrawRectangle(cursorXPos - scrollX, cursorYPos - scrollY + 50, cursorWidth, cursorHeight, cursorColour);

			// Draw lines of text

			for (int i = 0; i < lines.size(); i++) {
				float yPos = i * 50 - scrollY;
				Vector2 textPos = {-scrollX, yPos + 50};
				DrawTextEx(consolaFont, lines[i].c_str(), textPos, consolaFont.baseSize, 2, WHITE);
			}

			// Draw status bar

			DrawRectangle(0, 0, GetScreenWidth(), 50, PURPLE);

			drawTextCentreAligned(currentFileName.c_str(), {0, (float)GetScreenHeight() - 50}, consolaFont, WHITE);

		}else{
			float menuTextSpacing = GetScreenHeight()/4;

			drawTextCentreAligned("Welcome to CupyCode!", {0, menuTextSpacing}, consolaFont, WHITE);
			drawTextCentreAligned("CTRL + N to create a new file", {0, 0}, consolaFont, WHITE);
			drawTextCentreAligned("CTRL + O to open file", {0, -1*menuTextSpacing}, consolaFont, WHITE);
		}

		EndDrawing();
	}

	UnloadFont(consolaFont);

	CloseWindow();

	return 0;
}