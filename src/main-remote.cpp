	#ifdef REMOTE
	#include <math.h>
	#include <esp_now.h>
	#include "esp_wifi.h"
	#include <WiFi.h>
	#include <batteryMonitor.h>
	#include <motorControl.h>
	#include <ledUtility.h>
	#include "esp_log.h"
	#include "mac.h"

	static const char *TAG = "MAIN";
	
	//------------ turn on generic serial printing

	#define DEBUG_PRINTS

	//data that will be sent to the receiver

	typedef struct {
	int16_t speedmotorLeft;
	int16_t speedmotorRight;
	int16_t packetArg1;
	int16_t packetArg2;
	int16_t packetArg3;
	}
	packet_t;


	packet_t sentData;
	packet_t recData;

	#define UPMAX	0
	#define UPMIN	410
	#define DOWNMIN	590
	#define DOWNMAX	1023

	#define LMAX	0
	#define LMIN	410
	#define RMIN	590
	#define RMAX	1023

	//---------------------------------------ESP_NOW Variables


	String success;
	esp_now_peer_info_t peerInfo;
	// Callback when data is sent
	void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
	if (status == 0) {
		success = "Delivery Success :)";
	}
	else {
		success = "Delivery Fail :(";
	}
	}

	// Callback when data is received
	void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
	memcpy(&recData, incomingData, sizeof(recData));
	}




	//---------------------------------------HARDWARE DEPENDANT Variables
	// one ifdef case per hardware to speed up modularity of the code

	//RAC standard remote
	const int steerPot = 7;
	const int accPot = 10;
	const int leverPot = 8;
	//const int trimPot = 39;

	const int rightBtn = 2;
	const int leftBtn = 4;
	const int topBtn = 5;
	//const int lowSwitch = 32;
	//const int topSwitch = 25;
	LedUtility Led(21);

	//customisable vars
	int analogRes = 10;
	int analogReadMax = (1 << analogRes)-1;
	int accValue;
	int strValue;


	//variables for the sketch
	int leverValue = 0;

	unsigned long current_time = 0;


	void setup() {
	//store_values(); // uncomment only to initialize mem
	analogReadResolution(analogRes);
	analogSetAttenuation(ADC_11db);
	pinMode(rightBtn, INPUT_PULLUP);
	pinMode(leftBtn, INPUT_PULLUP);
	pinMode(topBtn, INPUT_PULLUP);
	//pinMode(lowSwitch, INPUT_PULLUP);
	//pinMode(topSwitch, INPUT_PULLUP);
	Led.init();
	Led.setBlinks(1,150);
	delay(2000);
	#ifdef DEBUG_PRINTS
	Serial.begin(115200);
	Serial.println("RAC GENERIC BOT");
	#endif


	//---------------------------------------ESP NOW setup
	WiFi.mode(WIFI_STA);
	if (esp_now_init() != ESP_OK) {
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_send_cb(OnDataSent);
	memcpy(peerInfo.peer_addr, robotAddress, 6);
	peerInfo.channel = 0;
	peerInfo.encrypt = false;
	if (esp_now_add_peer(&peerInfo) != ESP_OK) {
		Serial.println("Failed to add peer");
		return;
	}
	char macStr[18];
	Serial.print("Packet from: ");
	// Copies the sender mac address to a string
	snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
			peerInfo.peer_addr[0], peerInfo.peer_addr[1], peerInfo.peer_addr[2], peerInfo.peer_addr[3], peerInfo.peer_addr[4], peerInfo.peer_addr[5]);
	Serial.print("sending to: ");
	Serial.println(macStr);
	esp_now_register_recv_cb(OnDataRecv);
	Led.setBlinks(0);
	Led.ledOn();
	}

	void	fullForward( void )
	{
		sentData.speedmotorLeft = -512;
		sentData.speedmotorRight = 512;
	}

	void	fullBack( void )
	{
		sentData.speedmotorLeft = 512;
		sentData.speedmotorRight = -512;
	}

	void	fullStop( void )
	{
		sentData.speedmotorLeft = 0;
		sentData.speedmotorRight = 0;
	}
	
	void	fullturnLeft( void )
	{
		sentData.speedmotorLeft = -512;
		sentData.speedmotorRight = -512;
	}

	void	fullturnRight( void )
	{
		sentData.speedmotorLeft = 512;
		sentData.speedmotorRight = 512;
	}

	void	controlSpeed( int accValue, int strValue )
	{
		if ( strValue > 970 )
		{
			fullturnRight();
			return ;
		}
		if ( strValue < 50 )
		{
			fullturnLeft();
			return ;
		}
		if ( accValue >= UPMIN && accValue <= DOWNMIN )
			fullStop();
		else if ( accValue < UPMIN ) // up
			fullForward();
		else if ( accValue > DOWNMIN ) // down
			fullBack();
	}

	void loop() {
	//read pots values
	strValue = analogRead(steerPot); // left == 0 && right == 1023
	delay(3);
	accValue = analogRead(accPot); // up == 0 && down == 1023
	delay(3);
	leverValue = analogRead(leverPot);
	delay(3);
	current_time = millis(); 
	bool rightValue = !digitalRead(rightBtn);
	bool leftValue = !digitalRead(leftBtn);
	bool topValue = !digitalRead(topBtn);

///////////////////////////////////////////////////////////////////

	// left e' al contrario
	controlSpeed( accValue, strValue );
	
	// -------------------------------------------- //
	esp_err_t result = -1;
	result = esp_now_send(robotAddress, (uint8_t *) &sentData, sizeof(sentData));
	if (result == ESP_OK) {
		// Serial.println("Sent with success");
	} else {
		// Serial.println("Error sending the data");
	}
	delay(10);
	}
	#endif