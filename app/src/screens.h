// While displaying screens we need to implemnt *_getItem function and *_getNumItems function.
// There is a big overlap in the logic. These macros generate *_internal function that can be used to implemnt both functions.
// We provide SCREEN macros to achive this efficiently.
//
// Function *_internal parameters:
// displayIdx - display index or MAX_DISPLAYS if we want to count the number of displays
// pageIdx - page index within the display, not used if displayIdx == MAX_DISPLAYS
// outKey, outKeyLen, outVal, outValLen - strings to be displayed, uused if == MAX_DISPLAYS
// pageCount - returns the number of pages (displayIdx<MAX_DISPLAYS) or screens (displayIdx==MAX_DISPLAYS)
//             you should use this variable internally only to set the number of pages within BEGIN_SCREEN and END_SCREEN
//
// Macro parameters:
// functionNemePrefix_ - how the function should be named (with _internal appended)
// errType_ - return value of the function, shouldbe an error type
// okReturnValue_ - what to return in case of success
// errorReturnValue_ - what to return in case of failure
// 
// Usage:
// Between INIT_SCREENS and FINISH_SCREENS you can write any code you want.
// e.g. you can use if statements to decide, if you should show a screen or not
// Within BEGIN_SCREEN and END_SCREEN you should implement how to fill outKey and outVal (and *pageCount, if it should not be 1)
// Example use: see addr.c

#define MAX_DISPLAYS UINT8_MAX

#define BEGIN_SCREENS_FUNCTION(functionNemePrefix_, errType_, okReturnValue_, errorReturnValue_, errorNoDataValue_) \
    static errType_ functionNemePrefix_ ## _internal (uint8_t displayIdx,                                           \
                                                      char *outKey, uint16_t outKeyLen,                             \
                                                      char *outVal, uint16_t outValLen,                             \
                                                      uint8_t pageIdx, uint8_t *pageCount) {                        \
    errType_ okReturnValue = (okReturnValue_);                                                                      \
    errType_ errorReturnValue = (errorReturnValue_);                                                                \
    errType_ errorNoDataValue = (errorNoDataValue_);                                                                \
    uint8_t isGetItemMode = (displayIdx != MAX_DISPLAYS);                                                           \
                                                                                                                    \
    if ((isGetItemMode && !outKey) || (isGetItemMode && !outVal) || !pageCount) {                                   \
        return (errorReturnValue);                                                                                  \
    };                                                                                                              \
                                                                                                                    \
    uint8_t currentDisplay = 0;                                                                                     \
    *pageCount = 1;                                                               

#define BEGIN_SCREEN                                                        \
        if (currentDisplay == MAX_DISPLAYS) return errorReturnValue;        \
        if (currentDisplay++ == displayIdx) {  

#define END_SCREEN return okReturnValue; }; 

#define END_SCREENS_FUNCTION                                \
        if (isGetItemMode) {                                \
            return (errorNoDataValue);                      \
        }                                                   \
        else {                                              \
            *pageCount = currentDisplay;                    \
            return (okReturnValue);                         \
        }                                                   \
    }
