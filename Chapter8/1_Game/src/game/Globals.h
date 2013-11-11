/*
 * Copyright (C) 2013 Sergey Kosarevsky (sk@linderdaum.com)
 * Copyright (C) 2013 Viktor Latypov (vl@linderdaum.com)
 * Based on Linderdaum Engine http://www.linderdaum.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must display the names 'Sergey Kosarevsky' and
 *    'Viktor Latypov'in the credits of the application, if such credits exist.
 *    The authors of this work must be notified via email (sk@linderdaum.com) in
 *    this case of redistribution.
 *
 * 3. Neither the name of copyright holders nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS
 * IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL COPYRIGHT HOLDERS OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

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
