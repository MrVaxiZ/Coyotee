#ifndef KEYHANDLER_H
#define KEYHANDLER_H

#include <iostream>
#include "conio.h"
#include "KeyboardKeyCodes.h"

struct KeyHandler {
    // Variables
    bool holdKey = false;
    const int keyAssigned = VK_F8;
    int keyDetected;

    // Methodes
    int Key();
};

#endif // KEYHANDLER_H
