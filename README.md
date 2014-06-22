ofxCentralControl
=================

openframeworks addon used as central control
used together with ofxXmlSettings, ofxNetwork

Intro
=================
all projector configures was done in xml
all the function was self-evident

> ofxCentralControl m_CentralControl;

	void setProjector(u8 channel, bool turnOn = true);
	void closeProjector(u8 channel);

	bool setComputer(u16 n, u16 bitLen = 4);
	void closeComputer(u16 n, u16 bitLen = 4);

	bool setPower(u8 n, u16 bitLen = 1, bool force = false);
	void closePower(u8 n, u16 bitLen = 1);
	u8	 readPowerState();

	void openAllDevice();
	void closeAllDevice();

	void openAllProjector();
	void openAllComputer();
	void openAllPower();

	void closeAllProjector();
	void closeAllComputer();
	void closeAllPower();
