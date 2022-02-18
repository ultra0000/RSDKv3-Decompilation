#include <stdio.h>
#include <unistd.h>

#include <3ds.h>
#include <citro3d.h>
#include <citro2d.h>

#include "video.h"
#include "frame.h"
#include "explorer.h"

#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define WAVEBUFCOUNT 3
#define	MAX_LIST     28

C3D_RenderTarget* top;
THEORA_Context vidCtx;
TH3DS_Frame frame;
Thread vthread = NULL;
Thread athread = NULL;
static size_t buffSize = 8 * 4096;
static ndspWaveBuf waveBuf[WAVEBUFCOUNT];
int16_t* audioBuffer;
LightEvent soundEvent;
int ready = 0;
float scaleframe = 1.0f;

int isplaying = false;

static inline float getFrameScalef(float wi, float hi, float targetw, float targeth) {
	float w = targetw/wi;
	float h = targeth/hi;
	return fabs(w) > fabs(h) ? h : w;
}

void audioInit(THEORA_audioinfo* ainfo) {
	ndspChnReset(0);
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspChnSetInterp(0, ainfo->channels == 2 ? NDSP_INTERP_POLYPHASE : NDSP_INTERP_LINEAR);
	ndspChnSetRate(0, ainfo->rate);
	ndspChnSetFormat(0, ainfo->channels == 2 ? NDSP_FORMAT_STEREO_PCM16 : NDSP_FORMAT_MONO_PCM16);
	audioBuffer = linearAlloc((buffSize * sizeof(int16_t)) * WAVEBUFCOUNT);

	memset(waveBuf, 0, sizeof(waveBuf));
	for (unsigned i = 0; i < WAVEBUFCOUNT; ++i)
	{
		waveBuf[i].data_vaddr = &audioBuffer[i * buffSize];
		waveBuf[i].nsamples = buffSize;
		waveBuf[i].status = NDSP_WBUF_DONE;
	}
}

void audioClose(void) {
	ndspChnReset(0);
	if (audioBuffer) linearFree(audioBuffer);
}

void videoDecode_thread(void* nul) {
	THEORA_videoinfo* vinfo = THEORA_vidinfo(&vidCtx);
	THEORA_audioinfo* ainfo = THEORA_audinfo(&vidCtx);

	if (THEORA_HasAudio(&vidCtx))
		audioInit(ainfo);

	if (THEORA_HasVideo(&vidCtx)) {
		printf("Ogg stream is Theora %dx%d %.02f fps\n", vinfo->width, vinfo->height, vinfo->fps);
		frameInit(&frame, vinfo);
		scaleframe = getFrameScalef(vinfo->width, vinfo->height, SCREEN_WIDTH, SCREEN_HEIGHT);
	}

	isplaying = true;

	while (isplaying)
	{
		if (THEORA_eos(&vidCtx))
			break;

		if (THEORA_HasVideo(&vidCtx)) {
			th_ycbcr_buffer ybr;
			if (THEORA_getvideo(&vidCtx, ybr)) {
				frameWrite(&frame, vinfo, ybr);
			}
		}

		if (THEORA_HasAudio(&vidCtx)) {
			for (int cur_wvbuf = 0; cur_wvbuf < WAVEBUFCOUNT; cur_wvbuf++) {
				ndspWaveBuf *buf = &waveBuf[cur_wvbuf];

				if(buf->status == NDSP_WBUF_DONE) {
					//__lock_acquire(oggMutex);
					size_t read = THEORA_readaudio(&vidCtx, buf->data_pcm16, buffSize);
					//__lock_release(oggMutex);
					if(read <= 0)
						break;
					else if(read <= buffSize)
						buf->nsamples = read / ainfo->channels;

					ndspChnWaveBufAdd(0, buf);
				}
				DSP_FlushDataCache(buf->data_pcm16, buffSize * sizeof(int16_t));
			}
		}
	}

	printf("frames: %d dropped: %d\n", vidCtx.frames, vidCtx.dropped);

	if (THEORA_HasVideo(&vidCtx))
		frameDelete(&frame);

	if (THEORA_HasAudio(&vidCtx))
		audioClose();

	THEORA_Close(&vidCtx);
	threadExit(0);
}

void audioCallback(void *const arg_)
{
	(void)arg_;

	if (!isplaying)
		return;

	LightEvent_Signal(&soundEvent);
}

/*void audioDecode_thread(void* nul) {
	THEORA_audioinfo* ainfo = THEORA_audinfo(&vidCtx);

	if (THEORA_HasAudio(&vidCtx))
		audioInit(ainfo);

	while (isplaying) {
		if (THEORA_HasAudio(&vidCtx)) {
			for (int cur_wvbuf = 0; cur_wvbuf < WAVEBUFCOUNT; cur_wvbuf++) {
				ndspWaveBuf *buf = &waveBuf[cur_wvbuf];

				if(buf->status == NDSP_WBUF_DONE) {
					__lock_acquire(oggMutex);
					size_t read = THEORA_readaudio(&vidCtx, buf->data_pcm16, buffSize);
					__lock_release(oggMutex);
					if(read <= 0)
						break;
					else if(read <= buffSize)
						buf->nsamples = read / ainfo->channels;

					ndspChnWaveBufAdd(0, buf);
				}
				DSP_FlushDataCache(buf->data_pcm16, buffSize * sizeof(int16_t));
			}
			LightEvent_Wait(&soundEvent);
		}
	}

	//audioClose();

	threadExit(0);
}*/

static void exitThread(void) {
	isplaying = false;

	if (!vthread && !athread)
		return;

	threadJoin(vthread, U64_MAX);
	threadFree(vthread);

	LightEvent_Signal(&soundEvent);
	threadJoin(athread, U64_MAX);
	threadFree(athread);

	vthread = NULL;
	athread = NULL;
}

static int isOgg(const char* filepath) {
	FILE* fp = fopen(filepath, "r");
	char magic[16];

	if (!fp) {
		printf("Could not open %s. Please make sure file exists.\n", filepath);
		return 0;
	}

	fseek(fp, 0, SEEK_SET);
	fread(magic, 1, 16, fp);
	fclose(fp);

	if (!strncmp(magic, "OggS", 4))
		return 1;

	return 0;
}

static void changeFile(const char* filepath) {
	int ret = 0;

	if (vthread != NULL || athread != NULL)
		exitThread();

	if (!isOgg(filepath)) {
		printf("The file is not an ogg file.\n");
		return;
	}

	if ((ret = THEORA_Create(&vidCtx, filepath))) {
		printf("THEORA_Create exited with error, %d.\n", ret);
		return;
	}

	if (!THEORA_HasVideo(&vidCtx) && !THEORA_HasAudio(&vidCtx)) {
		printf("No audio or video stream could be found.\n");
		return;
	}

	printf("Theora Create sucessful.\n");

	s32 prio;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	vthread = threadCreate(videoDecode_thread, NULL, 32 * 1024, prio-1, -1, false);
	//athread = threadCreate(audioDecode_thread, NULL, 32 * 1024, prio-1, -1, false);
}

//---------------------------------------------------------------------------------
int main(int argc, char* argv[]) {
//---------------------------------------------------------------------------------
	dirList_t dirList = {0};
	int fileMax;
	int cursor = 0;
	int from = 0;

	romfsInit();
	ndspInit();
	gfxInitDefault();
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();
	consoleInit(GFX_BOTTOM, NULL);
	hidSetRepeatParameters(25, 5);

	osSetSpeedupEnable(true);

	chdir("sdmc:/");
	chdir("videos");

	// Create screens
	top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);

	//printf("%s.\n", th_version_string());

	fileMax = getDir(&dirList);
	printDir(from, MAX_LIST, 0, dirList);

	ndspSetCallback(audioCallback, NULL);

	while(aptMainLoop()){
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kDownRepeat = hidKeysDownRepeat();

		if (kDown & KEY_START)
			break;

		if (!isplaying) {
			if((kDown & KEY_UP || (kDownRepeat & KEY_UP)) && cursor > 0)
			{
				cursor--;

				/* 26 is the maximum number of entries that can be printed */
				if(fileMax - cursor > 26 && from != 0)
					from--;

				printDir(from, MAX_LIST, cursor, dirList);
			}

			if((kDown & KEY_DOWN || (kDownRepeat & KEY_DOWN)) && cursor < fileMax)
			{
				cursor++;

				if(cursor >= MAX_LIST && fileMax - cursor >= 0 &&
						from < fileMax - MAX_LIST)
					from++;

				printDir(from, MAX_LIST, cursor, dirList);
			}

			/*
			 * Pressing B goes up a folder, as well as pressing A or R when ".."
			 * is selected.
			 */
			if((kDown & KEY_B) || ((kDown & KEY_A) && (from == 0 && cursor == 0)))
			{
				chdir("..");
				consoleClear();
				fileMax = getDir(&dirList);

				cursor = 0;
				from = 0;

				printDir(from, MAX_LIST, cursor, dirList);

				continue;
			}

			if(kDown & KEY_A) {
				if(dirList.dirNum >= cursor) {
					chdir(dirList.directories[cursor - 1]);
					consoleClear();
					cursor = 0;
					from = 0;
					fileMax = getDir(&dirList);

					printDir(from, MAX_LIST, cursor, dirList);
					continue;
				}

				if(dirList.dirNum < cursor) {
					changeFile(dirList.files[cursor - dirList.dirNum - 1]);
					continue;
				}
			}
		} else {
			if (kDown & KEY_B) {
				exitThread();
				printDir(from, MAX_LIST, cursor, dirList);
				continue;
			}

			if (kDown & KEY_Y) {
				//printf("Ypress\n");
				if (scaleframe == 1.0f) {
					THEORA_videoinfo* vinfo = THEORA_vidinfo(&vidCtx);
					scaleframe = getFrameScalef(vinfo->width, vinfo->height, SCREEN_WIDTH, SCREEN_HEIGHT);
					//printf("sf %f\n", scaleframe);
				} else {
					scaleframe = 1.0f;
				}
			}

			if (kDown & KEY_X) {
				printf("frames: %d dropped: %d\n", vidCtx.frames, vidCtx.dropped);
			}
		}

		C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
			C2D_TargetClear(top, C2D_Color32(20, 29, 31, 255));
			C2D_SceneBegin(top);
			if (isplaying && THEORA_HasVideo(&vidCtx))
				frameDrawAtCentered(&frame, SCREEN_WIDTH/2, SCREEN_HEIGHT/2, 0.5f, scaleframe, scaleframe);
		C3D_FrameEnd(0);
	}

	exitThread();

	osSetSpeedupEnable(false);

	gfxExit();
	ndspExit();
	romfsExit();
	return 0;
}
