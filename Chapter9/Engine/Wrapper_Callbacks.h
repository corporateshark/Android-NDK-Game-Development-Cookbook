#pragma once

#include "VecMath.h"

#include <string>

void ExitApp();

void OnStart( const std::string& RootPath );
void OnStop();
void OnDestroy();
void OnDrawFrame();
void OnTimer( float Delta );
void OnKey( int code, bool state );
void OnMouseDown( int btn, const LVector2& Pos );
void OnMouseMove( const LVector2& Pos );
void OnMouseUp( int btn, const LVector2& Pos );
void GenerateTicks();
