#include <PubSubClient.h>
#include <YunClient.h>

#define SOFT 400
#define MDEIUM 500
#define HARD 900
#define NUM_SENSOR 6
#define MQTT_SERVER "127.0.0.1"
#define MQTT_CLIENTID "you_clinetID"

const IPAddress server(127, 0, 0, 1);
const char topic[] = "Patting_Data";

const int fsrPin0 = A0;
const int fsrPin1 = A1;
const int fsrPin2 = A2;
const int fsrPin3 = A3;
const int fsrPin4 = A4;
const int fsrPin5 = A5;
const int LEDRpin = 11; // PWM
const int LEDGpin = 10; // PWM
const int LEDBpin = 9; // PWM
const int limit = 300;

int fsrData[NUM_SENSOR];
int force_data[NUM_SENSOR][100] = {0};
char json[25];
int cnt = 0, i = 0;
int patNo;
bool trigger;
String msgStr = "";

YunClient yun;
PubSubClient mqtt(MQTT_SERVER, 1883, yun); // MQTT Broker port is 1883

void setup()
{
    pinMode(LEDRpin, OUTPUT);
    pinMode(LEDGpin, OUTPUT);
    pinMode(LEDBpin, OUTPUT);
    Serial.begin(9600);

    Bridge.begin();
    delay(1500); // For NIC Initialization
    if(mqtt.connect(MQTT_CLIENTID)){
        Serial.println("[Connect Success!]");
        mqtt.publish(topic, "Hello World\n"); // out topic
        //mqtt.subscribe("inTopic");
    }
    else Serial.println("[Connect Fail!]");

    for(int a=0; a<NUM_SENSOR; a++)
        for(int b=0; b<100; b++)
            force_data[a][b] = 0;
}
void loop()
{
    initialize(patNo);
    readata();
    trigger = false;
    /* pick patting position */
    for(i=0; i<NUM_SENSOR; i++){
        if(force_data[i][0] > limit){
            patNo = i;
            trigger = true;
            showInfo();
            break;
        }
    }
    int num_input_data = 0;
    for(; trigger = true && force_data[patNo][num_input_data] > limit; ){
        Serial.print(force_data[patNo][num_input_data]);
        Serial.print(" ");
        num_input_data++;
        if(num_input_data >= 15) break;

        switch(patNo){
            case 0:
                force_data[patNo][num_input_data] = analogRead(fsrPin0); break;
            case 1:
                force_data[patNo][num_input_data] = analogRead(fsrPin1); break;
            case 2:
                force_data[patNo][num_input_data] = analogRead(fsrPin2); break;
            case 3:
                force_data[patNo][num_input_data] = analogRead(fsrPin3); break;
            case 4:
                force_data[patNo][num_input_data] = analogRead(fsrPin4); break;
            case 5:
                force_data[patNo][num_input_data] = analogRead(fsrPin5); break;
            default:
                break;
        }
        if(force_data[patNo][num_input_data] > HARD){ // too hard
            set_LED_color(255,0,0); // RED
        }
        else if(force_data[patNo][num_input_data] > MDEIUM){ // standard
            set_LED_color(0,255,0); // GREEN
        }
        else{ // too soft
            set_LED_color(0,0,255); // BLUE
        }
        delay(100);
    }
    mqtt.loop();
    if(num_input_data){
        // Create MQTT message (JSON format)
        msgStr = msgStr + "{\"PatNo\":" + patNo + ",\"force\":";
        for(int j=0; j < num_input_data; j++){
            msgStr = msgStr + force_data[patNo][j] + " ";
        }
        msgStr = msgStr + "}";
        msgStr.toCharArray(json, 25);
        unsigned int len = strlen(json);
        
        if(mqtt.publish(topic, json))
            Serial.print("\nSend Success!");
        else Serial.print("\nSend Fail!");
        msgStr = ""; // clear message
    }
}

void initialize(int patNo){
    for(int b=0; b<100; b++)
        force_data[patNo][b] = 0;
}

void showInfo(){
    Serial.print("\n[No.");
    Serial.print(patNo);
    Serial.print("] ");
    Serial.print(++cnt);
    Serial.print(" th Patting: ");
}

void readata(){
    force_data[0][0] = analogRead(fsrPin0); // 0~1023
    force_data[1][0] = analogRead(fsrPin1);
    force_data[2][0] = analogRead(fsrPin2); 
    force_data[3][0] = analogRead(fsrPin3); 
    force_data[4][0] = analogRead(fsrPin4); 
    force_data[5][0] = analogRead(fsrPin5);
}

void set_LED_color(int R, int G, int B){
    analogWrite(LEDRpin, R); // arg2 range is 0~255
    analogWrite(LEDGpin, G);
    analogWrite(LEDBpin, B);
}
