#include "KeyHandler.h"

extern bool holdKey;
extern int keyDetected;

int KeyHandler::Key() {
	if (_kbhit()) {
		int res = _getch();
		keyDetected = res;
		holdKey = true;
		return res;
	}
}