#include <stdio.h>
#include <string>
#include <vector>
#include <chrono>
#include <raylib.h>

#include "tinyfiledialogs.h"

// #include "resource_dir.h"

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

	// SearchAndSetResourceDir("resources");

	Font consolaFont = LoadFontEx("CONSOLA.TTF", 35, 0, 250);

	SetTargetFPS(30);

	std::string currentFilePath = "UNNAMED";
	std::vector<std::string> lines = {""};
	int lineIndex = 0;
	int colIndex = 0;
	int keyPauseMs = 100;
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
		if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_N)){
			showEditor = true;
		
		}else if(IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_O)){
			char* path = tinyfd_openFileDialog("Select File", nullptr, 0, nullptr, nullptr, 0);

			if(path != nullptr){
				char* contents = LoadFileText(path);
				std::string fileContents(contents); 
				UnloadFileText(contents);

				currentFilePath = path;

				lines = stringSplit(fileContents);
		
				if(!showEditor){
					showEditor = true;
				}
			}
		
		}else if(showEditor && IsKeyDown(KEY_LEFT_CONTROL) && IsKeyPressed(KEY_S)){
			char* path;
			
			if(currentFilePath == "UNNAMED"){
				path = tinyfd_saveFileDialog("Save File As", nullptr, 0, nullptr, nullptr);
			}

			if(path != nullptr){
				std::string content = reconstructSplitString(lines);

				if(!SaveFileText(currentFilePath.c_str(), (char*)content.c_str())){
					printf("Error saving file %s with content %s\n", currentFilePath, content);
					// TODO: Implement better way to handle errors
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

			// Draw text

			for(int i=0; i<lines.size(); i++){
				DrawTextEx(consolaFont, lines[i].c_str(), {0, (float)50*i}, consolaFont.baseSize, 2, WHITE);
			}

			// Draw cursor

			std::string currentLine = lines[lineIndex];
			
			Vector2 charSize = MeasureTextEx(consolaFont, currentLine.c_str(), consolaFont.baseSize, 2);

			int cursorXPos = 0;
			if(charSize.x == 0 && charSize.y == 0){
				charSize = MeasureTextEx(consolaFont, "s", consolaFont.baseSize, 2);
				cursorXPos = 0;
			}else{
				cursorXPos = colIndex*(charSize.x/currentLine.length());
			}
			int cursorYPos = lineIndex*50;

			int cursorWidth = (currentLine.length() == 0) ? charSize.x : charSize.x/currentLine.length();

			DrawRectangle(cursorXPos, cursorYPos, cursorWidth, charSize.y, cursorColour);

			// Draw the status bar

			drawTextCentreAligned(currentFilePath.c_str(), {0, 0}, consolaFont, WHITE);
			
		}else{
			float menuTextSpacing = GetScreenHeight()/4;

			drawTextCentreAligned("Welcome to cupycode!", {0, menuTextSpacing}, consolaFont, WHITE);
			drawTextCentreAligned("CTRL + N to create a new file", {0, 0}, consolaFont, WHITE);
			drawTextCentreAligned("CTRL + O to open file", {0, -1*menuTextSpacing}, consolaFont, WHITE);
		}

		EndDrawing();
	}

	UnloadFont(consolaFont);

	CloseWindow();

	return 0;
}