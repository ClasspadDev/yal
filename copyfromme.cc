// 2000
static const auto my_Debug_Printf = (void (*)(int, int, bool, int, const char*, ...))0x8002DBC8;
static const auto my_Debug_WaitKey = (int (*)())0x80094380;
static const auto my_LCD_Refresh = (void (*)())0x8003733E;

// 7002
static const auto my_Debug_Printf = (void (*)(int, int, bool, int, const char*, ...))0x8002dbb0;
static const auto my_Debug_WaitKey = (int (*)())0x8009521c;
static const auto my_LCD_Refresh = (void (*)())0x800373ac;

// all
#define wait(...) { \
    my_Debug_Printf(0, 0, true, 0, __VA_ARGS__); \
    my_LCD_Refresh(); \
    my_Debug_WaitKey(); \
}
