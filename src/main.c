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
void XItem_DoItemUpdate_Hook(int* self);

#define NUM_MODES 5

enum
{
    MOVE,
    TINT,
    BRIGHT,
    FOG,
    MISC
};

in_game int PlayAnimators_NoPause; //0x803E3CD8
in_game int PlayAnimators_Pause; //0x803E3CDC

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

//Stuff to do on startup
void init() {
    //VTABLE HOOKS
    //Get function pointer to our hook
    int (*GUI_PanelItem_ptr)(int*, int*) = GUI_PanelItem_Hook;
    int (*GUI_ScreenItem_ptr)(int*, int*) = GUI_ScreenItem_Hook;
    void (*XItem_DoItemUpdate_ptr)(int*) = XItem_DoItemUpdate_Hook;
    //Store that hook pointer at the virtual function we're replacing (v_Draw)
    *(int*)0x80445D44 = GUI_PanelItem_ptr;
    *(int*)0x8042e30c = GUI_ScreenItem_ptr;
    *(int*)0x8044723c = XItem_DoItemUpdate_ptr;
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
}

uint getPlayerSkinHash() {
    int* animator = *(gpPlayerItem + (0x144/4));
    if (animator == 0) { return 0; }
    int* skinAnim = *(animator + (0x110/4));
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

    GC_Contrast = 1.11000001;
    Display_BloomIntensity = 0x40;

    GC_Fog_Near_Scale = 1.0;
    GC_Fog_Far_Scale = 1.0;

    engineFrameRate = 60;
    GC_Shadow_Precision_Scale = 2.0;
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

//main_hook.s | Runs every frame
void MainUpdate() {
    if (!hasInit) {
        init();
        hasInit = true;
    }

    //Check pause
    updatePauseState();

    //Toggle photomode on and off
    bool toggle;

    if (inPhotoMode) { //If in photomode already, just check for the Z button
        toggle = isButtonPressed(Button_Z, g_PadNum);
    } else { //Else require a double-press, or dpad-down at the same time
        if (checkZDoublePress()) {
            toggle = true;
        } else if(isButtonPressed(Button_Z, g_PadNum) && isButtonDown(Button_Dpad_Down, g_PadNum)) {
            toggle = true;
        } else {
            toggle = false;
        }
    }

    if (toggle) {
        if (inPhotoMode) {
            inPhotoMode = false;
            gameHudIsOn = true;
            GameSetPauseOff(&gGameLoop, 0);
            restoreCamHandlerVecs();
            updatePauseState();
            gCommonCamera.VFov = 1.05;
        } else if (!inPhotoMode && !gamePaused) { //only let you enable if game is unpaused
            inPhotoMode = true;
            gameHudIsOn = false;
            GameSetPauseOn(&gGameLoop, 0);
            saveCamHandlerVecs();
            updatePauseState();
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
        }
        if (isButtonPressed(Button_Dpad_Right, g_PadNum)) {
            mode++;
            if (mode >= NUM_MODES) { mode = 0; }
        }

        if (isButtonPressed(Button_B, g_PadNum)) {
            hudIsOn = !hudIsOn;
        }

        if (isButtonPressed(Button_Start, g_PadNum)) {
            resetValues();
        }

        switch (mode) {
            case MOVE:
                doCamControls();
                break;
            case TINT:
                doColorControls();
                break;
            case BRIGHT:
                doBrightnessControls();
                break;
            case FOG:
                doFogControls();
                break;
            case MISC:
                doMiscControls();
                break;
            default:
                break;
        }
    } else if(playerInScanMode()) {
        doScanmodeControls();
    }
    
    return;
}

//draw_hook.s | Runs every frame during the HUD draw loop
//Drawing stuff to the screen should be done here to avoid garbled textures
void DrawUpdate() {
    if (inPhotoMode && hudIsOn) {
        bool displayIncrementControls = false;

        switch (mode) {
            case MOVE:
                textPrint("<Move Camera>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);
                textSmpPrintF(20, 130, "FOV (Dpad Up/Down): %.2fx", gCommonCamera.VFov);

                textPrint("Cam Controls:\n[LStick] Move Hor.   [RStick] Look   [L/R] Move Ver.\n"
                "[Y] Faster   [X] Slower", 0, 20, 300, TopLeft, &COLOR_LIGHT_GREEN, 1.0);

                break;
            case TINT:
                textPrint("<Screen Tint>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                XRGBA* allCol = colorOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* redCol = colorOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* greCol = colorOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* bluCol = colorOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                textPrint( "All", 0, 20, 130, TopLeft, allCol, 1.0);
                textPrintF(20, 145, TopLeft, redCol, 1.0, "Red:   %.3f", Display_TintRed);
                textPrintF(20, 160, TopLeft, greCol, 1.0, "Green: %.3f", Display_TintGreen);
                textPrintF(20, 175, TopLeft, bluCol, 1.0, "Blue:  %.3f", Display_TintBlue);

                break;
            case BRIGHT:
                textPrint("<Brightness>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                XRGBA* contrastCol = brightOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* bloomCol    = brightOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                textPrintF(20, 130, TopLeft, contrastCol, 1.0, "Contrast: %.3fx", GC_Contrast);
                textPrintF(20, 145, TopLeft, bloomCol, 1.0, "Bloom Strength:  %d", Display_BloomIntensity);

                if (brightOption == 1) {
                    displayIncrementControls = true;
                }

                if (brightOption == 0) {
                    textPrint("! Negative/high values cause strange results",
                    0, 20, 180, TopLeft, &COLOR_LIGHT_RED, 1.0);
                }

                break;
            case FOG:
                textPrint("<Fog>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                XRGBA* nearCol = fogOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* farCol  = fogOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

                textPrintF(20, 130, TopLeft, nearCol, 1.0, "Near: %.3f", GC_Fog_Near_Scale);
                textPrintF(20, 145, TopLeft, farCol, 1.0, "Far:  %.3f", GC_Fog_Far_Scale);

                break;
            case MISC:
                textPrint("<Misc.>", 0, 20, 100, TopLeft, &COLOR_TEXT, 1.2);

                XRGBA* speedCol  = miscOption == 0 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* wideCol   = miscOption == 1 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* frameCol  = miscOption == 2 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* shadowCol = miscOption == 3 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* skinCol   = miscOption == 4 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;
                XRGBA* lightCol  = miscOption == 5 ? &COLOR_LIGHT_BLUE : &COLOR_TEXT;

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

                textPrintF(20, 130, TopLeft, speedCol, 1.0, "Game Speed: %.2f%%", getSpeedScale()*100.0);
                textPrint("Toggle Widescreen (X/Y)", 0, 20, 145, TopLeft, wideCol, 1.0);
                textPrint("Advance Player 1 Frame (X/Y)", 0, 20, 160, TopLeft, frameCol, 1.0);
                textPrintF(20, 175, TopLeft, shadowCol, 1.0, "Shadow Precision: %.2fx", GC_Shadow_Precision_Scale);
                textPrintF(20, 190, TopLeft, skinCol, 1.0, "Spyro Model: %s", skinText);
                textPrintF(20, 205, TopLeft, lightCol, 1.0, "Update Lighting: %s", doLightingUpdate ? "Yes" : "No");

                if (miscOption == 0) {
                    displayIncrementControls = true;
                }

                if (miscOption == 3) {
                    textPrint("! Higher values cause shadows to fade out quicker",
                    0, 20, 230, TopLeft, &COLOR_LIGHT_RED, 1.0);
                }

                break;
            default:
                break;
        }

        if (displayIncrementControls) {
            textPrint("[X/Y] Incremental", 0, 20, 280, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
        }

        textPrint("General Controls:\n[A] Reset Param   [Dpad] Browse   [L/R] Change Value\n"
                  "[Start] Reset All   [B] Toggle HUD   [Z] Exit", 0, 20, 380, TopLeft, &COLOR_LIGHT_GREEN, 1.0);
    }

    if (!inPhotoMode && playerInScanMode()) {
        if (gpPlayer == NULL) { return; }
        uint animMode = getCurrentAnimMode();

        textSmpPrintF(20, 130, "Current AnimMode: %x", animMode);
        textSmpPrint("Dpad left/right: Change", 0, 20, 150);
        textSmpPrint("Dpad down: Restart Anim", 0, 20, 170);
        textSmpPrintF(20, 190, "R: Turn Headtracking %s", headTrackEnabled() ? "Off" : "On");
    }

    return;
}

//scan_hook.s | On the frame this function returns true, scanmode will be enabled/disabled.
//Typically you'd return true if a button has just been pressed.
bool ScanUpdate() {
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

//Hooks into the general item update function
void XItem_DoItemUpdate_Hook(int* self)
{
    //Check if we're updating the player item and if we should advance this frame
    if ((self == gpPlayerItem) && doFrameAdvance) {
        doFrameAdvance = false;
        PlayAnimators_Pause = 1; //Make animators play while paused
        g_PadNum = 3; //Hack so controls don't interfere

        //Store player's current position
        EXVector* pos = (EXVector*) (self + (0xD0/4));
        EXVector store = {pos->x, pos->y, pos->z, pos->w};

        XItem_DoItemUpdate(self);

        //Restore position
        pos->x = store.x;
        pos->y = store.y;
        pos->z = store.z;
        pos->w = store.w;

        //Restore these variables
        PlayAnimators_Pause = 0;
        g_PadNum = 0;
    } else {
        XItem_DoItemUpdate(self);
    }
}