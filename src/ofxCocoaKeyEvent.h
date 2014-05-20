//
//  ofxCocoaKeyEvent.h
//
//  Created by TORU URAKAWA on 2014/04/10.
//
//

#pragma once

#include "ofMain.h"
#include <Carbon/Carbon.h>

enum KeyState {
    kDOWN = true,
    kUP = false,
};


class ofxCocoaKeyEvent {
public:
    void send(char c, KeyState k){
        CGEventSourceRef src = CGEventSourceCreate( kCGEventSourceStateHIDSystemState );
        CGEventRef ev = CGEventCreateKeyboardEvent (src, keyCodeForChar(c), k);
        CGEventPost( kCGHIDEventTap, ev );
        CFRelease( ev );
        CFRelease( src );
    }

    void send(int key, KeyState k){
        CGKeyCode code = getCodeFromOFKey(key);
        CGEventSourceRef src = CGEventSourceCreate( kCGEventSourceStateHIDSystemState );
        CGEventRef ev = CGEventCreateKeyboardEvent (src, code, k);
        CGEventPost( kCGHIDEventTap, ev );
        CFRelease( ev );
        CFRelease( src );
    }
    
    void send(char c, KeyState k, int num, ...){
        CGEventSourceRef src = CGEventSourceCreate( kCGEventSourceStateHIDSystemState );
        CGEventRef ev = CGEventCreateKeyboardEvent (src, keyCodeForChar(c), k);
        
        va_list args;
        va_start(args, num);
        
        for (int i = 0; i < num; i++) {
            int key = va_arg(args , int);
            CGEventFlags flags = getFlagFromOFKey(key);
            if (k == kDOWN) {
                CGEventSetFlags( ev, CGEventGetFlags( ev ) | flags );
            } else {
                // Use all existing flag except current one
                CGEventSetFlags( ev, CGEventGetFlags( ev ) & flags );
            }
        }
        
        CGEventPost( kCGHIDEventTap, ev );
        
        CFRelease( ev );
        CFRelease( src );
    }
    
    void send(int key, KeyState k, int num, ...){
        CGKeyCode code = getCodeFromOFKey(key);
        CGEventSourceRef src = CGEventSourceCreate( kCGEventSourceStateHIDSystemState );
        CGEventRef ev = CGEventCreateKeyboardEvent (src, code, k);
        
        va_list args;
        va_start(args, num);
        
        for (int i = 0; i < num; i++) {
            int key = va_arg(args , int);
            CGEventFlags flags = getFlagFromOFKey(key);
            if (k == kDOWN) {
                CGEventSetFlags( ev, CGEventGetFlags( ev ) | flags );
            } else {
                // Use all existing flag except current one
                CGEventSetFlags( ev, CGEventGetFlags( ev ) & flags );
            }
        }
        
        CGEventPost( kCGHIDEventTap, ev );
        
        CFRelease( ev );
        CFRelease( src );
    }

    
private:
    
    CGKeyCode keyCodeForChar(const char c)
    {
        CFDataRef currentLayoutData;
        TISInputSourceRef currentKeyboard = TISCopyCurrentKeyboardLayoutInputSource();
        
        if (currentKeyboard == NULL) {
            fputs("Could not find keyboard layout\n", stderr);
            return UINT16_MAX;
        }
        
        currentLayoutData = (CFDataRef)TISGetInputSourceProperty(currentKeyboard,
                                                                 kTISPropertyUnicodeKeyLayoutData);
        CFRelease(currentKeyboard);
        if (currentLayoutData == NULL) {
            fputs("Could not find layout data\n", stderr);
            return UINT16_MAX;
        }
        
        return keyCodeForCharWithLayout(c,
                                        (const UCKeyboardLayout *)CFDataGetBytePtr(currentLayoutData));
    }
    
    /* Beware! Messy, incomprehensible code ahead!
     * TODO: XXX: FIXME! Please! */
    CGKeyCode keyCodeForCharWithLayout(const char c,
                                       const UCKeyboardLayout *uchrHeader)
    {
        uint8_t *uchrData = (uint8_t *)uchrHeader;
        UCKeyboardTypeHeader *uchrKeyboardList = (UCKeyboardTypeHeader *)uchrHeader->keyboardTypeList;
        
        /* Loop through the keyboard type list. */
        ItemCount i, j;
        for (i = 0; i < uchrHeader->keyboardTypeCount; ++i) {
            /* Get a pointer to the keyToCharTable structure. */
            UCKeyToCharTableIndex *uchrKeyIX = (UCKeyToCharTableIndex *)
            (uchrData + (uchrKeyboardList[i].keyToCharTableIndexOffset));
            
            /* Not sure what this is for but it appears to be a safeguard... */
            UCKeyStateRecordsIndex *stateRecordsIndex;
            if (uchrKeyboardList[i].keyStateRecordsIndexOffset != 0) {
                stateRecordsIndex = (UCKeyStateRecordsIndex *)
                (uchrData + (uchrKeyboardList[i].keyStateRecordsIndexOffset));
                
                if ((stateRecordsIndex->keyStateRecordsIndexFormat) !=
                    kUCKeyStateRecordsIndexFormat) {
                    stateRecordsIndex = NULL;
                }
            } else {
                stateRecordsIndex = NULL;
            }
            
            /* Make sure structure is a table that can be searched. */
            if ((uchrKeyIX->keyToCharTableIndexFormat) != kUCKeyToCharTableIndexFormat) {
                continue;
            }
            
            /* Check the table of each keyboard for character */
            for (j = 0; j < uchrKeyIX->keyToCharTableCount; ++j) {
                UCKeyOutput *keyToCharData =
                (UCKeyOutput *)(uchrData + (uchrKeyIX->keyToCharTableOffsets[j]));
                
                /* Check THIS table of the keyboard for the character. */
                UInt16 k;
                for (k = 0; k < uchrKeyIX->keyToCharTableSize; ++k) {
                    /* Here's the strange safeguard again... */
                    if ((keyToCharData[k] & kUCKeyOutputTestForIndexMask) ==
                        kUCKeyOutputStateIndexMask) {
                        long keyIndex = (keyToCharData[k] & kUCKeyOutputGetIndexMask);
                        if (stateRecordsIndex != NULL &&
                            keyIndex <= (stateRecordsIndex->keyStateRecordCount)) {
                            UCKeyStateRecord *stateRecord = (UCKeyStateRecord *)
                            (uchrData +
                             (stateRecordsIndex->keyStateRecordOffsets[keyIndex]));
                            
                            if ((stateRecord->stateZeroCharData) == c) {
                                return (CGKeyCode)k;
                            }
                        } else if (keyToCharData[k] == c) {
                            return (CGKeyCode)k;
                        }
                    } else if (((keyToCharData[k] & kUCKeyOutputTestForIndexMask)
                                != kUCKeyOutputSequenceIndexMask) &&
                               keyToCharData[k] != 0xFFFE &&
                               keyToCharData[k] != 0xFFFF &&
                               keyToCharData[k] == c) {
                        return (CGKeyCode)k;
                    }
                }
            }
        }
        
        return UINT16_MAX;
    }
    
    CGKeyCode getCodeFromOFKey(int key) {
        switch (key) {
            case OF_KEY_ALT:
                return kVK_Option;
                
            case OF_KEY_COMMAND:
                return kVK_Command;
                
            case OF_KEY_CONTROL:
                return kVK_Control;
                
            case OF_KEY_DEL:
                return kVK_Delete;
                
            case OF_KEY_DOWN:
                return kVK_DownArrow;
                
            case OF_KEY_END:
                return kVK_End;
                
            case OF_KEY_ESC:
                return kVK_Escape;
                
            case OF_KEY_F1:
                return kVK_F1;
            case OF_KEY_F2:
                return kVK_F2;
            case OF_KEY_F3:
                return kVK_F3;
            case OF_KEY_F4:
                return kVK_F4;
            case OF_KEY_F5:
                return kVK_F5;
            case OF_KEY_F6:
                return kVK_F6;
            case OF_KEY_F7:
                return kVK_F7;
            case OF_KEY_F8:
                return kVK_F8;
            case OF_KEY_F9:
                return kVK_F9;
            case OF_KEY_F10:
                return kVK_F10;
            case OF_KEY_F11:
                return kVK_F11;
            case OF_KEY_F12:
                return kVK_F12;
                
            case OF_KEY_HOME:
                return kVK_Home;
                
            case OF_KEY_LEFT:
                return kVK_LeftArrow;
                
            case OF_KEY_PAGE_DOWN:
                return kVK_PageDown;
                
            case OF_KEY_PAGE_UP:
                return kVK_PageUp;
                
            case OF_KEY_RETURN:
                return kVK_Return;
                
            case OF_KEY_RIGHT:
                return kVK_RightArrow;
                
            case OF_KEY_SHIFT:
                return kVK_Shift;
                
            case OF_KEY_TAB:
                return kVK_Tab;
                
            case OF_KEY_UP:
                return kVK_UpArrow;
                
            default:
                return nil;
                break;
        }
    }
    
    CGEventFlags getFlagFromOFKey(int key) {
        switch (key) {
            case OF_KEY_ALT:
                return kCGEventFlagMaskAlternate;
            case OF_KEY_COMMAND:
                return kCGEventFlagMaskCommand;
            case OF_KEY_CONTROL:
                return kCGEventFlagMaskControl;
            case OF_KEY_SHIFT:
                return kCGEventFlagMaskShift;
            default:
                break;
        }
    }

    
};
