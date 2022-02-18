#ifndef VIDEO_3DS_H
#define VIDEO_3DS_H

extern int isplaying;
extern THEORA_Context vidCtx;
extern TH3DS_Frame frame;
extern float scaleframe;
extern bool videodone;
#if RETRO_RENDERTYPE == RETRO_SW_RENDER
extern C3D_RenderTarget* topScreen;
#endif
void PlayVideo(const char* fileName);
void CloseVideo();

#endif
