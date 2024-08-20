#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include "sv/sv.h"

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

bool ChekIfNewline(char *character, int size)
{
    int i;
    
    for (i = 0; i < size - 1; ++i) {
        if (isspace(character[i]) == 0) {
            return false;
        }
    }
    
    return true;
}

int32_t QueryNumber(int32_t minInclusive, int32_t maxInclusive, int32_t defaultValue)
{
    int32_t selectedNumber = defaultValue;
    bool validNumber = false;
    uint32_t i;
    
    do {
        char selection[256];
        char *err = fgets(selection, sizeof(selection), stdin);
        if (err == NULL) {
            continue;		
        }

        if (ChekIfNewline(selection, strlen(selection))) {
            return defaultValue;
        }
        
        validNumber = true; 
        for (i = 0; i < (strlen(selection) - 1); i++) {
            if (!isdigit(selection[i])) {
                validNumber = false;
                printf("Invalid number\n");
                break;
            }
        }
        
        if (validNumber) {
            selectedNumber = atoi(selection);
            if (selectedNumber < minInclusive || selectedNumber > maxInclusive) {
                printf("Number out of bounds!\n");
                validNumber = false;
            }
        }

    } while(!validNumber);

    printf("\n");

    return selectedNumber;
}

bool CheckQueryBool(const char* value, uint32_t sizeValue, const char **checkedValue, int32_t sizeCheckedValue)
{
    int32_t i;
    
    for (i = 0; i < sizeCheckedValue; i++) {
        if(sizeValue == strlen(checkedValue[i])) {
            if(strncmp(value, checkedValue[i], strlen(checkedValue[i])) == 0) {
                return true;
            }
        }
    }
    return false;
}

bool QueryBool(bool defaultValue)
{
    const char *trueValues[] = { "y", "yes", "true", "ok", "1" };
    const char *falseValues[] = { "n", "no", "false", "0" };
    uint32_t i;
    
    do {
        char line[256];
        char *err = fgets(line, sizeof(line), stdin);
        if (err == NULL) {
            continue;		
	}
        
        for (i = 0; i < strlen(line); i++) {
            line[i] = tolower(line[i]);
        }
        
        if (ChekIfNewline(line, strlen(line))) {
            return defaultValue;
        } else if (CheckQueryBool(line, (strlen(line) - 1), trueValues, ARRAY_LENGTH(trueValues))) {
            printf("\n");
            return true;
        } else if (CheckQueryBool(line, (strlen(line) - 1), falseValues, ARRAY_LENGTH(falseValues))) {
            printf("\n");
            return false;
        } else {
            printf("Invalid input\n");
        }
    } while(true);
}

void SelectPixelFormat(IControl* control)
{
    int menuSize;
    int32_t i;
    
    CMenuEntryList menu = sv_control_GetMenuEntries(control, &menuSize);
    if (menuSize == 0) {
        printf("Pixel format menu empty.\n");
        return;
    }

    printf("Choose a pixel format [0]\n");
    for (i = 0; i < menuSize; i++) {
        printf("%d - %s\n",menu[i].index, menu[i].name);
    }

    int32_t index = QueryNumber(0, menuSize - 1, 0);
    sv_control_Set(control, index);

    if (sv_control_Get(control) != index) {
        printf("Failed to set pixel format.\n");
        printf("Current pixel format is:\n");
        printf("ID: %d Name: %s\n", menu[sv_control_Get(control)].index, menu[sv_control_Get(control)].name);
    }
}

void SelectFrameSize(IControl* frmsizControl)
{
    int menuSizeFrmsiz;
    int i;
    
    CMenuEntryList menufrmsiz = sv_control_GetMenuEntries(frmsizControl, &menuSizeFrmsiz);
    if (menuSizeFrmsiz == 0) {
        printf("Frame size menu empty.\n");
        return;
    }

    printf("Choose a frame size [0]\n");
    for (i = 0; i < menuSizeFrmsiz; i++) {
        printf("%d - %s\n", menufrmsiz[i].index, menufrmsiz[i].name);
    }

    int32_t index = QueryNumber(0, menuSizeFrmsiz - 1, 0);
    sv_control_Set(frmsizControl, index);

    if (sv_control_Get(frmsizControl) != index) {
        printf("Failed to set frame size.\n");
        printf("Current Frame size is:\n");
        printf("ID: %d Name: %s\n", menufrmsiz[sv_control_Get(frmsizControl)].index, menufrmsiz[sv_control_Get(frmsizControl)].name);
    }
}

ICamera* SelectCamera(const CICameraList *cameras, int32_t size)
{
    int32_t i;
    int32_t j = 0;
    
    printf("Choose a camera [0]\n");
    
    for (i = 0; i < size; i++) {
        ICamera *camera = (ICamera*)((*cameras)[i]);
        printf("%d - %s\n", j, sv_camera_GetName(camera));
        j += 1;
    }

    int32_t index = QueryNumber(0, size - 1, 0);

    return (ICamera*)((*cameras)[index]);
}

int32_t SelectValue(const char *name, int32_t minValue, int32_t maxValue, int32_t defaultValue)
{
    printf("Enter %s [%d]\n", name, defaultValue);
    printf("Min: %d\n", minValue);
    printf("Max: %d\n", maxValue);

    return QueryNumber(minValue, maxValue, defaultValue);
}

bool SelectEnable(char *name, bool defaultValue)
{ 
    if (defaultValue == true) {
        printf("Enable %s [true]\n", name);
    } else {
        printf("Enable %s [false]\n", name);
    }
    
    printf("false\n");
    printf("true\n");
    return QueryBool(defaultValue);
}

int32_t SelectFromMenu(char *name, char menu[][64], uint32_t menuSize, uint32_t defaultIndex)
{
    uint32_t i;
    
    printf("Select %s [%d]\n", name, defaultIndex);
    
    for (i = 0; i < menuSize; ++i) {
        printf("%d - %s\n", i, menu[i]);
    }

    return QueryNumber(0, menuSize - 1, defaultIndex);
}
   
void WaitForEnter()
{
    printf("Press ENTER to exit...\n");
    getchar();
}
