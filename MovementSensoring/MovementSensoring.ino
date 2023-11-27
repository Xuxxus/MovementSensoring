
#include <Wire.h>
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <stdio.h>


//PARA ADICIONAR MAIS SENSORES, MUDE O VALOR DE n, CONECTE NA ORDEM OS PINOS AD0 E MANDE BALA!

/*

NEW SKETCH
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D5       SS
 *    D23      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    D18      SCK
 *    D19      MISO
NEW SKETCH

 */


TaskHandle_t Task1;

const uint8_t n = 5; //number of IMUs conected (Max of 13)

//Follow this pin numbers to conect your IMUs
const uint8_t AD0_MPU[] = {15,  2, 4, 16, 17, 3, 1, 13,  32, 33, 25, 26, 27}; 
const uint16_t history_size = 600/n; //if the code doesn't compile, consider making this number smaller
const uint8_t T = 0; //

//Buttons and LEDs connections
const uint8_t  LED_R  = 12;
const uint8_t  LED_G  = 13;
const uint8_t  SWITCH = 35;

volatile uint32_t index_data = 0, index_SDCard = 0;
volatile uint64_t contDATA = 0, contSD = 0;
volatile double Vector_data[n][7][history_size];
volatile unsigned long initialTime =0, LastRead = 0;


volatile uint8_t stopFlag = false;
volatile int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
const int MPU_ADDR = 0x69; // I2C address of the MPU-6050

char path[100]; //variable to hold the name of the txt file on the sd card
char lineToAppend[500]; // string que armazena valor de um ciclo

void appendFile(fs::FS &fs, const char * path, const char * message){
    ////Serial.printf("Appending to file: %s\n", path);
    ////Serial.print("Appending to file:");
    ////Serial.print(path);

    File file = fs.open(path, FILE_APPEND);
    if(!file){
        //Serial.println("Failed to open file for appending");
        return;
    }
    if(file.print(message)){
        //Serial.println("Message appended");
    } else {
        //Serial.println("Append failed");
    }
    file.close();
}


void setup_MPU(){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);  // PWR_MGMT_1 register
  Wire.write(0X01);     // set to zero (wakes up the MPU-6050)
  Wire.endTransmission(false);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x19); //sample rate
  Wire.write(0x00); // parametro que desejo 
  Wire.endTransmission(false);


  //Escolhendo valor full scale dos acelerômetros
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C); //Acessando registrador 
  Wire.write(0x00); //Escolhendo full scale como 2g 
  Wire.endTransmission(false);

  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); // registrador 27 ou 1B -> configura a escala de medição do giroscópio
  Wire.write(0x08); //8 é parametro em hexadecimal da tabela de configuração da escala +-500º/s
  Wire.endTransmission(false);

  Wire.beginTransmission(MPU_ADDR); // inicia a comunicação
  //i2C com o giroscopio do endereço MPU_ADDRESS (ou 1101000 em binario)
  Wire.write(0x1A); //registrador 26 ou 1A -> configura o filtro passa baixa
  Wire.write(0x05); // 05 é o número da tabela de configuração do filtro = 10Hz
  Wire.endTransmission();// termina a comunicação   
}

//foi criado uma variante do select_MPU pois nao faz sentido ter um loop para deixar tudo em low se ha a funçaõ diselect fazendo isso toda vez
void select_MPU(uint8_t mpu){
  if(mpu == 0){
    digitalWrite(AD0_MPU[n-1], LOW);    
    digitalWrite(AD0_MPU[mpu], HIGH);
    //delay(0.02);
  }
  else{
    //Serial.print("Entrou mpu dif 0 porta: ");
    //Serial.println(AD0_MPU[mpu]);
    //delay(300);
    digitalWrite(AD0_MPU[mpu-1], LOW);  
    //delay(40);   
    digitalWrite(AD0_MPU[mpu], HIGH);
  }
  
}

void diselect_MPU(){

  for (uint8_t i = 0; i < n; i++){
    digitalWrite(AD0_MPU[i], LOW);
    delay(10);
  }
}


void writeFile(fs::FS &fs, const char * path, const char * message){

    //Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
       // Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
       // Serial.println("File written");
    } else {
       // Serial.println("Write failed");
    }
    file.close();
}

bool readFile(fs::FS &fs, const char * path){

   // Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
      //  Serial.println("Failed to open file for reading");
        return false;
    }

    /*Serial.print("Read from file: ");
    while(file.available()){
        Serial.write(file.read());

    }*/
    file.close();

    return true;
}

void setup() {
  
  // colocando todos os pinos AD0 em output
  for (uint8_t i = 0; i < n; i++){ 
    pinMode(AD0_MPU[i], OUTPUT);
    digitalWrite(AD0_MPU[i], LOW);
  }

  // colocando pinos de LED em output
  pinMode(LED_G, OUTPUT);
  digitalWrite(LED_G, LOW);
  pinMode(LED_R, OUTPUT);
  digitalWrite(LED_R, LOW);

  //Writing switch button as input
  pinMode(SWITCH, INPUT);

  Serial.begin(115200);
  
  Wire.begin();
  Wire.setClock(400000); // escolho o valor da velocidade de comunicação em Hz do I2C

  //Initializing each MPU
  for (uint8_t i = 0; i < n; i++){
      select_MPU(i);
      setup_MPU();
  }
  diselect_MPU();
  

  Serial.println("Todo MPU configurado!");
  pinMode(5, OUTPUT);
  if(!SD.begin(5)){
    Serial.println("Card Mount Failed");
    //return;
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      Serial.println("No SD card attached");
      //return;
  }

  for (uint8_t i = 0; i < 255; i++){
    sprintf(path, "/euler%d.txt", i); 
    if (!readFile(SD,path))
      break;
  }
  Serial.println(path);
  while(1){ //Waiting button to be pressed
    delay(100);
    if(digitalRead(SWITCH) == HIGH){ //debugging
      delay(100);
      if(digitalRead(SWITCH)==HIGH)
        break;
    }
  }
  Serial.println("BOTAO PRESSIONADO");
  xTaskCreatePinnedToCore(
                    Task1code,   /* Task function. */
                    "Task1",     /* name of task. */
                    90000,       /* Stack size of task */
                    NULL,        /* parameter of the task */
                    1,           /* priority of the task */
                    &Task1,      /* Task handle to keep track of created task */
                    0);          /* pin task to core 0 */    
  
  writeFile(SD, path, "Starting the program!\n");
  initialTime = millis();
  //Green Light as started Recording
  digitalWrite(LED_G, HIGH);
}


//LOOP FUNCTION RUNNING ON CORE 0
//THIS FUNCTION READS THE SENSOR BASED ON PERIOD T
void loop() {
  //To T = 0.05 -> tempoAtual [ms] - LastRead [ms] >= 50 ms -> Must do the cicle every 50 miliseconds
  if((millis() - LastRead >= T) && (!stopFlag)){ //para T = 0.05 -> tempoAtual [ms] - LastRead [ms] >= 50 ms -> deve fazer o ciclo a cada 50ms
   // Serial.print("Task1 running on core ");Serial.println(xPortGetCoreID());
    LastRead = millis();
    for (uint8_t i = 0; i<n;i++){
     // Serial.print("for ");Serial.println(i);
      select_MPU(i);
      Serial.print("select ");Serial.println(i);
      data(i);
    }
    (index_data >= history_size-1)? index_data = 0: index_data++; //Check index overflow
    digitalWrite(4, LOW);//verificar
    contDATA++; //adding to number of data acquired

  }

  else if (stopFlag)
    vTaskDelay(1000); //enters dummy mode

}

//TASK1CODE RUNNING ON CORE 1
//THIS FUNCTION WRITES THE DATA ON SD CARD ONCE THEY ARE READY
void Task1code( void * pvParameters ){
  //Serial.print("Task1 running on core ");Serial.println(xPortGetCoreID());
  while(1){
    //Serial.print("Task1 running on core ");Serial.println(xPortGetCoreID());

    //Checking if button was pressed
    if((digitalRead(SWITCH)==HIGH) && (millis()-initialTime > 5000)){ //debugging
      delay(50);
      if(digitalRead(SWITCH)==HIGH){
        delay(50);
        if(digitalRead(SWITCH)==HIGH){
          delay(100);
          if(digitalRead(SWITCH)==HIGH){
            digitalWrite(LED_R, HIGH); //Indicating end of the program
            digitalWrite(LED_G, HIGH); //Indicating end of the program
            stopFlag = true; //ending program
          }
        }
      }
    }

    if ((stopFlag) && (contSD == contDATA)){
      digitalWrite(LED_G, LOW); //Indicating that you can take the SD card out
    }

    if(contSD<contDATA){ //IF THERE'S DATA TO ANALIZE
      Serial.print("DataLeft");
      Serial.println(contDATA-contSD);
      char data[500]; //STRING TO WRITE ON SD CARD
      for (uint8_t i = 0; i < n; i++){
        char txt[100]; //STRING TO CONVERT DATA ON STRING
        for (uint8_t j = 0; j < 6; j++){ 
          gcvt(Vector_data[i][j][index_SDCard], 6, txt);
          //dtostrf(Vector_data[i][j][index_SDCard], 4, 4, txt); //DATA TO STRING
          if((i==0)&&(j==0)) //IF FIRST TIME, DATA = TXT
            strcpy(data, txt);
          else
            strcat(data, txt);//IF NOT FIRST TIME, DATA += TXT

          strcat(data, ",");//COMMA TO SEPARATE EACH VALUE
        }
        gcvt(Vector_data[i][6][index_SDCard], 6, txt);
        //dtostrf(Vector_data[i][6][index_SDCard], 4, 4, txt); //TIME TO ADD TO EACH SENSOR
        strcat(data, txt);
        strcat(data, ",");
      }
      //strcat(data, ";"); // ; TO INDICATE END OF THIS READDING
      Serial.print("New data:");Serial.println(data);
      appendFile(SD, path, data);//APPENDING TO SD CARD
      (index_SDCard >= history_size-1)? index_SDCard = 0: index_SDCard++; //RE-STARTING INDEX
      contSD++;//MORE DATA SAVED
    } 
    else
      vTaskDelay(1); //So this doesnt die on WWDT
  } 
}

void data(uint8_t mpu_number){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.beginTransmission(MPU_ADDR);
  //IF MPU IS NOT CONNECTED, IT MIGHT NOT COME BACK FORM REQUEST FUNCTION BELOW
  //PUTTING GREEN LOW AND RED HIGH TO INDICATE ERROR
  digitalWrite(LED_G, LOW);
  digitalWrite(LED_R, HIGH);
  Wire.requestFrom(MPU_ADDR, 14, true); //IF IT DOESNT COME BACK FROM THIS, IT CAN BE AN CONNECTION ERROR
  //CAME BACK FROM FUNCTION, THEN TURN LED R ON
  digitalWrite(LED_G, HIGH);
  digitalWrite(LED_R, LOW);
  //TAKING DATA FROM MPU
  AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H)   & 0x42 (TEMP_OUT_L)
  GyX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  GyY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  GyZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)

  //SAVING DATA IN VECTORS
  Vector_data[mpu_number][0][index_data] = double(AcX)/16384;
  Vector_data[mpu_number][1][index_data] = double(AcY)/16384;
  Vector_data[mpu_number][2][index_data] = double(AcZ)/16384;

  Vector_data[mpu_number][3][index_data] = double(GyX)/65.5;
  Vector_data[mpu_number][4][index_data] = double(GyY)/65.5;
  Vector_data[mpu_number][5][index_data] = double(GyZ)/65.5;
  //SAVING TIME IN VECTOR
  Vector_data[mpu_number][6][index_data] = double(millis()-initialTime)/1000;
  Serial.print("Dado lido: "); Serial.println(Vector_data[mpu_number][0][index_data]);
}
