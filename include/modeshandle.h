#ifndef MODESHANDLE_H
#define MODESHANDLE_H
#include <common.h>
#include <symbols.h>
#include <rotation.h>

extern s8 colorOption;
extern s8 fogOption;
extern s8 brightOption;
extern s8 animOption;
extern s8 positionOption;
extern s8 miscOption;

extern bool doLightingUpdate;

extern int* customCam;

extern EXVector savedCamHandlerPos;
extern EXVector savedCamHandlerLook;
extern float savedFOV;

bool headTrackEnabled();
void setHeadTracking(bool enable);

void updateCameraViewport();
void resetCamPosition();

EXVector* getItemPosition(int* item);
EXVector* getItemRotation(int* item);

extern EXVector savedPlayerRot;
extern EXVector savedPlayerPos;

extern EXVector selectedItemRot;
extern EXVector selectedItemPos;

extern EXDListItem* selectedItem;

extern bool doCameraFollow;

void EXVector_Copy(EXVector* dest, EXVector* src);
void EXVector_Add(EXVector* dest, EXVector* add);

EXVector* mat_44_get_position(mat44* mat, EXVector* outvct);
EXVector* mat_44_get_rotation(mat44* mat, EXVector* outvct);

void updateAnimatorMatrix();
int* getItemAnimator(int* item);
mat44* getAnimatorMatrix(int* anim);

EXVector* getItemPosition(int* item);
EXVector* getItemRotation(int* item);

int* getZoneInfo();
DrawRender* getZoneFogInfo();
XRGBA* getZoneFogColour();
bool* getZoneFogEnabled();

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

void doAnimControls();

//"Position" Mode
void doPositionControls();

//"Misc" Mode
void doMiscControls();

void doItemControls();

#endif /* MODESHANDLE_H */