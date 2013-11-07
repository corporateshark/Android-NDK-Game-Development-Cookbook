#pragma once

static const int NUM_SHAPES = 22;
static const int SHAPES_X = 5;
static const int SHAPES_Y = 5;
static const int ROTATIONS = 4;

/**
   1: Dot
   2: I2
   3: I3, L2
   4: Square, I4, L, L-mirrored, N, N-mirrored
   5: F, P, P-mirrored, S, T, U, V, W, X, Y, Z
**/

extern char Shapes [ NUM_SHAPES ][ ROTATIONS ][ SHAPES_X ][ SHAPES_Y ];
