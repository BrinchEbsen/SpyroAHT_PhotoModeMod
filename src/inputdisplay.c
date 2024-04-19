#include <common.h>
#include <buttons.h>
#include <inputdisplay.h>
#include <symbols.h>

void drawInputButton(EXRect* r, Buttons b, int padNum) {
    XRGBA onCol  = {0x80, 0x80, 0x80, 0x70};
    XRGBA offCol = {0x40, 0x40, 0x40, 0x40};

    Util_DrawRect(gpPanelWnd, r, &offCol);
    if (isButtonDown(b, padNum)) {
        Util_DrawRect(gpPanelWnd, r, &onCol);
    }
}

void drawInputTrigger(EXRect* r, float amount) {
    XRGBA onCol  = {0x80, 0x80, 0x80, 0x70};
    XRGBA offCol = {0x40, 0x40, 0x40, 0x40};

    Util_DrawRect(gpPanelWnd, r, &offCol);

    if (amount > 0.0f) {
        int len = r->h * amount;
        EXRect onR = {r->x, r->y + (r->h - len), r->w, len};

        Util_DrawRect(gpPanelWnd, &onR, &onCol);
    }
}

void drawInputStick(EXRect* r, float x, float y) {
    XRGBA stckCol = {0x80, 0x80, 0x80, 0x70};
    XRGBA backCol = {0x40, 0x40, 0x40, 0x40};
    
    int cX = r->x + (r->w/2);
    int cY = r->y + (r->h/2);

    int stckX = cX + ((r->w/2) * x);
    int stckY = cY + ((r->h/2) * y);

    EXRect stckR = {stckX-1, stckY-1, 2, 2};

    Util_DrawRect(gpPanelWnd, r, &backCol);
    Util_DrawRect(gpPanelWnd, &stckR, &stckCol);
}

void drawInputVis(u16 x, u16 y, int padNum) {
    EXRect bg = {x-50, y-60, 100, 120};
    Util_DrawRect(gpPanelWnd, &bg, &COLOR_BLACK);

    EXRect startButton = {x-5, y-5, 10, 10};

    u16 faceX = x+20;
    u16 faceY = y-5;
    u16 faceSize = 10;
    u16 faceSpc = 10;
    EXRect aButton = {faceX, faceY+faceSpc, faceSize, faceSize};
    EXRect bButton = {faceX, faceY-faceSpc, faceSize, faceSize};
    EXRect xButton = {faceX+faceSpc, faceY, faceSize, faceSize};
    EXRect yButton = {faceX-faceSpc, faceY, faceSize, faceSize};

    u16 dirX = x-30;
    u16 dirY = y-5;
    u16 dirSize = 10;
    u16 dirSpc = 10;
    EXRect dirUp    = {dirX, dirY-dirSpc, dirSize, dirSize};
    EXRect dirDown  = {dirX, dirY+dirSpc, dirSize, dirSize};
    EXRect dirLeft  = {dirX-dirSpc, dirY, dirSize, dirSize};
    EXRect dirRight = {dirX+dirSpc, dirY, dirSize, dirSize};

    EXRect zButton = {x+20, y-25, 10, 5};

    EXRect lTrigger = {x-30, y-50, 10, 20};
    EXRect rTrigger = {x+20, y-50, 10, 20};

    EXRect lStick = {x-40, y+20, 30, 30};
    EXRect rStick = {x+10, y+20, 30, 30};

    //0.431

    drawInputButton(&startButton, Button_Start, padNum);

    drawInputButton(&aButton, Button_A, padNum);
    drawInputButton(&bButton, Button_B, padNum);
    drawInputButton(&xButton, Button_X, padNum);
    drawInputButton(&yButton, Button_Y, padNum);

    drawInputButton(&dirUp,    Button_Dpad_Up,    padNum);
    drawInputButton(&dirDown,  Button_Dpad_Down,  padNum);
    drawInputButton(&dirLeft,  Button_Dpad_Left,  padNum);
    drawInputButton(&dirRight, Button_Dpad_Right, padNum);

    drawInputButton(&zButton, Button_Z, padNum);

    drawInputTrigger(&lTrigger, Pads_Analog[padNum].LTrigger / 0.431f);
    drawInputTrigger(&rTrigger, Pads_Analog[padNum].RTrigger / 0.431f);

    drawInputStick(&lStick, Pads_Analog[padNum].LStick_X, Pads_Analog[padNum].LStick_Y);
    drawInputStick(&rStick, Pads_Analog[padNum].RStick_X, Pads_Analog[padNum].RStick_Y);
}