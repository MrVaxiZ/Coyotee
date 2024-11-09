#ifndef STATICDATA_H
#define STATICDATA_H

class StaticData {
private:
	static bool RecordingButtonPressed;
	static bool StreamingButtonPressed;

public:
	void setRecordingButtonStatus(bool status);
	bool getRecordingButtonStatus();
	void setStreamingButtonStatus(bool status);
	bool getStreamingButtonStatus();
};

#endif // !STATICDATA_H
