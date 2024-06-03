#include <common.h>
#include <symbols.h>
#include <rotation.h>
#include <hashcodes.h>
#include <Sound.h>
#include <screenmath.h>

#define MOVEMODE_POSITION 0
#define MOVEMODE_ROTATION 1

//Positions of menu selection
s8 colorOption = 0;
s8 fogOption = 0;
s8 brightOption = 0;
s8 animOption = 0;
s8 positionOption = 0;
s8 miscOption = 0;

//Whether the camera handler's vectors are updated to make the lighting update
bool doLightingUpdate = true;

int* customCam = NULL;

EXVector savedCamHandlerPos  = {0};
EXVector savedCamHandlerLook = {0};

EXVector savedPlayerRot = {0};
EXVector savedPlayerPos = {0};

EXVector selectedItemRot = {0};
EXVector selectedItemPos = {0};

EXDListItem* selectedItem;

bool doCameraFollow = false;

extern DrawRender storedFogInfo;

void warpCamera(EXVector* newPos) {
    EXVector dist = {
        gCommonCamera.Target.x - gCommonCamera.Position.x,
        gCommonCamera.Target.y - gCommonCamera.Position.y,
        gCommonCamera.Target.z - gCommonCamera.Position.z,
    };

    gCommonCamera.Target.x = newPos->x;
    gCommonCamera.Target.y = newPos->y;
    gCommonCamera.Target.z = newPos->z;

    gCommonCamera.Position.x = newPos->x - dist.x;
    gCommonCamera.Position.y = newPos->y - dist.y;
    gCommonCamera.Position.z = newPos->z - dist.z;
}

void updateCameraViewport() {
    gCommonCamera.Distance = gCommonCamera.Rect.h * 0.5 * (1.0 / ig_tanf(gCommonCamera.VFov * 0.5));
}

void updateLightingVector() {
    if (doLightingUpdate) {
        int* camHandler = *(gpGameWnd + 0x378/4);
        EXVector* camPos  = (EXVector*) (camHandler + 0x298/4);
        EXVector* camLook = (EXVector*) (camHandler + 0x2A8/4);

        camPos->x  = gCommonCamera.Position.x;
        camPos->y  = gCommonCamera.Position.y;
        camPos->z  = gCommonCamera.Position.z;
        camPos->w  = gCommonCamera.Position.w;
        camLook->x = gCommonCamera.Target.x;
        camLook->y = gCommonCamera.Target.y;
        camLook->z = gCommonCamera.Target.z;
        camLook->w = gCommonCamera.Target.w;
    }
}

bool headTrackEnabled() {
    int flags = *(gpPlayer + (0x614/4));

    return (flags & 1) == 1;
}

void setHeadTracking(bool enable) {
    int flags = *(gpPlayer + (0x614/4));

    *(gpPlayer + (0x614/4)) = (flags & ~1) | enable;
}

void EXVector_Copy(EXVector* dest, EXVector* src) {
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
    dest->w = src->w;
}

void EXVector_Add(EXVector* dest, EXVector* add) {
    dest->x = dest->x + add->x;
    dest->y = dest->y + add->y;
    dest->z = dest->z + add->z;
}

float EXVector_Magnitude(EXVector* vct) {
    return ig_sqrtf(
        (vct->x * vct->x) +
        (vct->y * vct->y) +
        (vct->z * vct->z)
    );
}

EXVector* mat_44_get_position(mat44* mat, EXVector* outvct) {
    EXVector_Copy(outvct, &mat->row3);

    return outvct;
}

void mat_44_set_position(mat44* mat, EXVector* newPos) {
    mat->row3.x = newPos->x;
    mat->row3.y = newPos->y;
    mat->row3.z = newPos->z;
}

float mat_44_get_avg_scale(mat44* mat) {
    float sX = EXVector_Magnitude(&mat->row0);
    float sY = EXVector_Magnitude(&mat->row1);
    float sZ = EXVector_Magnitude(&mat->row2);

    return (sX + sY + sZ) / 3.0;
}

void mat_44_change_scale(mat44* mat, float s) {
    mat->row0.x *= s;
    mat->row0.y *= s;
    mat->row0.z *= s;
    mat->row1.x *= s;
    mat->row1.y *= s;
    mat->row1.z *= s;
    mat->row2.x *= s;
    mat->row2.y *= s;
    mat->row2.z *= s;
}

AnimFX_Blink* getPlayerBlinkFX() {
    if (gpPlayer == NULL) { return 0; }

    return (AnimFX_Blink*)(gpPlayer + (0x560/4));
}

int* getItemAnimator(int* item) {
    if (item == NULL) { return 0; }

    return *(item + (0x144/4));
}
mat44* getAnimatorMatrix(int* anim) {
    if (anim == NULL) { return 0; }

    return (mat44*) (anim + (0x90/4));
}

void updateAnimatorMatrix(int* item, EXVector* pos, EXVector* rot) {
    if (item == NULL) { return; }

    int* animator = getItemAnimator(item);

    mat44* mat = getAnimatorMatrix(animator);

    mat_44_set_rotate(mat, rot, 4);
    mat->row3.x = pos->x;
    mat->row3.y = pos->y;
    mat->row3.z = pos->z;
}

EXVector* getItemPosition(int* item) {
    if (item == NULL) { return NULL; }

    return (EXVector*) (item + (0xD0/4));
}

EXVector* getItemRotation(int* item) {
    if (item == NULL) { return NULL; }

    return (EXVector*) (item + (0xE0/4));
}

uint getCurrentAnimMode() {
    if (gpPlayerItem == NULL) { return 0; }
    return *((uint*) (gpPlayerItem + (0x180/4)));
}

bool playerInScanMode() {
    if (gpPlayer == NULL) { return false; }

    return *((bool*) (gpPlayer + (0x588/4)));
}

int* getZoneInfo() {
    if (gpPlayerItem == NULL) {
        return NULL;
    }

    //Get the area the player is in
    u16 mapOn = 0;
    EXVector* vct = getItemPosition(gpPlayerItem);
    GetMapOn(&mapOn, &theItemEnv, vct);

    //Get map id and zone index
    char mapID = (char)(mapOn >> 8);
    byte zoneIndex = (byte)mapOn;

    //Get zone table for current map
    int* mapItem = GetMapItem(&theItemEnv, mapID);
    int* zoneInfoTable = (int*) *(mapItem + (0x124/4));
    
    //Get info for current zone
    return zoneInfoTable + ((zoneIndex*0x88)/4);
}

DrawRender* getZoneFogInfo() {
    int* zoneInfo = getZoneInfo();
    if (zoneInfo == NULL) { return NULL; }

    return (DrawRender*) (zoneInfo+(0x2C/4));
}

XRGBA* getZoneFogColour() {
    int* zoneInfo = getZoneInfo();

    if (zoneInfo == NULL) { return NULL; }

    //Get fog colour value for current zone
    return (XRGBA*) (zoneInfo + 0x3C/4);
}

bool* getZoneFogEnabled() {
    int* zoneInfo = getZoneInfo();

    if (zoneInfo == NULL) { return NULL; }

    //Get fog colour value for current zone
    return (bool*) (((char*)zoneInfo) + 0x40);
}

//Play the SFX "HT_Sound_SFX_GEN_HUD_NPC_CHOOSE" every 5 frames.
void PlayScrollSFX() {
    return; //decided against using it

    static int counter = 0;

    //Play every 5 frames
    counter++;
    counter = counter % 8;

    if (counter == 0) {
        PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
    }
}

void doScanmodeControls() {
    if (gpPlayer == NULL) { return; }
    uint currmode = getCurrentAnimMode();
    uint newMode = currmode;

    if (isButtonPressed(Button_Dpad_Right, g_PadNum)) {
        //ig_printf("before: %x\n", currmode);
        
        //Loop through animmodes until one is found that returns true (aka. it exists)
        while (true) {
            newMode++;
            if (newMode >= HT_AnimMode_HASHCODE_END) {
                newMode = HT_AnimMode_HASHCODE_BASE+1;
            }

            if (Player_ForceModeChange(gpPlayer, newMode)) {
                break;
            }
        }

        PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
    } else if (isButtonPressed(Button_Dpad_Left, g_PadNum)) {
        //ig_printf("before: %x\n", currmode);

        while (true) {
            newMode--;
            if (newMode <= HT_AnimMode_HASHCODE_BASE) {
                newMode = HT_AnimMode_HASHCODE_END-1;
            }

            if (Player_ForceModeChange(gpPlayer, newMode)) {
                break;
            }
        }

        PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
    }

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        Player_ForceModeChange(gpPlayer, currmode);

        PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
    }
    if (isButtonPressed(Button_R, g_PadNum)) {
        setHeadTracking(!headTrackEnabled());

        PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
    }
}

void doCamControls() {
    float lookSensitivity = gCommonCamera.VFov * 0.04;

    //Sensitivity modifiers
    float moveSensitivity = 0.2;
    if (isButtonDown(Button_Y, g_PadNum)) {
        moveSensitivity = 0.7;
    } else if(isButtonDown(Button_X, g_PadNum)) {
        moveSensitivity = 0.03;
    }

    float stickY = Pads_Analog[g_PadNum].LStick_Y;
    float stickX = Pads_Analog[g_PadNum].LStick_X;
    float trigL = Pads_Analog[g_PadNum].LTrigger;
    float trigR = Pads_Analog[g_PadNum].RTrigger;

    //Camera point vector
    float camPointX = CamMatrix.row0.x;
    float camPointZ = CamMatrix.row0.z;

    //Move the camera horizontally with the left stick, vertically with the triggers
    float moveX = stickX * camPointX * moveSensitivity + stickY *   camPointZ  * moveSensitivity;
    float moveZ = stickX * camPointZ * moveSensitivity + stickY * (-camPointX) * moveSensitivity;
    float moveY = (trigR) * moveSensitivity*2 + (-trigL) * moveSensitivity*2;

    //FOV controls
    if (isButtonDown(Button_Dpad_Down, g_PadNum)) {
        gCommonCamera.VFov += 0.01;
    }
    if (isButtonDown(Button_Dpad_Up, g_PadNum)) {
        gCommonCamera.VFov -= 0.01;
    }
    if (isButtonDown(Button_Dpad_Down | Button_Dpad_Up, g_PadNum)) {
        PlayScrollSFX();
    }

    if (gCommonCamera.VFov > 2.0)  { gCommonCamera.VFov = 2.0;  }
    if (gCommonCamera.VFov < 0.05) { gCommonCamera.VFov = 0.05; }

    if (isButtonPressed(Button_B, g_PadNum)) {
        gCommonCamera.VFov = 1.05;
    }

    //Move camera
    gCommonCamera.Position.x += moveX;
    gCommonCamera.Position.z += moveZ;
    gCommonCamera.Position.y += moveY;
    gCommonCamera.Target.x += moveX;
    gCommonCamera.Target.z += moveZ;
    gCommonCamera.Target.y += moveY;

    //Rotate camera
    float angHor = Pads_Analog[g_PadNum].RStick_X * lookSensitivity;
    float angVer = Pads_Analog[g_PadNum].RStick_Y * lookSensitivity;

    //Allocate memory for rotation matrix
    mat44 *rot = (mat44*)EXAlloc(0x40, 0);
    mat_44_set_rotate(rot, &((EXVector){-angVer, -angHor, 0.0, 0.0}), 4);

    //Rotate the target by the camera's matrix
    EXVector v = RotateAroundPoint(&gCommonCamera.Position, &gCommonCamera.Target, &CamMatrix, false);

    //Rotate by our rotation matrix
    v = RotateAroundPoint(&gCommonCamera.Position, &v, rot, false);

    //Reverse the camera's matrix to get the final new position
    gCommonCamera.Target = RotateAroundPoint(&gCommonCamera.Position, &v, &CamMatrix, true);

    //Free memory again
    EXFree(rot);

    //Make the view port size adapt to FOV
    updateCameraViewport();

    //Set camera handler's vectors to match, so the lighting adapts
    updateLightingVector();
}

void doColorControls() {
    float speed = 0.02;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        colorOption++;
        if (colorOption > 4) { colorOption = 0; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        colorOption--;
        if (colorOption < 0) { colorOption = 4; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }

    float rtrig = Pads_Analog[g_PadNum].RTrigger;
    float ltrig = Pads_Analog[g_PadNum].LTrigger;

    if ((rtrig > 0.0) || (ltrig > 0.0)) {
        PlayScrollSFX();
    }

    switch(colorOption) {
        case 0:
            Display_TintRed   += (rtrig * speed) - (ltrig * speed);
            Display_TintGreen += (rtrig * speed) - (ltrig * speed);
            Display_TintBlue  += (rtrig * speed) - (ltrig * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintRed   = 1.0;
                Display_TintGreen = 1.0;
                Display_TintBlue  = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 1:
            Display_TintRed += (rtrig * speed) -  (ltrig * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintRed = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        case 2:
            Display_TintGreen += (rtrig * speed) -  (ltrig * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintGreen = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        case 3:
            Display_TintBlue += (rtrig * speed) -  (ltrig * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintBlue = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        case 4:
            if (isButtonPressed(Button_X, g_PadNum) || isButtonDown(Button_R, g_PadNum)) {
                Display_BloomIntensity++;
            }
            if (isButtonPressed(Button_Y, g_PadNum) || isButtonDown(Button_L, g_PadNum)) {
                Display_BloomIntensity--;
            }

            if (isButtonPressed(Button_X | Button_Y, g_PadNum)) {
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_BloomIntensity = 0x40;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        default:
            break;
    }
}

//UNUSED
void doBrightnessControls() {
   float speed = 0.02;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        brightOption++;
        if (brightOption > 1) { brightOption = 0; }
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        brightOption--;
        if (brightOption < 0) { brightOption = 1; }
    }

    switch(brightOption) {
        case 0:
            GC_Contrast += (Pads_Analog[g_PadNum].RTrigger * speed) - (Pads_Analog[g_PadNum].LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Contrast = 1.11000001;
            }
            break;
        case 1:
            if (isButtonPressed(Button_X, g_PadNum) || isButtonDown(Button_R, g_PadNum)) {
                Display_BloomIntensity++;
            }
            if (isButtonPressed(Button_Y, g_PadNum) || isButtonDown(Button_L, g_PadNum)) {
                Display_BloomIntensity--;
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_BloomIntensity = 0x40;
            }
            break;
        default:
            break;
    }
}

void doFogControls() {
    float speed = 0.02;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        fogOption++;
        if (fogOption > 9) { fogOption = 0; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        fogOption--;
        if (fogOption < 0) { fogOption = 9; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }

    DrawRender* fogInfo = getZoneFogInfo();

    float rtrig = Pads_Analog[g_PadNum].RTrigger;
    float ltrig = Pads_Analog[g_PadNum].LTrigger;

    if (isButtonDown(Button_Y, g_PadNum)) {
        speed = 0.5;
    } else if(isButtonDown(Button_X, g_PadNum)) {
        speed = 0.005;
    }

    switch(fogOption) {
        case 0: //Global fog near scale
            GC_Fog_Near_Scale += (rtrig * speed) - (ltrig * speed);

            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Fog_Near_Scale = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        case 1: //Global fog far scale
            GC_Fog_Far_Scale += (rtrig * speed) - (ltrig * speed);

            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }
            
            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Fog_Far_Scale = 1.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            break;
        case 2: //Zone fog enabled
            if (fogInfo == NULL) { return; }

            if (isButtonPressed(Button_L | Button_R, g_PadNum)) {
                fogInfo->FogEnabled = !(fogInfo->FogEnabled);
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogEnabled = storedFogInfo.FogEnabled;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 3: //Zone fog near
            if (fogInfo == NULL) { return; }

            fogInfo->FogNear += (rtrig * speed) - (ltrig * speed);
            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogNear = storedFogInfo.FogNear;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 4: //Zone fog far
            if (fogInfo == NULL) { return; }

            fogInfo->FogFar += (rtrig * speed) - (ltrig * speed);
            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogFar = storedFogInfo.FogFar;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 5: //Zone fog min
            if (fogInfo == NULL) { return; }

            fogInfo->FogMin += (rtrig * speed) - (ltrig * speed);
            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogMin = storedFogInfo.FogMin;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 6: //Zone fog max
            if (fogInfo == NULL) { return; }

            fogInfo->FogMax += (rtrig * speed) - (ltrig * speed);
            if ((rtrig > 0.0) || (ltrig > 0.0)) {
                PlayScrollSFX();
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogMax = storedFogInfo.FogMax;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            break;
        case 7: //Fog color R
            if (fogInfo == NULL) { return; }

            if (isButtonDown(Button_R, g_PadNum) || isButtonPressed(Button_X, g_PadNum)) {
                fogInfo->FogColor.r += 1;
            }
            if (isButtonDown(Button_L, g_PadNum) || isButtonPressed(Button_Y, g_PadNum)) {
                fogInfo->FogColor.r -= 1;
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogColor.r = storedFogInfo.FogColor.r;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            if (isButtonDown(Button_R | Button_L, g_PadNum)) {
                PlayScrollSFX();
            }
            if (isButtonPressed(Button_X | Button_Y, g_PadNum)) {
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            break;
        case 8: //Fog color G
            if (fogInfo == NULL) { return; }

            if (isButtonDown(Button_R, g_PadNum) || isButtonPressed(Button_X, g_PadNum)) {
                fogInfo->FogColor.g += 1;
            }
            if (isButtonDown(Button_L, g_PadNum) || isButtonPressed(Button_Y, g_PadNum)) {
                fogInfo->FogColor.g -= 1;
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogColor.g = storedFogInfo.FogColor.g;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            if (isButtonDown(Button_R | Button_L, g_PadNum)) {
                PlayScrollSFX();
            }
            if (isButtonPressed(Button_X | Button_Y, g_PadNum)) {
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            break;
        case 9:  //Fog color B
            if (fogInfo == NULL) { return; }

            if (isButtonDown(Button_R, g_PadNum) || isButtonPressed(Button_X, g_PadNum)) {
                fogInfo->FogColor.b += 1;
            }
            if (isButtonDown(Button_L, g_PadNum) || isButtonPressed(Button_Y, g_PadNum)) {
                fogInfo->FogColor.b -= 1;
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                fogInfo->FogColor.b = storedFogInfo.FogColor.b;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            if (isButtonDown(Button_R | Button_L, g_PadNum)) {
                PlayScrollSFX();
            }
            if (isButtonPressed(Button_X | Button_Y, g_PadNum)) {
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            break;
        default:
            break;
    }
}

//UNUSED
void doAnimControls() {
    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        animOption++;
        if (animOption > 1) { animOption = 0; }
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        animOption--;
        if (animOption < 0) { animOption = 1; }
    }

    if (gpPlayerItem == NULL) { return; }
    int* anim = getItemAnimator(gpPlayerItem);

    //Get trigger values
    float RTrigger = Pads_Analog[g_PadNum].RTrigger;
    float LTrigger = Pads_Analog[g_PadNum].LTrigger;

    //Do nothing if the player isn't pressing anything
    if (RTrigger == 0.0 && LTrigger == 0.0) { return; }

    switch (animOption) {
        case 0: //Change player animation timeline
            float currTime = Animator_GetObjectTime(anim);

            //Speed factor (scripts have a default framerate of 30)
            float f = (0.5 / 30.0);

            currTime += f * RTrigger;
            currTime -= f * LTrigger;

            if (currTime < 0.0) { currTime = 0.0; }

            Animator_SetObjectTime(anim, currTime);
            Animator_UpdateObjectTime(anim, currTime);

            break;
        case 1:
            //Get blinking FX pointer
            AnimFX_Blink* blinkFX = getPlayerBlinkFX();
            if (blinkFX == NULL) { return; }

            float m = blinkFX->BlinkMorph;

            float change = (RTrigger - LTrigger) * 0.05;

            m += change;

            if (m < 0.0) { m = 0.0; }
            if (m > 1.0) { m = 1.0; }

            blinkFX->BlinkMorph = m;

            break;
        default:
            break;
    }
}

void doPositionControls(int moveMode) {
    float speed = 0.05;

    if (isButtonDown(Button_Y, g_PadNum)) {
        speed = 0.2;
    } else if(isButtonDown(Button_X, g_PadNum)) {
        speed = 0.01;
    }

    float stickX = Pads_Analog[g_PadNum].LStick_X;
    float stickY = Pads_Analog[g_PadNum].LStick_Y;
    float trigL  = Pads_Analog[g_PadNum].LTrigger;
    float trigR  = Pads_Analog[g_PadNum].RTrigger;

    //Check for no input
    if ( (stickX == 0.0) &&
         (stickY == 0.0) &&
         (trigL  == 0.0) &&
         (trigR  == 0.0) ) {
        return;
    }

    mat44 *mat = getAnimatorMatrix(getItemAnimator((int*)selectedItem));

    EXVector posMove = {0};
    EXVector rotMove = {0};

    switch (moveMode) {
        case MOVEMODE_POSITION:
            //Camera point vector
            float camPointX = CamMatrix.row0.x;
            float camPointZ = CamMatrix.row0.z;

            //Move the horizontally with the left stick, vertically with the triggers
            float moveX = stickX * camPointX + stickY * camPointZ;
            float moveZ = stickX * camPointZ + stickY * (-camPointX);
            float moveY = (trigR)*2 + (-trigL)*2;

            posMove.x += moveX * speed;
            posMove.y += moveY * speed;
            posMove.z += moveZ * speed;

            break;
        case MOVEMODE_ROTATION:
            rotMove.x += stickY * speed;
            rotMove.y += stickX * speed;

            rotMove.z += -(trigL * speed);
            rotMove.z +=   trigR * speed;
        
            break;
        default:
            break;
    }

    EXVector_Add(&selectedItemPos, &posMove);
    EXVector_Add(&selectedItemRot, &rotMove);

    mat_44_set_rotate  (mat, &selectedItemRot, 4);
    mat_44_set_position(mat, &selectedItemPos);
}

void doMiscControls() {
    float speed = 0.01;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        miscOption++;
        if (miscOption > 5) { miscOption = 0; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        miscOption--;
        if (miscOption < 0) { miscOption = 5; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }

    float rtrig = Pads_Analog[g_PadNum].RTrigger;
    float ltrig = Pads_Analog[g_PadNum].LTrigger;

    switch(miscOption) {
        case 0:
            if (isButtonPressed(Button_Y, g_PadNum) || isButtonDown(Button_L, g_PadNum)) {
                engineFrameRate++;
                if (engineFrameRate > 250) { engineFrameRate = 250; }
            }
            if (isButtonPressed(Button_X, g_PadNum) || isButtonDown(Button_R, g_PadNum)) {
                engineFrameRate--;
                if (engineFrameRate < 30) { engineFrameRate = 30; }
            }

            if (isButtonDown(Button_R | Button_L, g_PadNum)) {
                PlayScrollSFX();
            }
            if (isButtonPressed(Button_Y | Button_X, g_PadNum)) {
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            if (isButtonPressed(Button_B, g_PadNum)) {
                engineFrameRate = 60;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }
            
            break;
        case 1:
            if (isButtonPressed(Button_L | Button_R, g_PadNum)) {
                toggleWideScreen();
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            break;
        case 2:
            //Return if no input
            if ( (rtrig == 0.0) && (ltrig == 0.0)) { return; }

            int* anim = getItemAnimator(gpPlayerItem);
            float currTime = Animator_GetObjectTime(anim);

            float f = (1.0 / 60.0);

            currTime += f * rtrig;
            currTime -= f * ltrig;

            if (currTime < 0.0) { currTime = 0.0; }

            Animator_SetObjectTime(anim, currTime);
            Animator_UpdateObjectTime(anim, currTime);

            PlayScrollSFX();

            break;
        case 3:
            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Shadow_Precision_Scale = 2.0;
                PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
            }

            //Return if no input
            if ( (rtrig == 0.0) && (ltrig == 0.0)) { return; }

            GC_Shadow_Precision_Scale += (rtrig * 0.03) - (ltrig * 0.03);
            
            if (GC_Shadow_Precision_Scale < 0.5)  { GC_Shadow_Precision_Scale = 0.5;  }
            if (GC_Shadow_Precision_Scale > 10.0) { GC_Shadow_Precision_Scale = 10.0; }

            PlayScrollSFX();

            break;
        case 4:
            uint skinHash = getPlayerSkinHash();

            if ( (skinHash == HT_AnimSkin_Spyro) || (skinHash == HT_AnimSkin_Flame) || (skinHash == HT_AnimSkin_Amber)) {
                uint newHash = 0;
                if (isButtonPressed(Button_L, g_PadNum)) {
                    newHash = skinHash - 1;
                    if (newHash < HT_AnimSkin_Spyro) { newHash = HT_AnimSkin_Amber; }
                    PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
                }
                if (isButtonPressed(Button_R, g_PadNum)) {
                    newHash = skinHash + 1;
                    if (newHash > HT_AnimSkin_Amber) { newHash = HT_AnimSkin_Spyro; }
                    PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
                }

                if (newHash != 0) {
                    ItemHandler_ChangeAnimSkin(gpPlayer, *(gpPlayerItem + (0x144/4)), newHash);
                }
            }

            break;
        case 5:
            if (isButtonPressed(Button_L | Button_R, g_PadNum)) {
                doLightingUpdate = !doLightingUpdate;
                updateLightingVector();
                PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
            }

            break;
        default:
            break;
    }
}

void prevItem() {
    if (selectedItem->next == NULL) {
        selectedItem = ItemEnv_ItemList->head;
    } else {
        selectedItem = selectedItem->next;
    }
}

void nextItem() {
    if (selectedItem->prev == NULL) {
        selectedItem = ItemEnv_ItemList->tail;
    } else {
        selectedItem = selectedItem->prev;
    }
}

void doItemControls() {
    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        positionOption++;
        if (positionOption > 3) { positionOption = 0; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        positionOption--;
        if (positionOption < 0) { positionOption = 3; }
        PlaySFX(HT_Sound_SFX_GEN_HUD_CHANGE_BREATH);
    }

    if (selectedItem == NULL) {
        if (gpPlayerItem != NULL) {
            selectedItem = gpPlayerItem;
        } else if (ItemEnv_ItemList->head != NULL) {
            selectedItem = ItemEnv_ItemList->head;
        } else {
            selectedItem = NULL;
            return;
        }
    }

    switch (positionOption) {
    case 0:
        bool newItemSelected = false;

        if (isButtonPressed(Button_R, g_PadNum)) {
            EXDListItem* startingItem = selectedItem;

            nextItem();

            //Loop through items until we find one with an animator, or we get back to the start
            while(startingItem != selectedItem) {
                if ( *((int*) (((char*)selectedItem) + 0x144)) != NULL ) {
                    break;
                }
                nextItem();
            }

            PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);

            newItemSelected = true;
        }
        if (isButtonPressed(Button_L, g_PadNum)) {
            EXDListItem* startingItem = selectedItem;

            prevItem();

            //Loop through items until we find one with an animator, or we get back to the start
            while(startingItem != selectedItem) {
                if ( *((int*) (((char*)selectedItem) + 0x144)) != NULL ) {
                    break;
                }
                prevItem();
            }

            PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);

            newItemSelected = true;
        }

        if (newItemSelected) {
            mat44 *mat = getAnimatorMatrix(getItemAnimator((int*)selectedItem));

            EXVector animPos = {0};
            mat_44_get_position(mat, &animPos);
            EXVector animRot = {0};
            EXMatrix_GetRotation(mat, &animRot, 4);

            EXVector_Copy(&selectedItemPos, &animPos);
            EXVector_Copy(&selectedItemRot, &animRot);

            if (doCameraFollow) {
                warpCamera(&animPos);
            }
        }

        break;
    case 1:
        if (isButtonPressed(Button_R | Button_L, g_PadNum)) {
            doCameraFollow = !doCameraFollow;
            PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
        }
    
        break;
    case 2:
        doPositionControls(MOVEMODE_POSITION);
    
        break;
    case 3:
        doPositionControls(MOVEMODE_ROTATION);
    
        break;
    default:
        break;
    }
}