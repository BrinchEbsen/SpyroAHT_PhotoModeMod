INPUT(../../../.config/symbols/symbols.txt)
INPUT(../../../.config/symbols/function_symbols.txt)
INPUT(../../../.config/symbols/auto_symbols.txt)

MEMORY
{
    /* RAM locations where we'll inject the code for our replacement functions */
    SharkCodeCave : ORIGIN = 0x801858c0, LENGTH = 0x1D60
    ViewerCodeCave : ORIGIN = 0x80296618, LENGTH = 0x7dcc
    MainUpdateHook : ORIGIN = 0x8026fa6c, LENGTH = 0x100000
    HUDDrawHook : ORIGIN = 0x80222244, LENGTH = 0x100000
    ScanModeHook : ORIGIN = 0x8005c954, LENGTH = 0x100000
    GUIScreenHook : ORIGIN = 0x80222234, LENGTH = 0x100000
    AnimatorAnimBoundsHook : ORIGIN = 0x802a587c, LENGTH = 0x100000
}

SECTIONS
{
    /* Custom section for compiled code */
    /* Custom section for our hook code */
    .MainUpdateHook : 
    {
        main_hook.o(.text)
        main_hook.o(.rodata)
        main_hook.o(.rodata*)
        main_hook.o(.data)
        main_hook.o(.bss)
    } > MainUpdateHook

    /* Custom section for our hook code */
    .HUDDrawHook : 
    {
        draw_hook.o(.text)
        draw_hook.o(.rodata)
        draw_hook.o(.rodata*)
        draw_hook.o(.data)
        draw_hook.o(.bss)
    } > HUDDrawHook

    /* Custom section for our hook code */
    .ScanModeHook : 
    {
        scan_hook.o(.text)
        scan_hook.o(.rodata)
        scan_hook.o(.rodata*)
        scan_hook.o(.data)
        scan_hook.o(.bss)
    } > ScanModeHook

    /* Custom section for our hook code */
    .GUIScreenHook : 
    {
        screen_hook.o(.text)
        screen_hook.o(.rodata)
        screen_hook.o(.rodata*)
        screen_hook.o(.data)
        screen_hook.o(.bss)
    } > GUIScreenHook

    /* Custom section for our hook code */
    .AnimatorAnimBoundsHook : 
    {
        animatoranim_bounds_hook.o(.text)
        animatoranim_bounds_hook.o(.rodata)
        animatoranim_bounds_hook.o(.rodata*)
        animatoranim_bounds_hook.o(.data)
        animatoranim_bounds_hook.o(.bss)
    } > AnimatorAnimBoundsHook

    .SharkCodeCave : 
    {
        buttons.o(.text)
        buttons.o(.rodata)
        buttons.o(.rodata*)
        buttons.o(.data)
        buttons.o(.bss)
        buttons.o(.sdata)
        buttons.o(.sbss)
        buttons.o(.scommon)
        buttons.o(.buttons.o.*)
        colors.o(.text)
        colors.o(.rodata)
        colors.o(.rodata*)
        colors.o(.data)
        colors.o(.bss)
        colors.o(.sdata)
        colors.o(.sbss)
        colors.o(.scommon)
        colors.o(.colors.o.*)
        screenmath.o(.text)
        screenmath.o(.rodata)
        screenmath.o(.rodata*)
        screenmath.o(.data)
        screenmath.o(.bss)
        screenmath.o(.sdata)
        screenmath.o(.sbss)
        screenmath.o(.scommon)
        screenmath.o(.screenmath.o.*)
        textprint.o(.text)
        textprint.o(.rodata)
        textprint.o(.rodata*)
        textprint.o(.data)
        textprint.o(.bss)
        textprint.o(.sdata)
        textprint.o(.sbss)
        textprint.o(.scommon)
        textprint.o(.textprint.o.*)
        rotation.o(.text)
        rotation.o(.rodata)
        rotation.o(.rodata*)
        rotation.o(.data)
        rotation.o(.bss)
        rotation.o(.sdata)
        rotation.o(.sbss)
        rotation.o(.scommon)
        rotation.o(.rotation.o.*)
    } > SharkCodeCave

    .ViewerCodeCave : 
    {
        main.o(.text)
        main.o(.rodata)
        main.o(.rodata*)
        main.o(.data)
        main.o(.bss)
        main.o(.sdata)
        main.o(.sbss)
        main.o(.scommon)
        main.o(.main.o.*)
        modeshandle.o(.text)
        modeshandle.o(.rodata)
        modeshandle.o(.rodata*)
        modeshandle.o(.data)
        modeshandle.o(.bss)
        modeshandle.o(.sdata)
        modeshandle.o(.sbss)
        modeshandle.o(.scommon)
        modeshandle.o(.modeshandle.o.*)
        *(.text) /* Last section, place any potential remaining code sections */
        *(.branch_lt)
    } > ViewerCodeCave

    /DISCARD/ :
    {
        *(.comment)
        *(.pdr)
        *(.mdebug)
        *(.reginfo)
        *(.MIPS.abiflags)
        *(.eh_frame)
        *(.gnu.attributes)
    }
}