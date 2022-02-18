#include "RetroEngine.hpp"

#if RETRO_PLATFORM == RETRO_WII
#include <ogcsys.h>
#include <gccore.h>
#include <wiiuse/wpad.h>

static void *xfb;
static GXRModeObj *rmode;
#endif

#if !RETRO_USE_ORIGINAL_CODE

#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
#include "Windows.h"
#endif

void parseArguments(int argc, char *argv[]) {
    for (int a = 0; a < argc; ++a) {
        const char *find = "";

        find = strstr(argv[a], "stage=");
        if (find) {
            int b = 0;
            int c = 6;
            while (find[c] && find[c] != ';') Engine.startSceneFolder[b++] = find[c++];
            Engine.startSceneFolder[b] = 0;
        }

        find = strstr(argv[a], "scene=");
        if (find) {
            int b = 0;
            int c = 6;
            while (find[c] && find[c] != ';') Engine.startSceneID[b++] = find[c++];
            Engine.startSceneID[b] = 0;
        }

        find = strstr(argv[a], "console=true");
        if (find) {
            engineDebugMode       = true;
            Engine.devMenu        = true;
            Engine.consoleEnabled = true;
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
            AllocConsole();
            freopen_s((FILE **)stdin, "CONIN$", "w", stdin);
            freopen_s((FILE **)stdout, "CONOUT$", "w", stdout);
            freopen_s((FILE **)stderr, "CONOUT$", "w", stderr);
#endif
        }

        find = strstr(argv[a], "usingCWD=true");
        if (find) {
            usingCWD = true;
        }
    }
}
#endif

#if RETRO_PLATFORM == RETRO_WII
void initializeSelection() {
  
	VIDEO_Init();
	WPAD_Init();
 
	rmode = VIDEO_GetPreferredMode(NULL);

	xfb = MEM_K0_TO_K1(SYS_AllocateFramebuffer(rmode));
	console_init(xfb,20,20,rmode->fbWidth,rmode->xfbHeight,rmode->fbWidth*VI_DISPLAY_PIX_SZ);
 
	VIDEO_Configure(rmode);
	VIDEO_SetNextFramebuffer(xfb);
	VIDEO_SetBlack(FALSE);
	VIDEO_Flush();
	VIDEO_WaitVSync();
	if(rmode->viTVMode&VI_NON_INTERLACE) VIDEO_WaitVSync();
}
#endif

int main(int argc, char *argv[])
{
#if RETRO_PLATFORM == RETRO_WII
    initializeSelection();
    printf("Press A if you want the game at 4:3 with a higher framerate and bigger screen.\n");
    printf("Press B if you want the game at 16:9 with a lower framerate and smaller screen.\n");
    printf("Press HOME to exit.\n");
	while(1) {

		WPAD_ScanPads();

		u16 buttonsDown = WPAD_ButtonsDown(0);
		
		if(buttonsDown & WPAD_BUTTON_A) {
			DEFAULT_SCREEN_XSIZE = 320;
            break;
		}	
		if(buttonsDown & WPAD_BUTTON_B) {
			DEFAULT_SCREEN_XSIZE = 424;
            break;
		}	
	}
#endif

#if !RETRO_USE_ORIGINAL_CODE
    parseArguments(argc, argv);
#endif

    Engine.Init();
    Engine.Run();

#if !RETRO_USE_ORIGINAL_CODE
    if (Engine.consoleEnabled) {
#if RETRO_PLATFORM == RETRO_WIN && _MSC_VER
        FreeConsole();
#endif
    }
#endif


    return 0;
}
