#pragma once

#include <citro2d.h>

#include "video.h"

extern Handle y2rEvent;

typedef struct theora_3ds_vframe {
	C2D_Image img;
	C3D_Tex buff[2];
	bool curbuf;
} TH3DS_Frame;

int frameInit(TH3DS_Frame* vframe, THEORA_videoinfo* info);
void frameDelete(TH3DS_Frame* vframe);
void frameWrite(TH3DS_Frame* vframe, THEORA_videoinfo* info, th_ycbcr_buffer ybr);
bool frameDrawAtCentered(TH3DS_Frame* vframe, float x, float y, float depth, float scaleX, float scaleY);
