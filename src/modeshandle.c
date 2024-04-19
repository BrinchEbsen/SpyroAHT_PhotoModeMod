#include <common.h>
#include <symbols.h>
#include <rotation.h>
#include <hashcodes.h>

//Positions of menu selection
int colorOption = 0;
int fogOption = 0;
int brightOption = 0;
int miscOption = 0;

//Whether to advance the player's animation this frame
bool doFrameAdvance = false;

//Whether the camera handler's vectors are updated to make the lighting update
bool doLightingUpdate = true;

int* customCam = NULL;

EXVector savedCamHandlerPos  = {0};
EXVector savedCamHandlerLook = {0};

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

int* getPlayerAnimator() {
    if (gpPlayerItem == NULL) { return 0; }

    return *(gpPlayerItem + (0x144/4));
}

uint getCurrentAnimMode() {
    if (gpPlayerItem == NULL) { return 0; }
    return *((uint*) (gpPlayerItem + (0x180/4)));
}

bool playerInScanMode() {
    if (gpPlayer == NULL) { return false; }

    return *((bool*) (gpPlayer + (0x588/4)));
}

void doScanmodeControls() {
    if (gpPlayer == NULL) { return; }
    uint currmode = getCurrentAnimMode();
    uint newMode = currmode;

    if (isButtonPressed(Button_Dpad_Right, g_PadNum)) {
        //ig_printf("before: %x\n", currmode);
        
        while (true) {
            newMode++;
            if (newMode >= HT_AnimMode_HASHCODE_END) {
                newMode = HT_AnimMode_HASHCODE_BASE+1;
            }

            if (Player_ForceModeChange(gpPlayer, newMode)) {
                break;
            }
        }
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
    }

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        Player_ForceModeChange(gpPlayer, currmode);
    }
    if (isButtonPressed(Button_R, g_PadNum)) {
        setHeadTracking(!headTrackEnabled());
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

    float stickY = Pads_Analog->LStick_Y;
    float stickX = Pads_Analog->LStick_X;
    float trigL = Pads_Analog->LTrigger;
    float trigR = Pads_Analog->RTrigger;

    //Camera point vector
    float camPointX = CamMatrix.row0.x;
    float camPointZ = CamMatrix.row0.z;

    //Move the camera horizontally with the left stick, vertically with the triggers
    float moveX = stickX * camPointX * moveSensitivity + stickY * camPointZ    * moveSensitivity;
    float moveZ = stickX * camPointZ * moveSensitivity + stickY * (-camPointX) * moveSensitivity;
    float moveY = (trigR) * moveSensitivity*2 + (-trigL) * moveSensitivity*2;

    //FOV controls
    if (isButtonDown(Button_Dpad_Down, g_PadNum)) {
        gCommonCamera.VFov += 0.01;
    }
    if (isButtonDown(Button_Dpad_Up, g_PadNum)) {
        gCommonCamera.VFov -= 0.01;
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
    float angHor = Pads_Analog->RStick_X * lookSensitivity;
    float angVer = Pads_Analog->RStick_Y * lookSensitivity;

    //mat44 rot = EulerToMatrix(-angVer, -angHor, 0);

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
    gCommonCamera.Distance = gCommonCamera.Rect.h * 0.5 * (1.0 / ig_tanf(gCommonCamera.VFov * 0.5));

    //Set camera handler's vectors to match, so the lighting adapts
    updateLightingVector();
}

void doColorControls() {
    float speed = 0.02;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        colorOption++;
        if (colorOption > 3) { colorOption = 0; }
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        colorOption--;
        if (colorOption < 0) { colorOption = 3; }
    }

    switch(colorOption) {
        case 0:
            Display_TintRed   += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
            Display_TintGreen += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
            Display_TintBlue  += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintRed   = 1.0;
                Display_TintGreen = 1.0;
                Display_TintBlue  = 1.0;
            }

            break;
        case 1:
            Display_TintRed += (Pads_Analog->RTrigger * speed) -  (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintRed = 1.0;
            }
            break;
        case 2:
            Display_TintGreen += (Pads_Analog->RTrigger * speed) -  (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintGreen = 1.0;
            }
            break;
        case 3:
            Display_TintBlue += (Pads_Analog->RTrigger * speed) -  (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                Display_TintBlue = 1.0;
            }
            break;
        default:
            break;
    }
}

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
            GC_Contrast += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
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
        if (fogOption > 1) { fogOption = 0; }
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        fogOption--;
        if (fogOption < 0) { fogOption = 1; }
    }

    switch(fogOption) {
        case 0:
            GC_Fog_Near_Scale += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Fog_Near_Scale = 1.0;
            }
            break;
        case 1:
            GC_Fog_Far_Scale += (Pads_Analog->RTrigger * speed) - (Pads_Analog->LTrigger * speed);
            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Fog_Far_Scale = 1.0;
            }
            break;
        default:
            break;
    }
}

void doMiscControls() {
    float speed = 0.01;

    if (isButtonPressed(Button_Dpad_Down, g_PadNum)) {
        miscOption++;
        if (miscOption > 5) { miscOption = 0; }
    }
    if (isButtonPressed(Button_Dpad_Up, g_PadNum)) {
        miscOption--;
        if (miscOption < 0) { miscOption = 5; }
    }

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

            if (isButtonPressed(Button_B, g_PadNum)) {
                engineFrameRate = 60;
            }
            
            break;
        case 1:
            if (isButtonPressed(Button_X, g_PadNum) || isButtonPressed(Button_Y, g_PadNum)) {
                toggleWideScreen();
            }

            break;
        case 2:
            if (isButtonPressed(Button_X, g_PadNum) || isButtonPressed(Button_Y, g_PadNum)) {
                doFrameAdvance = true;
            }

            break;
        case 3:
            GC_Shadow_Precision_Scale += (Pads_Analog->RTrigger * 0.03) - (Pads_Analog->LTrigger * 0.03);
            
            if (GC_Shadow_Precision_Scale < 0.5)  { GC_Shadow_Precision_Scale = 0.5;  }
            if (GC_Shadow_Precision_Scale > 10.0) { GC_Shadow_Precision_Scale = 10.0; }

            if (isButtonPressed(Button_B, g_PadNum)) {
                GC_Shadow_Precision_Scale = 2.0;
            }

            break;
        case 4:
            uint skinHash = getPlayerSkinHash();

            if (skinHash == HT_AnimSkin_Spyro || skinHash == HT_AnimSkin_Flame || skinHash == HT_AnimSkin_Amber) {
                uint newHash = 0;
                if (isButtonPressed(Button_L, g_PadNum)) {
                    newHash = skinHash - 1;
                    if (newHash < HT_AnimSkin_Spyro) { newHash = HT_AnimSkin_Amber; }
                }
                if (isButtonPressed(Button_R, g_PadNum)) {
                    newHash = skinHash + 1;
                    if (newHash > HT_AnimSkin_Amber) { newHash = HT_AnimSkin_Spyro; }
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
            }

            break;
        default:
            break;
    }
}