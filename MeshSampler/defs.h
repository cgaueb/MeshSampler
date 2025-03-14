#pragma once

#define TEXFORMAT_RGB 1
#define TEXFORMAT_BGR 2
#define TEXFORMAT_RGBA 3
#define TEXFORMAT_BGRA 4

#define TEXSAMPLING_NEAREST 0
#define TEXSAMPLING_LINEAR 1
#define TEXSAMPLING_SHARP 2
#define TEXSAMPLING_SMOOTH 3

#define PI     3.14159265f
#define LERP(A,B,s) ((A)*(1-s)+(B)*s)
#define COSERP(A,B,t) (LERP(A,B,(1.0f-cosf(t*PI))/2.0f))