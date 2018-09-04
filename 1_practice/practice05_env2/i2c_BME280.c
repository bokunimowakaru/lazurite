/*******************************************************************************
本ソースリストは Lazurite IDEに含まれていたBME280用ドライバSSCI_BME280について
以下の改良を行ったものです。

　　・初期化パラメータの省略など、簡単アクセス用APIの追加
　　・不具合修正（キャリブレーションの型誤り!?）

                                                Copyright (c) 2018 Wataru KUNINO
********************************************************************************
元のソースコードには権利表示がありませんでしたが、以下のいずれかに著作権がある
ものと思われます。

　　ラピスセミコンダクタ社（Lazurite IDE・開発元）
　　スイッチサイエンス（Lazurite mini用環境センサ基板・開発元）
*******************************************************************************/

/********************************************************
  BME280 liblary for Lazurite

  https://www.switch-science.com/catalog/2236/
  https://www.switch-science.com/catalog/2323/

*********************************************************/

#include <Wire.h>
#include "i2c_BME280.h"
#define _I2C_BME280_C_
#include <lazurite.h>

static signed long int t_fine;
static int _i2c_addr;
static BME280_calib_data calibData;

static void writeReg(uint8_t reg_address, uint8_t data)
{
  Wire.beginTransmission(_i2c_addr);
  Wire.write_byte(reg_address);
  Wire.write_byte(data);
  Wire.endTransmission(true);
}

static signed long int calibration_T(signed long int adc_T)
{

  signed long int var1, var2, T;
  var1 = ((((adc_T >> 3) - ((signed long int)calibData.dig_T1 << 1))) * ((signed long int)calibData.dig_T2)) >> 11;
  var2 = (((((adc_T >> 4) - ((signed long int)calibData.dig_T1)) * ((adc_T >> 4) - ((signed long int)calibData.dig_T1))) >> 12) * ((signed long int)calibData.dig_T3)) >> 14;

  t_fine = var1 + var2;
  T = (t_fine * 5 + 128) >> 8;
  return T;
}
static unsigned long int calibration_P(signed long int adc_P)
{
  signed long int var1, var2;
  unsigned long int P;
  var1 = (((signed long int)t_fine) >> 1) - (signed long int)64000;
  var2 = (((var1 >> 2) * (var1 >> 2)) >> 11) * ((signed long int)calibData.dig_P6);
  var2 = var2 + ((var1 * ((signed long int)calibData.dig_P5)) << 1);
  var2 = (var2 >> 2) + (((signed long int)calibData.dig_P4) << 16);
  var1 = (((calibData.dig_P3 * (((var1 >> 2) * (var1 >> 2)) >> 13)) >> 3) + ((((signed long int)calibData.dig_P2) * var1) >> 1)) >> 18;
  var1 = ((((32768 + var1)) * ((signed long int)calibData.dig_P1)) >> 15);
  if (var1 == 0)
  {
    return 0;
  }
  P = (((unsigned long int)(((signed long int)1048576) - adc_P) - (var2 >> 12))) * 3125;
  if (P < 0x80000000)
  {
    P = (P << 1) / ((unsigned long int) var1);
  }
  else
  {
    P = (P / (unsigned long int)var1) * 2;
  }
  var1 = (((signed long int)calibData.dig_P9) * ((signed long int)(((P >> 3) * (P >> 3)) >> 13))) >> 12;
  var2 = (((signed long int)(P >> 2)) * ((signed long int)calibData.dig_P8)) >> 13;
  P = (unsigned long int)((signed long int)P + ((var1 + var2 + calibData.dig_P7) >> 4));
  return P;
}

static unsigned long int calibration_H(signed long int adc_H)
{
  signed long int v_x1;

  v_x1 = (t_fine - ((signed long int)76800));
  v_x1 = (((((adc_H << 14) - (((signed long int)calibData.dig_H4) << 20) - (((signed long int)calibData.dig_H5) * v_x1)) +
            ((signed long int)16384)) >> 15) * (((((((v_x1 * ((signed long int)calibData.dig_H6)) >> 10) *
                (((v_x1 * ((signed long int)calibData.dig_H3)) >> 11) + ((signed long int) 32768))) >> 10) + ((signed long int)2097152)) *
                ((signed long int)calibData.dig_H2) + 8192) >> 14));
  v_x1 = (v_x1 - (((((v_x1 >> 15) * (v_x1 >> 15)) >> 7) * ((signed long int)calibData.dig_H1)) >> 4));
  v_x1 = (v_x1 < 0 ? 0 : v_x1);
  v_x1 = (v_x1 > 419430400 ? 419430400 : v_x1);
  return (unsigned long int)(v_x1 >> 12);
}

static void setMode(uint8_t i2c_addr, uint8_t osrs_t, uint8_t osrs_p, uint8_t osrs_h, uint8_t bme280mode, uint8_t t_sb, uint8_t filter, uint8_t spi3w_en) {
//                            0x76,                1,              1,              1,                  3,            0,              0,                0
  uint8_t config_reg    = (t_sb << 5) | (filter << 2) | spi3w_en;			// 0xF5=0b00000000
  uint8_t ctrl_hum_reg  = osrs_h;											// 0xF2=0b00000001
  uint8_t ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | bme280mode;		// 0xF4=0b00100111(0x27)
  _i2c_addr = i2c_addr;
  writeReg(BME280_REG_ctrl_hum, ctrl_hum_reg);			// 0xF2
  writeReg(BME280_REG_ctrl_meas, ctrl_meas_reg);		// 0xF4
  writeReg(BME280_REG_config, config_reg);				// 0xF5
}

static void readTrim()
{
  uint8_t data[33], i = 0;
  Wire.beginTransmission(_i2c_addr);
  Wire.write_byte(BME280_REG_calib00);
  Wire.endTransmission(false);
  Wire.requestFrom(_i2c_addr, 24,true);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }

  Wire.beginTransmission(_i2c_addr);
  Wire.write_byte(BME280_REG_calib25);
  Wire.endTransmission(false);
  Wire.requestFrom(_i2c_addr, 1,true);
  data[i] = Wire.read();
  i++;

  Wire.beginTransmission(_i2c_addr);
  Wire.write_byte(BME280_REG_calib26);
  Wire.endTransmission(false);
  Wire.requestFrom(_i2c_addr, 8,true);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }
  calibData.dig_T1 = ((unsigned short)data[1] << 8) | data[0];
  calibData.dig_T2 = ((signed short)data[3] << 8) | data[2];
  calibData.dig_T3 = ((signed short)data[5] << 8) | data[4];
  calibData.dig_P1 = ((unsigned short)data[7] << 8) | data[6];
  calibData.dig_P2 = ((signed short)data[9] << 8) | data[8];
  calibData.dig_P3 = ((signed short)data[11] << 8) | data[10];
  calibData.dig_P4 = ((signed short)data[13] << 8) | data[12];
  calibData.dig_P5 = ((signed short)data[15] << 8) | data[14];
  calibData.dig_P6 = ((signed short)data[17] << 8) | data[16];
  calibData.dig_P7 = ((signed short)data[19] << 8) | data[18];
  calibData.dig_P8 = ((signed short)data[21] << 8) | data[20];
  calibData.dig_P9 = ((signed short)data[23] << 8) | data[22];
  calibData.dig_H1 = data[24];
  calibData.dig_H2 = ((signed short)data[26] << 8) | data[25];
  calibData.dig_H3 = data[27];
  calibData.dig_H4 = ((signed short)data[28]<< 4) | (0x000F & data[29]);
  calibData.dig_H5 = ((signed short)data[30]<< 4) | ((data[29] >> 4) & 0x000F);
  calibData.dig_H6 = (char)data[31]; 
}

static void readData(double *temp_act, double *press_act, double *hum_act)
{
  int i = 0;
  uint32_t data[8];
  unsigned long int hum_raw, temp_raw, press_raw;

  Wire.beginTransmission(_i2c_addr);
  Wire.write_byte(BME280_REG_press_msb);
  Wire.endTransmission(false);
  Wire.requestFrom(_i2c_addr, 8,true);
  while (Wire.available()) {
    data[i] = Wire.read();
    i++;
  }
  press_raw = (data[0] << 12) | (data[1] << 4) | (data[2] >> 4);
  temp_raw = (data[3] << 12) | (data[4] << 4) | (data[5] >> 4);
  hum_raw  = (data[6] << 8) | data[7];
  *temp_act = (double)calibration_T(temp_raw) / 100.0;
  *press_act = (double)calibration_P(press_raw) / 100.0;
  *hum_act = (double)calibration_H(hum_raw) / 1024.0;
}

static void readOneshot(double *temp_act, double *press_act, double *hum_act)
{
	writeReg(BME280_REG_config, 0x25);
	sleep(1000);
	readData(temp_act, press_act,hum_act);
}

/* 簡易インタフェース */
static void init(){
//	Wire.begin();
    bme280.setMode(0x76,1,1,1,3,0,0,0);
}

static void start(){
    writeReg(BME280_REG_ctrl_meas, 0x27);		// reg 0xF4 = 0x27
}

static void stop(){
    writeReg(BME280_REG_ctrl_meas, 0x00);		// reg 0xF4 = 0x00
}

const I2C_BME280 bme280 = {
	setMode,
	readTrim,
	readData,
	readOneshot,
	init,
	start,
	stop
};
