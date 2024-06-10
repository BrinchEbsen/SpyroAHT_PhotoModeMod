#include <common.h>
#include <hashcodes.h>
#include <Sound.h>
#include <textprint.h>
#include <buttons.h>
#include <playerstate.h>
#include <inputdisplay.h>
#include <symbols.h>
#include <screenmath.h>
#include <rotation.h>
#include <modeshandle.h>

//Whether our mod has initialized
bool hasInit = false;

//We have to declare the functions replaced in the vtable hooks
int GUI_PanelItem_Hook(int* panelItem, int* wnd);
int GUI_ScreenItem_Hook(int* screenItem, int* wnd);
//void XItem_DoItemUpdate_Hook(int* self);

#define NUM_MODES 5

enum
{
    MOVE,
    TINT,
    FOG,
    MISC,
    ITEM
};

//Game logic has paused (when pressing the start button etc.)
bool gamePaused;
//Photo mode is enabled
bool inPhotoMode = false;
//Photo mode HUD is enabled
bool hudIsOn = true;
//Game's own HUD elements are enabled
bool gameHudIsOn = true;
//Current selected mode in the photo mode
int mode = MOVE;

int toggleInfoTextTimer = 0;

DrawRender storedFogInfo = {0};

//Stuff to do on startup
void init() {
    //VTABLE HOOKS
    //Get function pointer to our hook
    int (*GUI_PanelItem_ptr)(int*, int*) = GUI_PanelItem_Hook;
    int (*GUI_ScreenItem_ptr)(int*, int*) = GUI_ScreenItem_Hook;
    //void (*XItem_DoItemUpdate_ptr)(int*) = XItem_DoItemUpdate_Hook;
    //Store that hook pointer at the virtual function we're replacing (v_Draw)
    *(int*)0x80445D44 = GUI_PanelItem_ptr;
    *(int*)0x8042e30c = GUI_ScreenItem_ptr;
    //*(int*)0x8044723c = XItem_DoItemUpdate_ptr;
}

void saveCamHandlerVecs() {
    int* camHandler = *(gpGameWnd + 0x378/4);
    EXVector* camPos  = (EXVector*) (camHandler + 0x298/4);
    EXVector* camLook = (EXVector*) (camHandler + 0x2A8/4);

    savedCamHandlerPos.x  = camPos->x;
    savedCamHandlerPos.y  = camPos->y;
    savedCamHandlerPos.z  = camPos->z;
    savedCamHandlerPos.w  = camPos->w;
    savedCamHandlerLook.x = camLook->x;
    savedCamHandlerLook.y = camLook->y;
    savedCamHandlerLook.z = camLook->z;
    savedCamHandlerLook.w = camLook->w;
    savedFOV = gCommonCamera.VFov;
}

void saveFogInfo() {
    DrawRender* fogInfo = getZoneFogInfo();
    if (fogInfo == NULL) { return; }

    storedFogInfo.FogNear    = fogInfo->FogNear;
    storedFogInfo.FogFar     = fogInfo->FogFar;
    storedFogInfo.FogMin     = fogInfo->FogMin;
    storedFogInfo.FogMax     = fogInfo->FogMax;
    storedFogInfo.FogColor.r = fogInfo->FogColor.r;
    storedFogInfo.FogColor.g = fogInfo->FogColor.g;
    storedFogInfo.FogColor.b = fogInfo->FogColor.b;
    storedFogInfo.FogColor.a = fogInfo->FogColor.a;
    storedFogInfo.FogEnabled = fogInfo->FogEnabled;
}

void restoreFogInfo() {
    DrawRender* fogInfo = getZoneFogInfo();
    if (fogInfo == NULL) { return; }

    fogInfo->FogNear    = storedFogInfo.FogNear;
    fogInfo->FogFar     = storedFogInfo.FogFar;
    fogInfo->FogMin     = storedFogInfo.FogMin;
    fogInfo->FogMax     = storedFogInfo.FogMax;
    fogInfo->FogColor.r = storedFogInfo.FogColor.r;
    fogInfo->FogColor.g = storedFogInfo.FogColor.g;
    fogInfo->FogColor.b = storedFogInfo.FogColor.b;
    fogInfo->FogColor.a = storedFogInfo.FogColor.a;
    fogInfo->FogEnabled = storedFogInfo.FogEnabled;
}

void savePlayerTransform() {
    EXVector* pos = getItemPosition(gpPlayerItem);
    if (pos == NULL) { return; }
    EXVector* rot = getItemRotation(gpPlayerItem);
    if (rot == NULL) { return; }

    savedPlayerPos.x = pos->x;
    savedPlayerPos.y = pos->y;
    savedPlayerPos.z = pos->z;
    savedPlayerPos.w = pos->w;
    selectedItemPos.x = pos->x;
    selectedItemPos.y = pos->y;
    selectedItemPos.z = pos->z;
    selectedItemPos.w = pos->w;

    savedPlayerRot.x = rot->x;
    savedPlayerRot.y = rot->y;
    savedPlayerRot.z = rot->z;
    savedPlayerRot.w = rot->w;
    selectedItemRot.x = rot->x;
    selectedItemRot.y = rot->y;
    selectedItemRot.z = rot->z;
    selectedItemRot.w = rot->w;
}

void restoreCamHandlerVecs() {
    int* camHandler = *(gpGameWnd + 0x378/4);
    EXVector* camPos  = (EXVector*) (camHandler + 0x298/4);
    EXVector* camLook = (EXVector*) (camHandler + 0x2A8/4);

    camPos->x  = savedCamHandlerPos.x;
    camPos->y  = savedCamHandlerPos.y;
    camPos->z  = savedCamHandlerPos.z;
    camPos->w  = savedCamHandlerPos.w;
    camLook->x = savedCamHandlerLook.x;
    camLook->y = savedCamHandlerLook.y;
    camLook->z = savedCamHandlerLook.z;
    camLook->w = savedCamHandlerLook.w;
    gCommonCamera.VFov = savedFOV;
}

uint getPlayerSkinHash() {
    if (gpPlayerItem == NULL) { return NULL; }
    int* anim = getItemAnimator(gpPlayerItem);
    int* skinAnim = *(anim + (0x110/4));
    if (skinAnim == 0) { return 0; }

    return *(skinAnim + (0xD0/4));
}

void toggleWideScreen() {
    static bool enabled = false;
    
    enabled = !enabled;

    if (enabled) {
        worldCullWidth = 0.5;
        entityCullWidth = 0.75;
        aspectRatio = (16.0/9.0);
    } else {
        worldCullWidth = 0.75;
        entityCullWidth = 0.5;
        aspectRatio = (4.0/3.0);
    }
}

void resetValues() {
    Display_TintRed = 1.0;
    Display_TintGreen = 1.0;
    Display_TintBlue = 1.0;

    //GC_Contrast = 1.11000001;
    Display_BloomIntensity = 0x40;

    GC_Fog_Near_Scale = 1.0;
    GC_Fog_Far_Scale = 1.0;

    engineFrameRate = 60;
    GC_Shadow_Precision_Scale = 2.0;

    restoreFogInfo();
    resetCamPosition();
}

bool checkZDoublePress() {
    static int numPressed = 0;
    static int timer = 0;

    /*
    //If pressing any other button, fail check
    if (isButtonDown(
        
        Button_A | Button_B | Button_L | Button_R | Button_X | Button_Y | Button_Dpad_Down |
        Button_Dpad_Left | Button_Dpad_Right | Button_Dpad_Up | Button_Dpad_Down | Button_Start

        , g_PadNum)) {
        numPressed = 0;
        timer = 0;
        
        return false;
    }
    */

    bool zPressed = isButtonPressed(Button_Z, g_PadNum);

    if (numPressed == 0 && !zPressed) {
        //Have not pressed and am not pressing
        timer = 0;

        return false;
    }

    //Have either pressed it before or am pressing now
    timer++;

    //If timer is within the limit
    if (timer <= 20) {
        if (zPressed) { numPressed++; }

        if (numPressed >= 2) {
            //Have successfully double-pressed
            timer = 0;
            numPressed = 0;

            return true;
        }

        return false;
    } else {
        //Timer expired
        timer = 0;
        numPressed = 0;
    
        return false;
    }

    //Just for safety
    return false;
}

void updatePauseState() {
    uint flags = *((&gGameLoop) + (0x88/4));
    gamePaused = (flags & 0x80000000) == 0x80000000; //Get the current state of the gameloop (pause flag)
}

float getSpeedScale() {
    return (60.0 / (float)engineFrameRate);
}

bool selectedItemExists() {
    EXDListItem* item = ItemEnv_ItemList->head;

    if (item == NULL) { return false; }

    do {
        if (item == selectedItem) {
            return true;
        }
        
        item = item->next;
    } while(item != NULL);

    return false;
}

void drawItemMarker(EXVector* pos) {
    static int counter = 0;
    counter++;
    counter = counter % 30;

    int size;
    if (counter < 15) {
        size = 6;
    } else {
        size = 8;
    }

    drawSquareAtVec(pos, size*2, &COLOR_DARK_RED);
    drawSquareAtVec(pos, size, &COLOR_RED);
}

//main_hook.s | Runs every frame
void MainUpdate() {
    if (!hasInit) {
        //init();
        hasInit = true;
    }

    //Check pause
    updatePauseState();

    if (toggleInfoTextTimer > 0) {
        toggleInfoTextTimer--;
    }

    if (isButtonPressed(Button_Z, g_PadNum)) {
        toggleInfoTextTimer = 120;
    }

    //Toggle photomode on and off
    bool toggle;

    //Require a double-press, or dpad-down at the same time
    if (checkZDoublePress()) {
        toggle = true;
    } else if(isButtonPressed(Button_Z, g_PadNum) && isButtonDown(Button_Dpad_Down, g_PadNum)) {
        toggle = true;
    } else {
        toggle = false;
    }

    if (toggle) {
        toggleInfoTextTimer = 0;

        if (inPhotoMode) {
            inPhotoMode = false;
            gameHudIsOn = true;
            GameSetPauseOff(&gGameLoop, 0);
            restoreCamHandlerVecs();
            selectedItem = NULL;
            updatePauseState();
            gCommonCamera.VFov = savedFOV;
            updateCameraViewport();

            PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_SELECT);
        } else if (!inPhotoMode && !gamePaused) { //only let you enable if game is unpaused
            inPhotoMode = true;
            gameHudIsOn = false;
            GameSetPauseOn(&gGameLoop, 0);
            saveCamHandlerVecs();
            updatePauseState();
            savePlayerTransform();
            saveFogInfo();

            PlaySFX(HT_Sound_SFX_GEN_HUD_PAUSE);
        }
    }

    //In case of desync
    if (!gamePaused && inPhotoMode) {
        inPhotoMode = false;
        resetValues();
    }

    //Handle modes
    if (inPhotoMode) {
        if (isButtonPressed(Button_Dpad_Left, g_PadNum)) {
            mode--;
            if (mode < 0) { mode = NUM_MODES-1; }

            PlaySFX(HT_Sound_SFX_GEN_HUD_CURSOR);
        }
        if (isButtonPressed(Button_Dpad_Right, g_PadNum)) {
            mode++;
            if (mode >= NUM_MODES) { mode = 0; }

            PlaySFX(HT_Sound_SFX_GEN_HUD_CURSOR);
        }

        if(isButtonPressed(Button_Dpad_Right | Button_Dpad_Left, g_PadNum)) {
            //If the selected item doesn't exist, we should reset it.
            if (!selectedItemExists()) {
                selectedItem = NULL;
            }
        }

        if (isButtonPressed(Button_A, g_PadNum)) {
            hudIsOn = !hudIsOn;

            PlaySFX(HT_Sound_SFX_GEN_HUD_NPC_CHOOSE);
        }

        if (isButtonPressed(Button_Start, g_PadNum)) {
            resetValues();
            PlaySFX(HT_Sound_SFX_GEN_HUD_SELECT);
        }

        switch (mode) {
            case MOVE:
                doCamControls();
                break;
            case TINT:
                doColorControls();
                break;
            case FOG:
                doFogControls();
                break;
            case MISC:
                doMiscControls();
                break;
            case ITEM:
                doItemControls();
                break;
            default:
                break;
        }
    } else if(playerInScanMode()) {
        if (!gamePaused) {
            doScanmodeControls();
        }
    }
    
    return;
}

//draw_hook.s | Runs every frame during the HUD draw loop
//Drawing stuff to the screen should be done here to avoid garbled textures
void DrawUpdate() {
    if (inPhotoMode && hudIsOn) {
        bool displayIncrementControls = false;

        XRGBA* selCol1;
        XRGBA* selCol2;
        XRGBA* selCol3;
        XRGBA* selCol4;
        XRGBA* selCol5;
        XRGBA* selCol6;
        XRGBA* selCol7;
        XRGBA* selCol8;
        XRGBA* selCol9;
        XRGBA* selCol10;

        switch (mode) {
            case MOVE:
                textPrint("<Move Camera>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);
                textSmpPrintF(20, 130, "FOV (Dpad Up/Down): %.2fx", gCommonCamera.VFov);

                textPrint("Cam Controls:\n[LStick] Move Hor.   [RStick] Look   [L/R] Move Ver.\n"
                "[Y] Faster   [X] Slower", 0, 20, 300, TopLeft, &COLOR_LIGHT_GREEN, 1.0);

                break;
            case TINT:
                textPrint("<Screen Tint>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                selCol1 = colorOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol2 = colorOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol3 = colorOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol4 = colorOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol5 = colorOption == 4 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                textPrint( "Color", 0, 20, 130, TopLeft, selCol1, 1.0);
                textPrintF(20, 145, TopLeft, selCol2, 1.0, "  R: %.2f", Display_TintRed);
                textPrintF(20, 160, TopLeft, selCol3, 1.0, "  G: %.2f", Display_TintGreen);
                textPrintF(20, 175, TopLeft, selCol4, 1.0, "  B: %.2f", Display_TintBlue);
                textPrintF(20, 190, TopLeft, selCol5, 1.0, "Bloom Strength: %d", Display_BloomIntensity);

                if (colorOption == 4) {
                    displayIncrementControls = true;
                }

                break;
            case FOG:
                textPrint("<Fog>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                selCol1  = fogOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol2  = fogOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol3  = fogOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol4  = fogOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol5  = fogOption == 4 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol6  = fogOption == 5 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol7  = fogOption == 6 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol8  = fogOption == 7 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol9  = fogOption == 8 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol10 = fogOption == 9 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                textSmpPrint("Global:", 0, 20, 130);
                textPrintF(20, 145, TopLeft, selCol1, 1.0, "  Near Scale: %.2f", GC_Fog_Near_Scale);
                textPrintF(20, 160, TopLeft, selCol2, 1.0, "  Far Scale:  %.2f", GC_Fog_Far_Scale);

                DrawRender* fogInfo = getZoneFogInfo();
                if (fogInfo == NULL) {
                    if (fogOption > 1) { fogOption = 0; }
                    break;
                }

                textSmpPrint("Zone:", 0, 20, 175);

                textPrintF(20, 190, TopLeft, selCol3, 1.0, "  Enabled: %s", fogInfo->FogEnabled ? "Yes" : "No");

                textPrintF(20, 205, TopLeft, selCol4, 1.0, "  Near: %.2f", fogInfo->FogNear);
                textPrintF(20, 220, TopLeft, selCol5, 1.0, "  Far: %.2f", fogInfo->FogFar);
                textPrintF(20, 235, TopLeft, selCol6, 1.0, "  Min: %.2f", fogInfo->FogMin);
                textPrintF(20, 250, TopLeft, selCol7, 1.0, "  Max: %.2f", fogInfo->FogMax);

                textPrintF(20, 265, TopLeft, selCol8, 1.0, "  Color R:  %d", fogInfo->FogColor.r);
                textPrintF(20, 280, TopLeft, selCol9, 1.0, "  Color G:  %d", fogInfo->FogColor.g);
                textPrintF(20, 295, TopLeft, selCol10, 1.0, "  Color B:  %d", fogInfo->FogColor.b);

                if (fogOption > 6) {
                    displayIncrementControls = true;
                }

                if (((fogOption >= 0) && (fogOption <= 1)) || ((fogOption >= 3) && (fogOption <= 6))) {
                    textPrint("[Y] Faster    [X] Slower", 0, 20, 320, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
                }

                break;
            case MISC:
                textPrint("<Misc.>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                selCol1 = miscOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol2 = miscOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol3 = miscOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol4 = miscOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol5 = miscOption == 4 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol6 = miscOption == 5 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                char* skinText;
                uint skinHash = getPlayerSkinHash();

                switch(skinHash) {
                    case HT_AnimSkin_Spyro:
                        skinText = "Spyro";
                        break;
                    case HT_AnimSkin_Flame:
                        skinText = "Flame";
                        break;
                    case HT_AnimSkin_Amber:
                        skinText = "Ember";
                        break;
                    default:
                        skinText = "N/A";
                        break;
                }
                
                char* animTimeStr;

                int* anim = getItemAnimator(gpPlayerItem);
                if (anim == NULL) {
                    animTimeStr = "N/A";
                } else {
                    float currTime = Animator_GetObjectTime(anim);

                    char c[10];
                    ig_sprintf(c, "%.2f", currTime);
                    animTimeStr = c;
                }

                textPrintF(20, 130, TopLeft, selCol1, 1.0, "Game Speed: %.2f%%", getSpeedScale()*100.0);
                textPrint("Toggle Widescreen", 0, 20, 145, TopLeft, selCol2, 1.0);
                textPrintF(20, 160, TopLeft, selCol3, 1.0, "Player Animation Time: %s", animTimeStr);
                textPrintF(20, 175, TopLeft, selCol4, 1.0, "Shadow Precision: %.2fx", GC_Shadow_Precision_Scale);
                textPrintF(20, 190, TopLeft, selCol5, 1.0, "Spyro Model: %s", skinText);
                textPrintF(20, 205, TopLeft, selCol6, 1.0, "Update Lighting: %s", doLightingUpdate ? "Yes" : "No");

                if (miscOption == 0) {
                    displayIncrementControls = true;
                }

                if (miscOption == 3) {
                    textPrint("! Higher values cause shadows to fade out quicker",
                    0, 20, 230, TopLeft, &COLOR_LIGHT_RED, 1.0);
                }

                if (miscOption == 2) {
                    textPrint("[Y] Faster    [X] Slower", 0, 20, 320, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
                }

                break;
            case ITEM:
                textPrint("<Position Objects>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);
                
                if (selectedItem == NULL) {
                    textPrint("No moveable items loaded", 0, 20, 130, TopLeft, &COLOR_LIGHT_RED, 1.0);
                    return;
                }

                selCol1 = positionOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol2 = positionOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol3 = positionOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                selCol4 = positionOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                char* itemName = ItemHandler_GetName(*((int*) (((char*)selectedItem) + 0x14C)));

                textPrintF(20, 130, TopLeft, selCol1, 1.0, "Selected Item: %s", itemName);
                textPrintF(20, 145, TopLeft, selCol2, 1.0, "Move Camera: %s", doCameraFollow ? "Yes" : "No");
                textPrint("Position", 0, 20, 160, TopLeft, selCol3, 1.0);
                textPrint("Rotation", 0, 20, 175, TopLeft, selCol4, 1.0);

                //DEBUG:
                //int* animator = getItemAnimator(selectedItem);
                //textPrintF(20, 205, TopLeft, selCol1, 1.0, "%x", animator);
                //int* vtable = (int*) *(animator + (0x18/4));
                //textPrintF(20, 225, TopLeft, selCol1, 1.0, "%x", vtable);

                if (positionOption == 2) {
                    textPrint("Position Controls:\n[LStick] Move Hor.   [L/R] Move Ver.\n"
                "[Y] Faster   [X] Slower", 0, 20, 300, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
                }
                if (positionOption == 3) {
                    textPrint("Rotation Controls:\n[LStick] Turn/Pitch   [L/R] Roll\n"
                "[Y] Faster   [X] Slower", 0, 20, 300, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
                }

                if (selectedItem != NULL) {
                    drawItemMarker(&selectedItemPos);
                }

                break;
            default:
                break;
        }

        if (displayIncrementControls) {
            textPrint("[X/Y] Incremental", 0, 20, 320, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
        }

        textPrint("General Controls:\n[B] Reset Param   [Dpad] Browse   [L/R] Change Value\n"
                  "[Start] Reset All   [A] Toggle HUD   [Z] Exit", 0, 20, 380, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
    }

    if (!inPhotoMode && playerInScanMode() && !gamePaused) {
        if (gpPlayer == NULL) { return; }
        uint animMode = getCurrentAnimMode();

        textSmpPrintF(20, 130, "Current AnimMode: %d", animMode & 0xFFFF);
        textSmpPrint("[Dpad Left/Right]: Change", 0, 20, 150);
        textSmpPrint("[Dpad Up]: Restart Anim", 0, 20, 170);
        textSmpPrintF(20, 190, "[R]: Toggle Head Tracking: %s", headTrackEnabled() ? "On" : "Off");
        textSmpPrint("[Z+Dpad Up]: Exit", 0, 20, 210);
    }

    //Tutorial text
    if (toggleInfoTextTimer > 0) {
        if (inPhotoMode) {
            textPrint(
                "Toggle Photo Mode:\nZ+Dpad Down / Double-Press Z",
                0, 0, 170, TopRight, &COLOR_LIGHT_GREEN, 1.0
            );
        } else if(!playerInScanMode()) {
            textPrint(
                "Toggle Photo Mode:\nZ+Dpad Down / Double-Press Z\n"
                "Toggle ScanMode:\nZ+Dpad Up",
                0, 0, 170, TopRight, &COLOR_LIGHT_GREEN, 1.0
            );
        }
    }

    /*
    DrawRender* fogInfo = getZoneFogInfo();
    textSmpPrintF(20, 100, "Near: %.2f", fogInfo->FogNear);
    textSmpPrintF(20, 115, "Far: %.2f", fogInfo->FogFar);
    textSmpPrintF(20, 130, "Min: %.2f", fogInfo->FogMin);
    textSmpPrintF(20, 145, "Max: %.2f", fogInfo->FogMax);
    textSmpPrintF(20, 160, "Color: %x%x%x%x",
        fogInfo->FogColor.r,
        fogInfo->FogColor.g,
        fogInfo->FogColor.b,
        fogInfo->FogColor.a
    );
    textSmpPrintF(20, 175, "Enabled: %s", fogInfo->FogEnabled ? "True" : "False");

    if (isButtonPressed(Button_Z, g_PadNum)) {
        ig_printf("%x\n", fogInfo);
    }
    */

    return;
}

//scan_hook.s | On the frame this function returns true, scanmode will be enabled/disabled.
//Typically you'd return true if a button has just been pressed.
bool ScanUpdate() {
    //Hold d-pad up and press Z
    return isButtonPressed(Button_Z, g_PadNum) && isButtonDown(Button_Dpad_Up, g_PadNum);
}

//Hooks into GUI_PanelItem::v_Draw (text)
int GUI_PanelItem_Hook(int* panelItem, int* wnd) {
    if (inPhotoMode && !gameHudIsOn) {
        return 0;
    }
    return GUI_PanelItem_Draw(panelItem, wnd);
}

//Hooks into GUI_ScreenItem::v_Draw (icons)
int GUI_ScreenItem_Hook(int* screenItem, int* wnd) {
    if (inPhotoMode && !gameHudIsOn) {
        return 0;
    }
    return GUI_ScreenItem_Draw(screenItem, wnd);
}

int GUI_Screen_Hook(int* screen, int* wnd) {
    if (inPhotoMode && !gameHudIsOn) {
        return 0;
    }
    return GUI_Screen_Draw(screen, wnd);
}

void Animator_Anim_Bounds_Hook(int* animator, int* InBounds, int* Bounds, uint Flags) {
    if (inPhotoMode) {
        return;
    }
    Animator_ApplyMatrixBoundsBox(animator, InBounds, Bounds, Flags);
}