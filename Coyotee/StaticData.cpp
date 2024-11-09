#include "StaticData.h"

bool StaticData::StreamingButtonPressed = false;
bool StaticData::RecordingButtonPressed = false;

void StaticData::setStreamingButtonStatus(bool status) {
	StreamingButtonPressed = status;
}

bool StaticData::getStreamingButtonStatus() {
	return StreamingButtonPressed;
}
void StaticData::setRecordingButtonStatus(bool status) {
	RecordingButtonPressed = status;
}

bool StaticData::getRecordingButtonStatus() {
	return RecordingButtonPressed;
}
