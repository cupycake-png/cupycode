#include "raylib.h"
#include "resource_dir.h"

void drawTextCentreAligned(const char* text, Vector2 centrePos, Font font){
	
}

int main(){
	Color bg = {30, 23, 41, 255};

	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);

	InitWindow(1280, 800, "cupycode");

	SearchAndSetResourceDir("resources");

	Font consolaFont = LoadFontEx("CONSOLA.TTF", 50, 0, 250);

	SetTargetFPS(60);

	while(!WindowShouldClose()){
		BeginDrawing();

		ClearBackground(bg);

		Vector2 size = MeasureTextEx(consolaFont, "Welcome to cupycode", consolaFont.baseSize, 2);

		DrawTextEx(consolaFont, "Welcome to cupycode", Vector2{(float)GetScreenWidth()/2 - size.x/2, (float)GetScreenHeight()/2 - size.y/2}, (float)consolaFont.baseSize, 2, WHITE);

		EndDrawing();
	}

	UnloadFont(consolaFont);

	CloseWindow();

	return 0;
}