
#include "raylib.h"
#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <time.h>
#include <algorithm>
#include <math.h>
#include <fstream>

using namespace std;

// global variables
int GAME_WIDTH = 1280, GAME_HEIGHT = 800, AMOUNT_OF_MOUSE_INPUTS_SAVED = 8, MOUSE_INPUTS_USED_FOR_COLLISION = 1, TEXTURE_SIZE = 256, FPS_COUNT = 240, CUT_DISTANCE_REQUIRED = 6, LIVE_COUNT = 3, SCORE = 0, HIGH_SCORE = 0, FONT_SIZE = 75, MAX_AMOUNT_OF_SPLATTERS = 150;
int GAME_STATE = 0; // 0 = start menu, 1 - playing, 2 - game over
string GAME_TITLE = "Fruit Ninja Fangame";
int foodGenFrameCounter = 0, framesAfterGameOver = 0, foodGenTicks = 0, foodGenVariable=0;
vector<Vector2> mouseMovement;
string fileString;
// audio & visual global variables
int smudgeRadius = 10;
float HEART_TEXTURE_SCALE = 0.35, GAME_OVER_TIME = 0;
string startMenuText = "Slice to start", gameOverText1 = "Game over", gameOverText2 = "Slice to play again", highScoreString="";
Color smudgeColor = RAYWHITE, textColor = BLACK, fruitSplatColors[9] = { {141, 230, 158, 255}, {251, 255, 188, 255}, {255, 251, 102, 255}, {226, 255, 127, 255}, {225, 60, 71, 255}, {255, 241, 107, 255}, {250, 255, 68, 255}, {255, 167, 34, 255}, {255, 177, 60, 255} };
Texture2D textures[37];
Sound sfx[9], music[4];
bool IS_SFX_ENABLED = true, IS_MUSIC_ENABLED = true;
float masterVolume = 0.5;

int checkCutDistance() {
	int totalDistance = 0;
	if (mouseMovement.size() > MOUSE_INPUTS_USED_FOR_COLLISION) {
		for (int i = mouseMovement.size() - 1; i > mouseMovement.size() - MOUSE_INPUTS_USED_FOR_COLLISION-1; i--) {
			totalDistance += abs(mouseMovement[i].x - mouseMovement[i - 1].x);
			totalDistance += abs(mouseMovement[i].y - mouseMovement[i - 1].y);
		}
	}
	return totalDistance;
}
Vector2 CheckCutDistanceWithDirection() {
	Vector2 totalDistance = { 0,0 };
	if (mouseMovement.size() > MOUSE_INPUTS_USED_FOR_COLLISION) {
		for (int i = mouseMovement.size() - 1; i > mouseMovement.size() - MOUSE_INPUTS_USED_FOR_COLLISION - 1; i--) {
			totalDistance.x += (mouseMovement[i].x - mouseMovement[i - 1].x);
			totalDistance.y += (mouseMovement[i].y - mouseMovement[i - 1].y);
		}
	}
	return totalDistance;
}

class fruitHalf {
public:
	float centerX, centerY, xVel, yVel, frameCounter, scale, rotation;
	unsigned int textureIndex;
	void update() {
		centerX += xVel;
		centerY -= yVel;
		rotation += (xVel * 1.2) * GetFrameTime() / 0.016666;
		frameCounter++;
		if (frameCounter > 10 * GetFrameTime() / 0.016666) {
			frameCounter = 0;
			yVel -= (0.23 * GetFrameTime() / 0.016666);
		}
	}
	void draw() {
		DrawTexturePro(textures[textureIndex], { 0, 0, 128, 256 }, { centerX, centerY, TEXTURE_SIZE/2 * scale, TEXTURE_SIZE * scale }, { (float)(TEXTURE_SIZE * scale / 2/2), (float)(TEXTURE_SIZE * scale / 2) }, rotation, WHITE);

	}
private:
};
struct Splatter {
	float centerX, centerY, scale, rotation;
	Color tintOnTexture;
};
class Button {
public:
	float centerX, centerY, scale;
	unsigned int textureIndex;
	Texture2D texture;
	void draw() {
		DrawTexturePro(texture, { 0, 0, 256, 256 }, { centerX, centerY, TEXTURE_SIZE * scale, TEXTURE_SIZE * scale }, { (float)(TEXTURE_SIZE * scale / 2), (float)(TEXTURE_SIZE * scale / 2) }, 0, BLACK);
	}
	bool isBeingPressed() {
		if (CheckCollisionPointRec(GetMousePosition(), {(float)(centerX - 0.5*TEXTURE_SIZE*scale), (float)(centerY - 0.5 * TEXTURE_SIZE * scale),(float)(TEXTURE_SIZE * scale), (float)(TEXTURE_SIZE * scale) }) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
			return true;
		};
		return false;
	}
};
class Cuttable {
public:
	float hitboxRadius, centerX, centerY, xVel, yVel, frameCounter, scale, rotation;
	int commonPoints; // used for collision with mouse
	unsigned int textureIndex;
	bool isBomb = false;
	void update() {
		centerX += xVel;
		centerY -= yVel;
		rotation += (xVel*1.2) * GetFrameTime() / 0.016666;
		if (rotation > 360) {
			rotation -= 360;
		}
		frameCounter++;
		if (frameCounter > 10 * GetFrameTime() / 0.016666) {
			frameCounter = 0;
			yVel -= (0.23 *GetFrameTime() / 0.016666);
			if (isBomb) {
				yVel -= (0.05 * GetFrameTime() / 0.016666);
			}
		}
	}
	void draw() {
		DrawTexturePro(textures[textureIndex], { 0, 0, 256, 256 }, { centerX, centerY, TEXTURE_SIZE * scale, TEXTURE_SIZE * scale }, { (float)(TEXTURE_SIZE*scale / 2), (float)(TEXTURE_SIZE*scale / 2) }, rotation, WHITE);
		
	}
	bool detectSlice() {
		if (mouseMovement.size() > MOUSE_INPUTS_USED_FOR_COLLISION+1) {
			for (int i = mouseMovement.size() - 1; i > mouseMovement.size() - 1 - MOUSE_INPUTS_USED_FOR_COLLISION; i--) {
				if (CheckCollisionPointCircle(mouseMovement[i], { centerX, centerY }, hitboxRadius)) {
					commonPoints++;
				}
			}
		}
		if (commonPoints >= MOUSE_INPUTS_USED_FOR_COLLISION && checkCutDistance()>3) {
			commonPoints = 0;
			return true;
		}
		else {
			commonPoints = 0;
			return false;
		}
	}
private:
};

vector<Cuttable> foods;
vector<fruitHalf> fruitHalves;
vector<Splatter> splatters;
Cuttable cuttableToPush;
fruitHalf fruitHalfToPush;
Splatter splatterToPush;
Button buttonToPush;
vector<Button> buttons;
long long int howManyCuttablesToGenerate;



void FoodGenerator() {
	foodGenFrameCounter++;
	foodGenVariable = max(120, FPS_COUNT - foodGenTicks/2 - rand() % 31 + 30);
	if (foodGenFrameCounter > foodGenVariable) {
		//cout << foodGenFrameCounter << endl;
		foodGenFrameCounter = 0;
		foodGenTicks++;
		howManyCuttablesToGenerate = rand() % 4;
		for (int i = 0; i < howManyCuttablesToGenerate; i++) {
			cuttableToPush.centerX = GAME_WIDTH / 2 + (float)((rand() % 400) - 200);;
			cuttableToPush.centerY = GAME_HEIGHT + 100;
			cuttableToPush.rotation = 45;
			cuttableToPush.scale = (float)((rand() % 6) / 10 + 0.5);
			cuttableToPush.isBomb = false;
			if (rand() % 10 == 9) {
				cuttableToPush.isBomb = true;
				cuttableToPush.textureIndex = 9;
				cuttableToPush.hitboxRadius = TEXTURE_SIZE * cuttableToPush.scale * 0.85 / 2;
			}
			if (!cuttableToPush.isBomb) {
				cuttableToPush.textureIndex = rand() % 9;
				cuttableToPush.hitboxRadius = (TEXTURE_SIZE * cuttableToPush.scale*1.1)/2;
			}
			cuttableToPush.xVel = (float)((rand() % 8) - 4) * GetFrameTime() / 0.016666;
			cuttableToPush.yVel = (float)((rand() % 3) + 17) * GetFrameTime() / 0.016666;
			foods.push_back(cuttableToPush);
			if (IS_SFX_ENABLED) {
				PlaySound(sfx[6+rand()%3]);
			}
		}
	}
}
void MouseInputFunction() {
	if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
		mouseMovement.push_back(GetMousePosition());
	}
	else {
		mouseMovement.clear();
	}
	if (mouseMovement.size() > AMOUNT_OF_MOUSE_INPUTS_SAVED) {
		mouseMovement.erase(mouseMovement.begin());
	}
}
void setupFruitHalves(unsigned int iterator) {
	fruitHalfToPush.rotation = foods[iterator].rotation;
	fruitHalfToPush.centerX = foods[iterator].centerX - abs(25 * cos(180 / PI * fruitHalfToPush.rotation));
	fruitHalfToPush.centerY = foods[iterator].centerY - abs(25 * sin(180 / PI * fruitHalfToPush.rotation));
	fruitHalfToPush.xVel = foods[iterator].xVel + CheckCutDistanceWithDirection().x / (rand()% 5 + 25);
	fruitHalfToPush.yVel = foods[iterator].yVel - CheckCutDistanceWithDirection().y / (rand() % 5 + 20);
	fruitHalfToPush.scale = foods[iterator].scale;
	fruitHalfToPush.rotation = foods[iterator].rotation;
	fruitHalfToPush.textureIndex = foods[iterator].textureIndex * 2 + 12;
	fruitHalves.push_back(fruitHalfToPush);
	fruitHalfToPush.rotation = foods[iterator].rotation;
	fruitHalfToPush.centerX = foods[iterator].centerX + abs(25 * cos(180 / PI * fruitHalfToPush.rotation));
	fruitHalfToPush.centerY = foods[iterator].centerY + abs(25 * sin(180 / PI * fruitHalfToPush.rotation));
	
	fruitHalfToPush.xVel = foods[iterator].xVel + CheckCutDistanceWithDirection().x / (rand() % 5 + 25);
	fruitHalfToPush.yVel = foods[iterator].yVel - CheckCutDistanceWithDirection().y / (rand() % 5 + 20);
	fruitHalfToPush.scale = foods[iterator].scale;
	fruitHalfToPush.textureIndex = foods[iterator].textureIndex * 2 + 12 + 1;
	fruitHalves.push_back(fruitHalfToPush);
}
void setupSplatter(unsigned int iterator) {
	splatterToPush.centerX = foods[iterator].centerX;
	splatterToPush.centerY = foods[iterator].centerY;
	splatterToPush.tintOnTexture = fruitSplatColors[foods[iterator].textureIndex];
	splatterToPush.scale = foods[iterator].scale * (0.85 + (float)(rand() % 5) / 10);
	splatterToPush.rotation = rand() % 361;
	splatters.push_back(splatterToPush);
}
void setupButtons() {
	for (int i = 0; i < 2; i++) {
		for (int j = 1; j < 2; j++) {
			buttonToPush.scale = 0.35;
			buttonToPush.centerX = GAME_WIDTH - TEXTURE_SIZE * buttonToPush.scale * j - 15;
			buttonToPush.centerY = GAME_HEIGHT - TEXTURE_SIZE * buttonToPush.scale * i - 0.5 * TEXTURE_SIZE * buttonToPush.scale - 15;
			buttonToPush.texture = textures[33 + j + i * 2];
			buttonToPush.textureIndex = 33 + j + i * 2;
			buttons.push_back(buttonToPush);
		}
	}
}

int main()
{	
	// set up the window
	srand(time(NULL));
	InitWindow(GAME_WIDTH, GAME_HEIGHT, GAME_TITLE.c_str());
	InitAudioDevice();
	string appDirectory = GetApplicationDirectory();
	ifstream highScoreFileRead;
	ofstream highScoreFileWrite;
	highScoreFileRead.open(appDirectory + "data/save_file.txt");
	if (highScoreFileRead.is_open()) {
		highScoreFileRead >> fileString;
		cout << fileString;
		HIGH_SCORE = stoi(fileString);
		highScoreFileRead.close();
	}
	// loading textures

	
	textures[0] = LoadTexture((appDirectory + ("data/Apple.png")).c_str());
	textures[1] = LoadTexture((appDirectory + ("data/Pear.png")).c_str());
	textures[2] = LoadTexture((appDirectory + ("data/Banana.png")).c_str());
	textures[3] = LoadTexture((appDirectory + ("data/Durian.png")).c_str());
	textures[4] = LoadTexture((appDirectory + ("data/Watermelon.png")).c_str());
	textures[5] = LoadTexture((appDirectory + ("data/Lemon.png")).c_str());
	textures[6] = LoadTexture((appDirectory + ("data/Pineapple.png")).c_str());
	textures[7] = LoadTexture((appDirectory + ("data/Orange.png")).c_str());
	textures[8] = LoadTexture((appDirectory + ("data/Clementine.png")).c_str());
	textures[9] = LoadTexture((appDirectory + ("data/Bomb.png")).c_str());
	textures[10] = LoadTexture((appDirectory + ("data/Red_Heart.png")).c_str());
	textures[11] = LoadTexture((appDirectory + ("data/Dojo_Background.png")).c_str());
	textures[12] = LoadTexture((appDirectory + ("data/fruitHalves/AppleHalf1.png")).c_str());
	textures[13] = LoadTexture((appDirectory + ("data/fruitHalves/AppleHalf2.png")).c_str());
	textures[14] = LoadTexture((appDirectory + ("data/fruitHalves/PearHalf1.png")).c_str());
	textures[15] = LoadTexture((appDirectory + ("data/fruitHalves/PearHalf2.png")).c_str());
	textures[16] = LoadTexture((appDirectory + ("data/fruitHalves/BananaHalf1.png")).c_str());
	textures[17] = LoadTexture((appDirectory + ("data/fruitHalves/BananaHalf2.png")).c_str());
	textures[18] = LoadTexture((appDirectory + ("data/fruitHalves/DurianHalf1.png")).c_str());
	textures[19] = LoadTexture((appDirectory + ("data/fruitHalves/DurianHalf2.png")).c_str());
	textures[20] = LoadTexture((appDirectory + ("data/fruitHalves/WatermelonHalf2.png")).c_str());
	textures[21] = LoadTexture((appDirectory + ("data/fruitHalves/WatermelonHalf1.png")).c_str());
	textures[22] = LoadTexture((appDirectory + ("data/fruitHalves/LemonHalf1.png")).c_str());
	textures[23] = LoadTexture((appDirectory + ("data/fruitHalves/LemonHalf2.png")).c_str());
	textures[24] = LoadTexture((appDirectory + ("data/fruitHalves/PineappleHalf2.png")).c_str());
	textures[25] = LoadTexture((appDirectory + ("data/fruitHalves/PineappleHalf1.png")).c_str());
	textures[26] = LoadTexture((appDirectory + ("data/fruitHalves/OrangeHalf1.png")).c_str());
	textures[27] = LoadTexture((appDirectory + ("data/fruitHalves/OrangeHalf2.png")).c_str());
	textures[28] = LoadTexture((appDirectory + ("data/fruitHalves/ClementineHalf1.png")).c_str());
	textures[29] = LoadTexture((appDirectory + ("data/fruitHalves/ClementineHalf2.png")).c_str());
	textures[30] = LoadTexture((appDirectory + ("data/splatter.png")).c_str());
	textures[31] = LoadTexture((appDirectory + ("data/settings/music_off.png")).c_str());
	textures[32] = LoadTexture((appDirectory + ("data/settings/sound_off.png")).c_str());
	textures[33] = LoadTexture((appDirectory + ("data/settings/music_on.png")).c_str());
	textures[34] = LoadTexture((appDirectory + ("data/settings/volume_down.png")).c_str());
	textures[35] = LoadTexture((appDirectory + ("data/settings/sound_on.png")).c_str());
	textures[36] = LoadTexture((appDirectory + ("data/settings/volume_up.png")).c_str());
	//index 0-8 - fruit, index 9 - bomb, index 10 - red heart, index 11 - background, 12-29 - fruit halves, 30-36 - buttons
	sfx[0] = LoadSound((appDirectory + ("data/audio/slice1.wav")).c_str());
	sfx[1] = LoadSound((appDirectory + ("data/audio/slice2.wav")).c_str());
	sfx[2] = LoadSound((appDirectory + ("data/audio/slice3.wav")).c_str());
	sfx[3] = LoadSound((appDirectory + ("data/audio/splat1.wav")).c_str());
	sfx[4] = LoadSound((appDirectory + ("data/audio/splat2.wav")).c_str());
	sfx[5] = LoadSound((appDirectory + ("data/audio/explosion.wav")).c_str());
	sfx[6] = LoadSound((appDirectory + ("data/audio/whoosh1.wav")).c_str());
	sfx[7] = LoadSound((appDirectory + ("data/audio/whoosh2.wav")).c_str());
	sfx[8] = LoadSound((appDirectory + ("data/audio/whoosh3.wav")).c_str());
	//
	music[0] = LoadSound((appDirectory + ("data/audio/gameplay_BG1.wav")).c_str());
	music[1] = LoadSound((appDirectory + ("data/audio/gameplay_BG2.wav")).c_str());
	music[2] = LoadSound((appDirectory + ("data/audio/main_menu.wav")).c_str());
	music[3] = LoadSound((appDirectory + ("data/audio/game_over.wav")).c_str());
	//
	Cuttable startFruit;
	startFruit.centerX = GAME_WIDTH / 2;
	startFruit.centerY = GAME_HEIGHT / 2;
	startFruit.scale = (float)(rand()%5 / 10 + 0.8);
	startFruit.textureIndex = rand() % 9;
	startFruit.rotation = rand() % 170 - 85;
	startFruit.hitboxRadius = TEXTURE_SIZE * startFruit.scale / 2;
	setupButtons();

	SetTargetFPS(FPS_COUNT);
	// game loop
	while (!WindowShouldClose())
	{
		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && IS_SFX_ENABLED) {
			PlaySound(sfx[rand() % 3]);
		}
		BeginDrawing();
		if(GAME_STATE == 0){
			//logic
			if (!IsSoundPlaying(music[2]) && IS_MUSIC_ENABLED) {
				PlaySound(music[2]);
			}
			MouseInputFunction();
			if (startFruit.detectSlice()) {
				if (IS_SFX_ENABLED) {
					PlaySound(sfx[3 + rand() % 2]);
				}
				StopSound(music[2]);
				GAME_STATE = 1;
			}
			if (buttons[1].isBeingPressed() && masterVolume < 1.0) {
				masterVolume += 0.005;
				SetMasterVolume(masterVolume);
			}
			if (buttons[0].isBeingPressed() && masterVolume > 0) {
				masterVolume -= 0.005;
				SetMasterVolume(masterVolume);
			}
			//drawing
			DrawTexture(textures[11], 0, 0, WHITE);
			startFruit.draw();
			for (int i = 0; i < buttons.size(); i++) {
				buttons[i].draw();
			}
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				for (int i = 0; i < mouseMovement.size() - 1; i++) {
					DrawLineEx(mouseMovement[i], mouseMovement[i + 1], smudgeRadius * (1 - i / mouseMovement.size()), smudgeColor);
					DrawCircle(mouseMovement[i + 1].x, mouseMovement[i + 1].y, smudgeRadius * (1 - i / mouseMovement.size()) / 2, smudgeColor);
				}
			}
			DrawTextPro(GetFontDefault(), startMenuText.c_str(), {(float)(GAME_WIDTH - FONT_SIZE * startMenuText.size()) + 150, (float)GAME_HEIGHT/4}, {0,0}, 0, FONT_SIZE, 6, textColor);
			
		}
		if (GAME_STATE == 1) {
			//logic
			if (!IsSoundPlaying(music[0]) && !IsSoundPlaying(music[1]) && IS_MUSIC_ENABLED) {
				PlaySound(music[rand()%2]);
			}
			MouseInputFunction();
			FoodGenerator();
			for (int i = 0; i < foods.size(); i++) {
				foods[i].update();
				if (foods[i].detectSlice() && checkCutDistance() >= CUT_DISTANCE_REQUIRED) {
					if (foods[i].isBomb) {
						LIVE_COUNT = 0;
						if (IS_SFX_ENABLED) {
							PlaySound(sfx[5]);
						}
					}
					else {
						if (IS_SFX_ENABLED) {
							PlaySound(sfx[3 + rand() % 2]);
						}
						SCORE++;
						if (SCORE > HIGH_SCORE) {
							HIGH_SCORE = SCORE;
						}
						setupFruitHalves(i);
						setupSplatter(i);
					}
					foods.erase(foods.begin() + i);
				}
				else if (foods[i].centerY > GAME_HEIGHT + 500) {
					if(!foods[i].isBomb){
						LIVE_COUNT--;
					}
					foods.erase(foods.begin() + i);
				}
			}
			for (int i = 0; i < fruitHalves.size(); i++) {
				fruitHalves[i].update();
				if (fruitHalves[i].centerY > GAME_HEIGHT+200) {
					fruitHalves.erase(fruitHalves.begin() + i);
				}
			}
			while (splatters.size() > MAX_AMOUNT_OF_SPLATTERS) {
				splatters.erase(splatters.begin());
			}
			if (LIVE_COUNT < 1) {
				StopSound(music[0]);
				StopSound(music[1]);
				foods.clear();
				fruitHalves.clear();
				splatters.clear();
				GAME_OVER_TIME = GetTime();
				SCORE = 0;
				startFruit.textureIndex = rand() % 9;
				foodGenTicks = 0;
				GAME_STATE = 2;
			}
			// drawing
			DrawTexture(textures[11], 0, 0, WHITE);
			for (int i = 0; i < splatters.size(); i++) {
				DrawTexturePro(textures[30], { 0, 0, 256, 256 }, { splatters[i].centerX, splatters[i].centerY, TEXTURE_SIZE * splatters[i].scale, TEXTURE_SIZE * splatters[i].scale }, { (float)(TEXTURE_SIZE * splatters[i].scale / 2), (float)(TEXTURE_SIZE * splatters[i].scale / 2) }, splatters[i].rotation, splatters[i].tintOnTexture);
			}
			for (int i = 0; i < fruitHalves.size(); i++) {
				fruitHalves[i].draw();
			}
			for (int i = 0; i < foods.size(); i++) {
				foods[i].draw();
			}
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				for (int i = 0; i < mouseMovement.size() - 1; i++) {
					DrawLineEx(mouseMovement[i], mouseMovement[i + 1], smudgeRadius * (1 - i / mouseMovement.size()), smudgeColor);
					DrawCircle(mouseMovement[i + 1].x, mouseMovement[i + 1].y, smudgeRadius * (1 - i / mouseMovement.size()) / 2, smudgeColor);
				}
			}
			DrawTextPro(GetFontDefault(), to_string(SCORE).c_str(), { (float)(GAME_WIDTH - FONT_SIZE * (to_string(SCORE).size()-1)) / 2, 10 }, { 0,0 }, 0, FONT_SIZE, 6, textColor);
			for (int i = 0; i < LIVE_COUNT; i++) {
				DrawTexturePro(textures[10], { 0, 0, (float)TEXTURE_SIZE, (float)TEXTURE_SIZE }, { i * TEXTURE_SIZE * HEART_TEXTURE_SCALE, 0, (float)(TEXTURE_SIZE * HEART_TEXTURE_SCALE), (float)(TEXTURE_SIZE * HEART_TEXTURE_SCALE) }, { 0, 0 }, 0, WHITE);
			}
		}
		if (GAME_STATE == 2) {
			highScoreFileWrite.open(appDirectory + "data/save_file.txt");
			if (highScoreFileWrite.is_open()) {
				highScoreFileWrite.clear();
				highScoreFileWrite << HIGH_SCORE;
				highScoreFileWrite.close();
			}
			if (!IsSoundPlaying(music[3]) && IS_MUSIC_ENABLED) {
				PlaySound(music[3]);
			}
			MouseInputFunction();
			if (GetTime() - GAME_OVER_TIME > 1 && startFruit.detectSlice()) {
				if (IS_SFX_ENABLED) {
					PlaySound(sfx[3 + rand() % 2]);
				}
				StopSound(music[3]);
				framesAfterGameOver = 0;
				LIVE_COUNT = 3;
				GAME_STATE = 1;
			}
			if (buttons[1].isBeingPressed() && masterVolume < 1.0) {
				masterVolume += 0.005;
				SetMasterVolume(masterVolume);
			}
			if (buttons[0].isBeingPressed() && masterVolume > 0) {
				masterVolume -= 0.005;
				SetMasterVolume(masterVolume);
			}
			//drawing
			DrawTexture(textures[11], 0, 0, WHITE);
			startFruit.draw();
			for (int i = 0; i < buttons.size(); i++) {
				buttons[i].draw();
			}
			if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
				for (int i = 0; i < mouseMovement.size() - 1; i++) {
					DrawLineEx(mouseMovement[i], mouseMovement[i + 1], smudgeRadius * (1 - i / mouseMovement.size()), smudgeColor);
					DrawCircle(mouseMovement[i + 1].x, mouseMovement[i + 1].y, smudgeRadius * (1 - i / mouseMovement.size()) / 2, smudgeColor);
				}
			}
			DrawTextPro(GetFontDefault(), gameOverText1.c_str(), { (float)(GAME_WIDTH - FONT_SIZE * gameOverText1.size() - 125), (float)(GAME_HEIGHT / 11) }, { 0,0 }, 0, FONT_SIZE, 6, textColor);
			DrawTextPro(GetFontDefault(), gameOverText2.c_str(), { (float)(GAME_WIDTH - FONT_SIZE * gameOverText1.size()*1.4), (float)(GAME_HEIGHT / 5)}, {0,0}, 0, FONT_SIZE, 6, textColor);
			highScoreString = "High score:" + to_string(HIGH_SCORE);
			DrawTextPro(GetFontDefault(), highScoreString.c_str(), {(float)(GAME_WIDTH - FONT_SIZE * highScoreString.size() + 400) / 2, (float)GAME_HEIGHT - FONT_SIZE - 10}, {0,0}, 0, FONT_SIZE, 6, textColor);
		}
		ClearBackground(BROWN);
		
		EndDrawing();
	}

	// cleanup
	CloseAudioDevice();
	CloseWindow();
	return 0;
}