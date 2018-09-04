/*******************************************************************************
本ソースリストは Lazurite IDEに含まれていた Switch Science 用ドライバについて
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
#ifndef _I2C_BME280_H_
#define _I2C_BME280_H_

#include "lazurite.h"

// I2C Address
//#define   BME280_ADDRESS  0x76

// BME280 Registers
#define   BME280_REG_calib00    0x88
#define   BME280_REG_calib25    0xa1
#define   BME280_REG_ID         0xd0
#define   BME280_REG_reset      0xe0
#define   BME280_REG_calib26    0xe1
#define   BME280_REG_ctrl_hum   0xf2
#define   BME280_REG_status     0xf3
#define   BME280_REG_ctrl_meas  0xf4
#define   BME280_REG_config     0xf5
#define   BME280_REG_press_msb  0xf7
#define   BME280_REG_press_lsb  0xf8
#define   BME280_REG_press_xlsb 0xf9
#define   BME280_REG_temp_msb   0xfa
#define   BME280_REG_temp_lsb   0xfb
#define   BME280_REG_temp_xlsb  0xfc
#define   BME280_REG_hum_msb    0xfd
#define   BME280_REG_hum_lsb    0xfe

// Caribration data storage
typedef struct {
  uint16_t  dig_T1;
  int16_t   dig_T2;
  int16_t   dig_T3;
  uint16_t  dig_P1;
  int16_t   dig_P2;
  int16_t   dig_P3;
  int16_t   dig_P4;
  int16_t   dig_P5;
  int16_t   dig_P6;
  int16_t   dig_P7;
  int16_t   dig_P8;
  int16_t   dig_P9;
  uint8_t   dig_H1; // 不具合修正
  int16_t   dig_H2;
  uint8_t   dig_H3; // 不具合修正
  int16_t   dig_H4;
  int16_t   dig_H5;
  int8_t    dig_H6;
} BME280_calib_data;


typedef struct  {
	void (*setMode)(
      uint8_t i2c_addr,         //I2C Address
      uint8_t osrs_t,           //Temperature oversampling
      uint8_t osrs_p,           //Pressure oversampling
      uint8_t osrs_h,           //Humidity oversampling
      uint8_t bme280mode,       //Mode Sleep/Forced/Normal
      uint8_t t_sb,             //Tstandby
      uint8_t filter,           //Filter off
      uint8_t spi3w_en          //3-wire SPI Enable/Disable
    );
	void (*readTrim)(void);
	void (*readData)(double *temp_act, double *press_act, double *hum_act);
	void (*readOneshot)(double *temp_act, double *press_act, double *hum_act);
	void (*init)(void);
	void (*start)(void);
	void (*stop)(void);
} I2C_BME280;

extern const I2C_BME280 bme280;

#ifndef _I2C_BME280_C_
#define _I2C_BME280_C_
#include "i2c_BME280.c"
#endif // _I2C_BME280_C_

#endif // _I2C_BME280_H_
