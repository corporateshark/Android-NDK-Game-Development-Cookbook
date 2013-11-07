#pragma once

#include "Engine.h"

struct sGameState
{
	sGameState() {}

	void Reset();

	void SwitchColor();

	static int SelectDifferentColor( int OldColor );

	bool   FGameOver;
	float  FGameTime;
	float  FGameTimeCount;
	float  FUpdateSpeed;
	int    FScore;
	int    FLevel;
	int    FCurrentColor;
	int    FNextColor;

	int FCurX;
	int FCurY;
};

extern sGameState g_GS;

const float Field_X1 = 0.185f;
const float Field_Y1 = 0.085f;
const float Field_Width = 0.8f;
const float Field_Height = 0.71f;
const int   BlocksToDisappear = 3; // number of blocks to disappear

const int b_MoveLeft = 0;
const int b_Down = 1;
const int b_MoveRight = 2;
const int b_TurnLeft = 3;
const int b_TurnRight = 4;
const int b_Reset = 5;
const int b_Paused = 6;

const float NextLevelTime = 30.0f; // seconds

const int NUM_BRICK_IMAGES = 4;
const float BLOCK_SIZE = 16.0f;
const float BLOCK_SIZE_SMALL = 14.5f;
const int FIELD_INVISIBLE_RAWS = 2;

const float g_KeyTypematicDelay = 0.2f;  // 200 ms delay
const float g_KeyTypematicRate  = 0.03f; // 33 Hz repeat rate
