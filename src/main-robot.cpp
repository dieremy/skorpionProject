#ifdef ROBOT
#include <esp_now.h>
#include "esp_wifi.h"
#include <WiFi.h>
#include <motorControl.h>
#include <batteryMonitor.h>
#include <ledUtility.h>
#include "esp_log.h"
#include "mac.h"
static const char *TAG = "MAIN";

#define weapPot 7

//------------ turn on generic serial printing
// #define DEBUG_PRINTS

#define MOTOR_A_IN1 8	// 8
#define MOTOR_A_IN2 18	// 18

#define MOTOR_B_IN1 17	// 16
#define MOTOR_B_IN2 16	// 17

#define MOTOR_C_IN1 5
#define MOTOR_C_IN2 4

// positivo arma sale
// negativo arma scende

// RIGHT
MotorControl motor1 = MotorControl(MOTOR_B_IN1, MOTOR_B_IN2);
// LEFT
MotorControl motor2 = MotorControl(MOTOR_A_IN1, MOTOR_A_IN2);
// WPN
MotorControl motor3 = MotorControl(MOTOR_C_IN1, MOTOR_C_IN2,180);  // 300

BatteryMonitor Battery = BatteryMonitor();

LedUtility Led = LedUtility();

typedef struct
{
	int16_t speedmotorLeft;
	int16_t speedmotorRight;
	int16_t packetArg1;
	int16_t packetArg2;
	int16_t packetArg3;
} packet_t;
packet_t recData;

bool failsafe = false;
unsigned long failsafeMaxMillis = 400;
unsigned long lastPacketMillis = 0;

int recLpwm = 0;
int recRpwm = 0;
int recArg1 = 0;
int recArg2 = 0;
int recArg3 = 0;

// Callback when data is received
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
	memcpy(&recData, incomingData, sizeof(recData));
	recLpwm = recData.speedmotorLeft;
	recRpwm = recData.speedmotorRight;
	recArg1 = recData.packetArg1;
	recArg2 = recData.packetArg2;
	recArg3 = recData.packetArg3;
	lastPacketMillis = millis();
	failsafe = false;
}

int handle_blink()
{
	if (Battery.isLow())
	{
		Led.setBlinks(1, 1000);
		return 1;
	}
	if (failsafe)
	{
		Led.setBlinks(2, 500);
		return -1;
	}
	Led.setBlinks(0);
	Led.ledOn();
	return 0;
}

void setup()
{
	Serial.begin(115200);
	Serial.println("Ready.");
	analogReadResolution(10);
	Led.init();
	delay(20);
	Led.setBlinks(1, 150);
	delay(2000);
	Battery.init();
	delay(20);

	analogSetAttenuation(ADC_11db);
	motor1.setSpeed(0);
	motor2.setSpeed(0);
	motor3.setSpeed(0);
	delay(500);

	WiFi.mode(WIFI_STA);
	esp_wifi_set_channel(1, WIFI_SECOND_CHAN_NONE);
	if (esp_wifi_set_mac(WIFI_IF_STA, &robotAddress[0]) != ESP_OK)
	{
		Serial.println("Error changing mac");
		return;
	}
	Serial.println(WiFi.macAddress());
	if (esp_now_init() != ESP_OK)
	{
		Serial.println("Error initializing ESP-NOW");
		return;
	}
	esp_now_register_recv_cb(OnDataRecv);
	Led.setBlinks(0);
	Led.ledOn();
}

#define WPDOWN 825
#define WPUP 0

float kp = 3.1;
float kd = 15.0;
float ki = 0.10;
float prev_err = 0.0;
int	  level;
float err;
float pwn;
float integral = 0.0;

void controlWeapon(int target)
{
	level = analogRead(weapPot);
	target = map(target, 0, 1023, 0, 830);
		
	err = ( level - target );
	if ( abs( err ) < 10 )
		integral += err * ki;
	else
		integral = 0;
	float deriv = ( err - prev_err );
	
	pwn = err * kp  + deriv * kd + integral;
	pwn = constrain( pwn, -512, 512 );

	if ( abs( err ) < 10 )
		motor3.setSpeed( 0 );
	else
		motor3.setSpeed( pwn );
	prev_err = err;

	// Serial.print("err: ");
	// Serial.print( err );
	// Serial.print("\tderiv: ");
	// Serial.print( deriv );
	// Serial.print("\tintegral: ");
	// Serial.print( integral );
	// Serial.print("\tlevel: ");
	// Serial.print( level );
	// Serial.print("\tpwn: ");
	// Serial.println( pwn );
}

unsigned long actualTime;
unsigned long newTime;
unsigned long limit = 10;

// positivo arma sale
// negativo arma scende

void	comboSting( int sting )
{
	if ( sting == 1 )
	{
		motor3.setSpeed( 512 );
		delay(150);
		motor3.setSpeed( -512 );

		delay(80);
		return ;
	}
		// actualTime = millis();
		// if ( ( actualTime - millis() ) > 10 )
	// if ( sting == 0 )
	// 	motor3.setSpeed( 0 );
}

void loop()
{

	unsigned long current_time = millis();
	if (current_time - lastPacketMillis > failsafeMaxMillis)
	{
		failsafe = true;
	}
	handle_blink();
	if (failsafe)
	{
		motor1.setSpeed(0);
		motor2.setSpeed(0);
		motor3.setSpeed(0);
	}
	else
	{
		// vvvv ----- YOUR AWESOME CODE HERE ----- vvvv //

		// int speedLevel = controlWeap( wpn, recArg1 );

		motor1.setSpeed(recLpwm);
		motor2.setSpeed(recRpwm);

		comboSting( recArg1 );
		controlWeapon( recArg2 );
		// Serial.print( "level: " );
		// Serial.println( level );

		// motor3.setSpeed(200);

		// Serial.println("speedLevel");
		// Serial.println(speedLevel);

		// Serial.println("recData.packetArg1");
		// Serial.println(recData.packetArg1);
		// delay(255);

		// Serial.println("\n");
		// delay(255);

		// Serial.println("recLeft");
		// Serial.println(recData.speedmotorLeft);
		// Serial.println("right");
		// Serial.println(recData.speedmotorRight);

		// -------------------------------------------- //
	}
	delay(10);
}
#endif