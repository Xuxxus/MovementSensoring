#include<Wire.h>
#include <string.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>

//NESTE PROGRAMA, ESTOU USANDO 2 MPUS
//O CÓDIGO VALIDA O FUNCIONAMENTO DO PINO AD0

//PARA ADICIONAR MAIS SENSORES, MUDE O VALOR DE n, CONECTE NA ORDEM OS PINOS AD0 E MANDE BALA!

/*
 * Connect the SD card to the following pins:
 *
 * SD Card | ESP32
 *    D5       SS
 *    D23      MOSI
 *    VSS      GND
 *    VDD      3.3V
 *    D18      SCK
 *    D19      MISO
 */

const uint8_t n = 1; //número de MPUs sendo utilizado MAXIMO 13

//NUMERO DO PINO DO ESP32 PARA CONECTAR CADA AD0

//const uint8_t AD0_MPU[] = {15,  4, 16, 17, 3, 1, 34, 25, 32, 33, 25, 26, 27}; 
const uint8_t AD0_MPU[] = {17,16, 17,  4, 34, 32, 33, 25, 26,3,1,27}; 
//const uint8_t AD0_MPU[] = {4, 16, 17, 3, 1, 34, 25, 32, 33, 25, 26}; 

float AngleRoll,AnglePitch,AngleYaw;
int valor;

double Vector_data[n][7];
unsigned long tempoAnterior =0;
unsigned long LastTempo =0;
//uint8_t T =4000; // T está em ms, ou seja, a cada T ms eu desejo fazer 1 ciclo
int T=15;

int16_t AcX, AcY, AcZ, Tmp, GyX, GyY, GyZ;
const int MPU_ADDR = 0x69; // I2C address of the MPU-6050

char path[100]; //variavel para adicionar nome do txt

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

void select_MPU(uint8_t mpu){

  for (uint8_t i = 0; i < n; i++){
    digitalWrite(AD0_MPU[i], LOW);
    delay(10);
  }
  digitalWrite(AD0_MPU[mpu], HIGH);
  delay(10);
}
//const uint8_t AD0_MPU[] = {16, 17, 1, 2, 34, 25, 32, 33, 25, 26}; 
//foi criado uma variante do select_MPU1 pois nao faz sentido ter um loop para deixar tudo em low se ha a funçaõ diselect fazendo isso toda vez
void select_MPU1(uint8_t mpu){
  if(mpu == 0){
    if(n>1){
      //Serial.print("Entrou n>1 porta:");
      //Serial.println(AD0_MPU[mpu]);
      
      digitalWrite(AD0_MPU[n-1], LOW);   
       
      digitalWrite(AD0_MPU[mpu], HIGH);
      delay(1);
    }
    else{     
      //Serial.println("Entrou n = 0");
      digitalWrite(AD0_MPU[mpu], HIGH);
      delay(1);
    }
  }
  else{
    //Serial.print("Entrou mpu dif 0 porta: ");
    //Serial.println(AD0_MPU[mpu]);
    //delay(300);
    digitalWrite(AD0_MPU[mpu-1], LOW);  
    //delay(40);   
    digitalWrite(AD0_MPU[mpu], HIGH);
    delay(1);
  }
  
}

void diselect_MPU(){

  for (uint8_t i = 0; i < n; i++){
    digitalWrite(AD0_MPU[i], LOW);
    delay(10);
  }
}


void writeFile(fs::FS &fs, const char * path, const char * message){
    ////Serial.printf("Writing file: %s\n", path);

    File file = fs.open(path, FILE_WRITE);
    if(!file){
        //Serial.println("Failed to open file for writing");
        return;
    }
    if(file.print(message)){
        ////Serial.println("File written");
    } else {
        //Serial.println("Write failed");
    }
    file.close();
}

bool readFile(fs::FS &fs, const char * path){
    //Serial.printf("Reading file: %s\n", path);

    File file = fs.open(path);
    if(!file){
        ////Serial.println("Failed to open file for reading");
        return false;
    }

    /*//Serial.print("Read from file: ");
    while(file.available()){
        //Serial.write(file.read());
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
  
  Serial.begin(115200);
  
  Wire.begin(); // sda, scl
  Wire.setClock(200000); // escolho o valor da velocidade de comunicação em Hz do I2C
  delay(250);

  SPI.begin();
  SPI.setFrequency(1000000);
  

  //inicializando cada MPU por vez // tentei usar BROADCASTING E N FUNFOU, tentar novamente um dia
  for (uint8_t i = 0; i < n; i++){
      select_MPU(i);
      setup_MPU();
  }
  for (uint8_t i = 0; i < n; i++){
      diselect_MPU();
  }
  

  ////Serial.println("Todo MPU configurado!");

  if(!SD.begin()){
    ////Serial.println("Card Mount Failed");
    return;
  }

  uint8_t cardType = SD.cardType();

  if(cardType == CARD_NONE){
      ////Serial.println("No SD card attached");
      return;
  }

  for (uint8_t i = 0; i < 255; i++){
    sprintf(path, "/euler%d.txt", i); 
    if (!readFile(SD,path))
      break;
  }
  ////Serial.println(path);
  
  writeFile(SD, path, "Starting the program!\n");
  tempoAnterior = millis();
  Serial.print("Comecou ");
}

void loop() {
  
  char lineToAppend[5000]; // string que armazena valor de um ciclo
  //Serial.print("Tempo do if:");
  //Serial.println(millis() - LastTempo);
  if(millis() - LastTempo >= T){ //para T = 0.05 -> tempoAtual [ms] - tempoAnterior [ms] >= 0.05 ms -> deve fazer o ciclo a cada 0.05ms
    LastTempo = millis();
    for (uint8_t i = 0; i<n;i++){
      ////Serial.print("MPU ");//Serial.print(i);//Serial.println(" :");
      select_MPU1(i);
      data(i);
    }
    //digitalWrite(4, LOW);//verificar
    for (uint8_t i = 0; i < n; i++){
      char txt[100]; 
      for (uint8_t j = 0; j < 6; j++){
        gcvt(Vector_data[i][j], 6, txt);
        if((i==0)&&(j==0)){
          strcpy(lineToAppend, txt);
        }
        else
          strcat(lineToAppend, txt);
        strcat(lineToAppend, ",");
        //appendFile(SD, path, txt);
        //appendFile(SD, path, ",");
      } 
      gcvt(Vector_data[i][6], 6, txt);
      strcat(lineToAppend, txt);
      strcat(lineToAppend, ",");
      //appendFile(SD, path, txt);
      //appendFile(SD, path, "\n");
    }
    //strcat(lineToAppend, ",");
    //appendFile(SD, path, ";");
    appendFile(SD, path, lineToAppend);
    //Serial.println(lineToAppend);
  }
  
}


void data(uint8_t mpu_number){
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);  // starting with register 0x3B (ACCEL_XOUT_H)
  Wire.endTransmission(false);
  Wire.beginTransmission(MPU_ADDR);
  Wire.requestFrom(MPU_ADDR, 14, true); // request a total of 14 registers
  AcX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  AcY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  AcZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)
  Tmp = Wire.read() << 8 | Wire.read(); // 0x41 (TEMP_OUT_H)   & 0x42 (TEMP_OUT_L)
  GyX = Wire.read() << 8 | Wire.read(); // 0x3B (ACCEL_XOUT_H) & 0x3C (ACCEL_XOUT_L)
  GyY = Wire.read() << 8 | Wire.read(); // 0x3D (ACCEL_YOUT_H) & 0x3E (ACCEL_YOUT_L)
  GyZ = Wire.read() << 8 | Wire.read(); // 0x3F (ACCEL_ZOUT_H) & 0x40 (ACCEL_ZOUT_L)

  Vector_data[mpu_number][0] = double(AcX)/16384;
  Vector_data[mpu_number][1] = double(AcY)/16384;
  Vector_data[mpu_number][2] = double(AcZ)/16384;

  Vector_data[mpu_number][3] = double(GyX)/65.5;
  Vector_data[mpu_number][4] = double(GyY)/65.5;
  Vector_data[mpu_number][5] = double(GyZ)/65.5;
  
  Vector_data[mpu_number][6] = (double(millis()-tempoAnterior))/1000;
  
 }
