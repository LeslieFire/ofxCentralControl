#include "ofxCentralControl.h"

u8 BenQbaudRate = 0x04;
u8 BenQdataOpen[] = {0x06, 0x14, 0x00, 0x03, 0x00, 0x34, 0x11, 0x00, 0x5c};
u8 BenQdataClose[] = {0x06, 0x14, 0x00, 0x03, 0x00, 0x34, 0x11, 0x01, 0x5d};
u16 BenQdataLen = 9;



ofxCentralControl::ofxCentralControl(){
	ofSetLogLevel(OF_LOG_VERBOSE);
	
	mProjectors = NULL;
	mProHead = 0xCC;			// projector head
	mProTail = 0xDD;			// projector tail
	mProAddr = 0x01;			// projector address
	mProCmd = 0x80;				// projector command
	mProRet = 0x06;				// projector return command length
	mFixedLen = 0x000A;			//¹Ì¶¨³¤¶È
	maxProNum = 16;

	mPcHead = 0xCC;
	mPcTail = 0xDD;
	mPcFixedLen = 0x07;
	maxPcNum = 16;

	PORT = 50000;

	mPowerState = 0x00;
	mPowerHead = 0xCC;			// projector head
	mPowerTail = 0xDD;			// projector tail
	mPowerFixedLen = 0x07;
	maxPowerNum = 8;

	mLatency = 2000;

	int temp = 9;

	if ( xml.loadFile("settings.xml") ){
		ofLogVerbose() << "Load file successful !";

		PORT = xml.getValue("Network:PORT", PORT);
		mProHead = xml.getValue("Projector:framehead", mProHead);
		mProTail = xml.getValue("Projector:frametail", mProTail);
		mProAddr = xml.getValue("Projector:frameaddress", mProAddr);
		mProCmd = xml.getValue("Projector:command", mProCmd);
		mProRet = xml.getValue("Projector:returncommandlength", mProRet);
		mFixedLen = xml.getValue("Projector:fixedlength", mFixedLen);
		maxProNum = xml.getValue("Projector:maxprojectornumber", maxProNum);

		mPcHead = xml.getValue("Computer:framehead", mPcHead);
		mPcTail = xml.getValue("Computer:frametail", mPcTail);
		mPcFixedLen = xml.getValue("Computer:fixedlength", mPcFixedLen);
		maxPcNum = xml.getValue("Computer:maxcomputernumber", maxPcNum);

		mPowerHead = xml.getValue("Power:framehead", mPowerHead);
		mPowerTail = xml.getValue("Power:frametail", mPowerTail);
		mPowerFixedLen = xml.getValue("Power:fixedlength", mPowerFixedLen);
		maxPowerNum = xml.getValue("Power:maxpowernumber", maxPowerNum);
		mPowerState = xml.getValue("Power:PowerState", mPowerState);

		mLatency = xml.getValue("Configure:Latency", mLatency);
	}else{

		ofLogVerbose() << "Fail to load file! ";

		xml.setValue("Network:PORT", PORT);
		xml.setValue("Projector:framehead", mProHead);
		xml.setValue("Projector:frametail", mProTail);
		xml.setValue("Projector:frameaddress", mProAddr);
		xml.setValue("Projector:command", mProCmd);
		xml.setValue("Projector:returncommandlength", mProRet);
		xml.setValue("Projector:fixedlength", mFixedLen);
		xml.setValue("Projector:maxprojectornumber", maxProNum);

		xml.setValue("Computer:framehead", mPcHead);
		xml.setValue("Computer:frametail", mPcTail);
		xml.setValue("Computer:fixedlength", mPcFixedLen);
		xml.setValue("Computer:maxcomputernumber", maxPcNum);

		xml.setValue("Power:framehead", mPowerHead);
		xml.setValue("Power:frametail", mPowerTail);
		xml.setValue("Power:fixedlength", mPowerFixedLen);
		xml.setValue("Power:maxpowernumber", maxPowerNum);
		xml.setValue("Power:PowerState", mPowerState);

		xml.setValue("Configure:Latency", mLatency);

		for (int i = 0; i < maxProNum; ++i){
			string root = "Projectors:projector" + ofToString(i+1);
			string baudRate = root+":baudRate";
			xml.setValue(baudRate,temp);

			string codeOnLen = root+":TurnOnCodeLen";
			string codeOn = root+":TurnOnCode";
			xml.setValue(codeOnLen, temp);
			for (int j = 0; j < temp; ++j){
				xml.setValue(codeOn+":bit"+ofToString(j+1), temp);
			}

			string codeOffLen = root+":TurnOffCodeLen";
			string codeOff = root+":TurnOffCode";
			xml.setValue(codeOffLen, temp);
			for (int j = 0; j < temp; ++j){
				xml.setValue(codeOff+":bit"+ofToString(j+1), temp);
			}
		}
		xml.saveFile("settings.xml");
	}

	setupTCPServer(PORT);

	mProjectors = (projectorUnit *)malloc((maxProNum) * sizeof(projectorUnit));
	if (mProjectors != NULL){
		ofLogVerbose() << "allocate for projects' Unit OK!";
	}else{
		ofLogVerbose() << "allocate for projects' Unit Failed!";
	}

	// read all projector commands
	for (int i = 0; i < maxProNum; ++i){
		string root = "Projectors:projector" + ofToString(i+1);
		mProjectors[i].channal = i+1;

		string baudRate = root+":baudRate";
		mProjectors[i].baudRate = xml.getValue(baudRate, mProjectors[i].baudRate);

		string codeOnLen = root+":TurnOnCodeLen";
		string codeOn = root+":TurnOnCode";
		mProjectors[i].onLength = xml.getValue(codeOnLen, mProjectors[i].onLength);
		for (int j = 0; j < mProjectors[i].onLength; ++j){
			mProjectors[i].codeOn[j] = xml.getValue(codeOn+":bit"+ofToString(j+1), mProjectors[i].codeOn[j]);
		}

		string codeOffLen = root+":TurnOffCodeLen";
		string codeOff = root+":TurnOffCode";
		mProjectors[i].offLength = xml.getValue(codeOffLen, mProjectors[i].offLength);
		for (int j = 0; j < mProjectors[i].offLength; ++j){
			mProjectors[i].codeOff[j] = xml.getValue(codeOff+":bit"+ofToString(j+1), mProjectors[i].codeOff[j]);
		}
	}
}

ofxCentralControl::~ofxCentralControl(){

	TCPServer.close();

	if (mProjectors != NULL){
		free(mProjectors);
		mProjectors = NULL;
	}
}

u16 ofxCentralControl::ModbusRTU_CRC16(u8 *puchMsg, u16 usDataLen) 
{ 
	u8 uchCRCHi = 0xFF;
	u8 uchCRCLo = 0xFF;
	u32 uIndex;

	while (usDataLen--)
	{ 
		uIndex = uchCRCHi ^ *puchMsg++;
		uchCRCHi = uchCRCLo ^ auchCRCHi[uIndex]; 
		uchCRCLo = auchCRCLo[uIndex]; 
	}
	
	return (uchCRCHi << 8 | uchCRCLo); 
}

void ofxCentralControl::setupTCPServer(int port){
	TCPServer.setup(port);
}

bool ofxCentralControl::setProjector(u8 channel, u8 baudRate, u8 *data, u16 dataLen, unsigned char *msg, int &frameLen)
{
	u8 *sendList = (u8 *)malloc((mFixedLen + dataLen + 1)*sizeof(u8));
	if ( sendList == NULL ){
		ofLogVerbose() << " Failed to allocate memory for sendList! ";
		return false;
	}else{
		ofLogVerbose() << " Successful to allocate memory for sendList! ";
	}

	ostringstream oss;
	u16 offset = 0;
	sendList[offset++] = mProHead;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mProAddr;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(mFixedLen + dataLen);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mProCmd;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mProRet;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = channel;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = baudRate;
	oss << hex << (int)sendList[offset - 1] << " ";

	memcpy(sendList + offset, data, dataLen);
	offset += dataLen;
	for (int i = offset - dataLen; i < offset; ++i){
		oss << hex << (int)sendList[i] << " ";
	}

	ofLogVerbose() << "Frame length before CRC : " + ofToString(offset);
	u16 uiCRC = ModbusRTU_CRC16(sendList, offset);
	ofLogVerbose() <<"CRC : "<< hex << uiCRC;

	sendList[offset++] = (u8)(uiCRC >> 8);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(uiCRC & 0x00FF);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mProTail;
	oss << hex << (int)sendList[offset - 1] << " ";

	ofLogVerbose() << "Frame sent : " + oss.str() + " with length " + ofToString(offset);

	memcpy(msg, sendList, offset);
	frameLen = offset;

	if (sendList != NULL){
		free(sendList);
		sendList  = NULL;
	}
	return true;
}

void ofxCentralControl::setProjector(u8 channel, bool turnOn){
	if (channel >= 0 && channel < maxProNum){
		if (turnOn){
			setProjector(channel,  mProjectors[channel-1].baudRate, mProjectors[channel-1].codeOn,  mProjectors[channel-1].onLength);
		}else{
			setProjector(channel,  mProjectors[channel-1].baudRate, mProjectors[channel-1].codeOff,  mProjectors[channel-1].offLength);
		}
	}
}

bool ofxCentralControl::setProjector(u8 channel, u8 baudRate, u8 *data, u16 dataLen){
	unsigned char sendList[256];
	int frameLength = 0;

	if ( !setProjector(channel, baudRate, data, dataLen, sendList, frameLength) ){
		ofLogVerbose() << "Failed to set projector state!";
		return false;
	} else {
		ofLogVerbose() << "Success to set projector state!";
	}
	TCPServer.sendRawBytesToAll((char*)sendList, frameLength);

	return true;
}

void ofxCentralControl::closeProjector(u8 channel){
	if ( channel <= maxProNum && channel > 0){
		setProjector(channel, false);
	}
}

bool ofxCentralControl::setComputer(u16 n, u16 bitLen, unsigned char *msg, int &frameLen){
	u8 *sendList = (u8 *)malloc((mPcFixedLen + bitLen + 1)*sizeof(u8));

	if ( sendList == NULL ){
		ofLogVerbose() << " Failed to allocate memory for sendList! ";
		return false;
	}else{
		ofLogVerbose() << " Successful to allocate memory for sendList! ";
	}

	ostringstream oss;
	u16 offset = 0;
	sendList[offset++] = mPcHead;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = 0x01;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(mPcFixedLen + bitLen);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = 0x01;
	oss << hex << (int)sendList[offset - 1] << " ";

	offset += bitLen;
	for ( int i = 0; i < bitLen; ++i){
		u8 x = 1;
		x = x << (n - 1 - 8*i);
		sendList[offset - 1 - i] = x;
	}

	for (int i = offset - bitLen; i < offset; ++i){
		oss << hex << (int)sendList[i] << " ";
	}

	ofLogVerbose() << "Frame length before CRC : " + ofToString(offset);
	u16 uiCRC = ModbusRTU_CRC16(sendList, offset);
	ofLogVerbose() <<"CRC : "<< hex << uiCRC;

	sendList[offset++] = (u8)(uiCRC >> 8);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(uiCRC & 0x00FF);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mPcTail;
	oss << hex << (int)sendList[offset - 1] << " ";

	ofLogVerbose() << "Frame sent : " + oss.str() + " with length " + ofToString(offset);
	memcpy(msg, sendList, offset);
	frameLen = offset;

	if (sendList != NULL){
		free(sendList);
		sendList  = NULL;
	}

	return true;
}

bool ofxCentralControl::setComputer(u16 n, u16 bitLen){
	unsigned char sendList[256];
	int frameLength = 0;

	if ( !setComputer(n, bitLen, sendList, frameLength) ){
		ofLogVerbose() << "Failed to set computer state!";
		return false;
	} else {
		ofLogVerbose() << "Success to set computer state!";
	}
	TCPServer.sendRawBytesToAll((char*)sendList, frameLength);

	return true;
}

void ofxCentralControl::closeComputer(u16 n, u16 bitLen){
	if ( n < maxPcNum ){
		setComputer(n, 4);
	}
}

bool ofxCentralControl::setPower(u8 n, u16 bitLen, unsigned char * msg, int &frameLen, bool force){
	if (n >= 0 && n <= maxPowerNum){
		u8 *sendList = (u8 *)malloc((mPowerFixedLen + bitLen + 1)*sizeof(u8));

		if ( sendList == NULL ){
			ofLogVerbose() << " Failed to allocate memory for sendList! ";
			return false;
		}else{
			ofLogVerbose() << " Successful to allocate memory for sendList! ";
		}

		ostringstream oss;
		u16 offset = 0;
		sendList[offset++] = mPowerHead;
		oss << hex << (int)sendList[offset - 1] << " ";

		sendList[offset++] = 0x01;
		oss << hex << (int)sendList[offset - 1] << " ";

		sendList[offset++] = (u8)(mPowerFixedLen + bitLen);
		oss << hex << (int)sendList[offset - 1] << " ";

		sendList[offset++] = 0x21;
		oss << hex << (int)sendList[offset - 1] << " ";

		offset += bitLen;
		for ( int i = 0; i < bitLen; ++i){
			u8 x = 1;
			x = x << (n - 1 - 8*i);
			if (force){
				mPowerState |= x;
			}else{
				mPowerState ^= x; 
			}
			if (n == 0){
				//close all
				mPowerState = 0x00;
			}
			sendList[offset - 1 - i] = mPowerState;
		}

		for (int i = offset - bitLen; i < offset; ++i){
			oss << hex << (int)sendList[i] << " ";
		}

		ofLogVerbose() << "Frame length before CRC : " + ofToString(offset);
		u16 uiCRC = ModbusRTU_CRC16(sendList, offset);
		ofLogVerbose() <<"CRC : "<< hex << uiCRC;

		sendList[offset++] = (u8)(uiCRC >> 8);
		oss << hex << (int)sendList[offset - 1] << " ";

		sendList[offset++] = (u8)(uiCRC & 0x00FF);
		oss << hex << (int)sendList[offset - 1] << " ";

		sendList[offset++] = mPowerTail;
		oss << hex << (int)sendList[offset - 1] << " ";

		ofLogVerbose() << "Frame sent : " + oss.str() + " with length " + ofToString(offset);
		memcpy(msg, sendList, offset);
		frameLen = offset;

		if (sendList != NULL){
			free(sendList);
			sendList  = NULL;
		}
	}else{
		return false;
	}
	
	return true;
}

bool ofxCentralControl::setPower(u8 n, u16 bitLen, bool force){
	unsigned char sendList[256];
	int frameLength = 0;

	if ( !setPower(n, bitLen, sendList, frameLength, force) ){
		ofLogVerbose() << "Failed to set Power state!";
		return false;
	} else {
		ofLogVerbose() << "Success to set Power state!";
	}
	TCPServer.sendRawBytesToAll((char*)sendList, frameLength);
	

	xml.setValue("Power:PowerState", mPowerState);
	xml.saveFile("settings.xml");
	
	return true;
}

void ofxCentralControl::closePower(u8 n, u16 bitLen){
	if ( n < maxPowerNum){
		setPower(n, 1);
	}
}

bool ofxCentralControl::readPowerState(unsigned char * msg, int &frameLen){
	u8 *sendList = (u8 *)malloc((mPowerFixedLen + 1)*sizeof(u8));

	if ( sendList == NULL ){
		ofLogVerbose() << " Failed to allocate memory for sendList! ";
		return false;
	}else{
		ofLogVerbose() << " Successful to allocate memory for sendList! ";
	}

	ostringstream oss;
	u16 offset = 0;
	sendList[offset++] = mPowerHead;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = 0x01;
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(mPowerFixedLen);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = 0x22;
	oss << hex << (int)sendList[offset - 1] << " ";

	ofLogVerbose() << "Frame length before CRC : " + ofToString(offset);
	u16 uiCRC = ModbusRTU_CRC16(sendList, offset);
	ofLogVerbose() <<"CRC : "<< hex << uiCRC;

	sendList[offset++] = (u8)(uiCRC >> 8);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = (u8)(uiCRC & 0x00FF);
	oss << hex << (int)sendList[offset - 1] << " ";

	sendList[offset++] = mPowerTail;
	oss << hex << (int)sendList[offset - 1] << " ";

	ofLogVerbose() << "Frame sent : " + oss.str() + " with length " + ofToString(offset);
	memcpy(msg, sendList, offset);
	frameLen = offset;

	if (sendList != NULL){
		free(sendList);
		sendList  = NULL;
	}
	return true;
}

u8 ofxCentralControl::readPowerState(){
	unsigned char sendList[256];
	int frameLength = 0;

	if ( !readPowerState(sendList, frameLength) ){
		ofLogVerbose() << "Failed to read Power state!";
		return false;
	} else {
		ofLogVerbose() << "Success to read Power state!";
	}
	TCPServer.sendRawBytesToAll((char*)sendList, frameLength);

	return mPowerState;
}

void ofxCentralControl::openAllDevice(){
	openAllComputer();
	openAllProjector();
	openAllPower();
}

void ofxCentralControl::closeAllDevice(){
	closeAllComputer();
	closeAllProjector();
	closeAllPower();
}

void ofxCentralControl::openAllProjector(){
	for ( int i = 0; i < maxProNum; ++i ){
		setProjector(i+1, true);
		Sleep(mLatency);
	}
}

void ofxCentralControl::openAllComputer(){
	for ( int i = 0; i < maxPcNum; ++i ){
		setComputer(i+1);
		Sleep(mLatency);
	}
}

void ofxCentralControl::openAllPower(){
	for ( int i = 0; i < maxPowerNum; ++i ){
		setPower(i+1, 1, true);
		Sleep(mLatency);
	}
}

void ofxCentralControl::closeAllProjector(){
	for ( int i = 0; i < maxProNum; ++i ){
		closeProjector(i+1);
		Sleep(mLatency);
	}
}

void ofxCentralControl::closeAllComputer(){
	openAllComputer();  // the same as open computers
}

void ofxCentralControl::closeAllPower(){
	setPower(0);
}