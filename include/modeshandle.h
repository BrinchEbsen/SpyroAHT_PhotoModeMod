#ifndef MODESHANDLE_H
#define MODESHANDLE_H
#include <common.h>
#include <symbols.h>
#include <rotation.h>

extern int colorOption;
extern int fogOption;
extern int brightOption;
extern int miscOption;

extern bool doFrameAdvance;

extern bool doLightingUpdate;

extern int* customCam;

extern EXVector savedCamHandlerPos;
extern EXVector savedCamHandlerLook;

bool headTrackEnabled();
void setHeadTracking(bool enable);

//Get the player's animator
int* getPlayerAnimator();
//Get current anim mode hash
uint getCurrentAnimMode();
//If player is in Scanmode
bool playerInScanMode();

//Handle menu while in scanmode
void doScanmodeControls();

//"Move Camera" Mode
void doCamControls();

//"Screen Tint" Mode
void doColorControls();

//"Brightness" Mode
void doBrightnessControls();

//"Fog" Mode
void doFogControls();

//"Misc" Mode
void doMiscControls();

#endif /* MODESHANDLE_H */