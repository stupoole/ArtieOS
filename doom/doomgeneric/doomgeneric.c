#include "stdio.h"
#include "m_argv.h"

#include "doomgeneric.h"
#include "d_main.h"

pixel_t* DG_ScreenBuffer = NULL;
boolean doom_is_running;

void M_FindResponseFile(void);
void D_DoomMain(void);


void doomgeneric_Create(int argc, char** argv)
{
    // save arguments
    myargc = argc;
    myargv = argv;
    doom_is_running = true;

    // M_FindResponseFile();

    DG_ScreenBuffer = malloc(DOOMGENERIC_RESX * DOOMGENERIC_RESY * 4);

    DG_Init();

    D_DoomMain();
}
