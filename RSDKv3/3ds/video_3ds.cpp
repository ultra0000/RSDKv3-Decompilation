#include "../RetroEngine.hpp"
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240

#define WAVEBUFCOUNT 3
#define	MAX_LIST     28

// Citro2D should always be used for rendering video,
// across both hardware and software builds

// following code ripped from 3ds-theoraplayer's main.c
C3D_RenderTarget* topScreen;
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
bool videodone = false;

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
	audioBuffer = (int16_t*) linearAlloc((buffSize * sizeof(int16_t)) * WAVEBUFCOUNT);

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

	audioInit(ainfo);

	if (THEORA_HasVideo(&vidCtx)) {
		//printf("Ogg stream is Theora %dx%d %.02f fps\n", vinfo->width, vinfo->height, vinfo->fps);
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
					size_t read = THEORA_readaudio(&vidCtx, (char*) buf->data_pcm16, buffSize);
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

	//printf("frames: %d dropped: %d\n", vidCtx.frames, vidCtx.dropped);
	videodone = true;

	if (THEORA_HasVideo(&vidCtx))
		frameDelete(&frame);

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

	//printf("Theora Create sucessful.\n");

	s32 prio;
	svcGetThreadPriority(&prio, CUR_THREAD_HANDLE);
	vthread = threadCreate(videoDecode_thread, NULL, 32 * 1024, prio-1, -1, false);
}
// end ripped code

void PlayVideo(const char* fileName) {
	// de-init Retro Engine audio thread
	ReleaseAudioDevice();
	ndspInit();
	ndspSetCallback(audioCallback, NULL);
	//printf("Loading from %s\n", fileName);

#if !RETRO_USING_C2D
	// using the software-rendered build; init Citro2D here
	gfxSetScreenFormat(GFX_TOP, GSP_BGR8_OES);
	
	C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
	C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
	C2D_Prepare();

	topScreen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
#else
	gfxSet3D(false);
#endif

	changeFile(fileName);
	videodone = false;
	videoPlaying = true;
}

void CloseVideo() {
	printLog("video done, attempting to de-init");
	exitThread();

	// re-init Retro Engine audio thread
	ndspExit();
	ndspInit();
	InitAudioPlayback();

#if !RETRO_USING_C2D
	C2D_Fini();
	C3D_Fini();

	gfxSetScreenFormat(GFX_TOP, GSP_RGB565_OES);
#else
	gfxSet3D(true);
#endif
}
