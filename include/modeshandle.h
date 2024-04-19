#ifndef MODESHANDLE_H
#define MODESHANDLE_H
#include <common.h>
#include <symbols.h>
#include <rotation.h>

extern s8 colorOption;
extern s8 fogOption;
extern s8 brightOption;
extern s8 positionOption;
extern s8 miscOption;

extern bool doFrameAdvance;

extern bool doLightingUpdate;

extern int* customCam;

extern EXVector savedCamHandlerPos;
extern EXVector savedCamHandlerLook;

bool headTrackEnabled();
void setHeadTracking(bool enable);

EXVector* getPlayerPosition();
EXVector* getPlayerRotation();

extern EXVector savedPlayerRot;
extern EXVector savedPlayerPos;

extern EXVector currentPlayerRot;
extern EXVector currentPlayerPos;

void updateAnimatorMatrix();

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

//"Position" Mode
void doPositionControls();

//"Misc" Mode
void doMiscControls();

#endif /* MODESHANDLE_H */