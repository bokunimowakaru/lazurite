#include "practice05_env2_ide.h"        // Additional Header

/*******************************************************************************
Practice 05: Lazurite用  超低消費電力 温度・湿度・気圧＋照度 送信機

                                                Copyright (c) 2018 Wataru KUNINO
*******************************************************************************/

#include "i2c_BME280.h"
#define CH              36                      // チャンネル24～61
#define BPS             SUBGHZ_100KBPS          // 50K or 100Kbps(32ch 61ch以外)
#define POW             SUBGHZ_PWR_20MW         // 出力 1MW (1mW) or 20MW (20mW)
#define PAN             0xABCD                  // PAN ID
#define TO              0xFFFF                  // 宛先
#define DEVICE          "envir_0,"              // 送信電文内へ付与する機器名
#define PIN_LED_BLUE    26                      // 青色LED(動作確認用)
#define PIN_LED_ORANGE  25                      // オレンジ色のLED(エラー表示用)
SUBGHZ_MSG msg;                                 // SubGHz命令の応答メッセージ用

unsigned int getBatt(){                         // 電池電圧を取得する関数を定義
    int i,level;
    unsigned int table[13]
        ={1898,2000,2093,2196,2309,2409,2605,2800,3068,3394,3797,4226,4667};
    level = voltage_check(VLS_4_667);           // 電圧を取得
    if(level >= 15) return 4667;                // レベル15のとき4667mV以上
    if(level <= 2) return 1898;                 // レベル2のとき1898mV以下
    return (table[level-2] + table[level-3])/2; // その他のときtableから判定
}

void setup(){
    pinMode(PIN_LED_BLUE,OUTPUT);               // 青色LED用ポートを出力に設定
    pinMode(PIN_LED_ORANGE,OUTPUT);             // オレンジLED用ポートを出力に
    digitalWrite(PIN_LED_BLUE,1);               // 青色LED(動作確認用)を消灯
    digitalWrite(PIN_LED_ORANGE,1);             // オレンジ色LED(エラー用)を消灯
    Serial.begin(115200);                       // 動作確認用シリアルレート設定
    if( (msg=SubGHz.init()) != 0){              // 初期化時にエラーが発生した時
        digitalWrite(PIN_LED_ORANGE,0);         // エラー表示用LEDを点灯する
        SubGHz.msgOut(msg);                     // エラーメッセージを表示する
        while(1) sleep(1000);                   // 永久に停止する(起動に失敗)
    } 
    Wire.begin();                               // I2Cインタフェースを開始
    Serial.println("Practice 05 ENV");          // 起動表示をシリアルモニタ出力
    bme280.init();                              // BME280の初期設定
    bme280.readTrim();                          // BME280のデータ補正の実行
}

void loop(){
    char tx[65];                                // 送信用の文字配列変数txを定義
    double temp,press,hum;                      // 温度、気圧、湿度用の変数定義
    float lum;                                  // 照度用の変数定義
    
    bme280.start();                             // BME280の動作開始
    bme280.readData(&temp,&press,&hum);         // 温度、気圧、湿度を取得
    bme280.stop();                              // BME280の動作停止
    rpr0521rs.init();
    rpr0521rs.get_oneShot(0,&lum);              // 照度センサから照度値を取得
    Print.init(tx,64);                          // 変数txの初期化
    Print.p(DEVICE);                            // 変数txへデバイス名を代入
    Print.f(temp,0);                            // 変数txへ温度値を追加
    Print.p(",");                               // カンマを追加
    Print.f(hum,0);                             // 変数txへ湿度値を追加
    Print.p(",");                               // カンマを追加
    Print.f(press,0);                           // 変数txへ気圧値を追加
    Print.p(",");                               // カンマを追加
    Print.f(lum,0);                             // 変数txへ気圧値を追加
    Print.p(",");                               // カンマを追加
    Print.l(getBatt(),DEC);                     // 変数txへ電池電圧を追加
    Print.ln();                                 // 変数txへ改行を追加
    Serial.print(tx);                           // 送信内容をシリアルモニタ出力
    
    digitalWrite(PIN_LED_BLUE,0);               // 動作確認用LEDを点灯
    SubGHz.begin(CH, PAN, BPS, POW);            // 指定した条件で通信を開始する
    msg=SubGHz.send(PAN,TO,tx,sizeof(tx),NULL); // 変数txのデータを送信する
    if(msg){                                    // 応答値がSUBGHZ_OK以外の時
        Serial.print("ERROR ");                 // エラー表示
        SubGHz.msgOut(msg);                     // 応答内容を表示
        digitalWrite(PIN_LED_ORANGE,0);         // エラー表示用LEDを点灯する
    }else digitalWrite(PIN_LED_ORANGE,1);       // エラー表示用LEDを消灯する
    SubGHz.close();                             // 通信を切断する
    digitalWrite(PIN_LED_BLUE,1);               // 動作確認用LEDを消灯
    sleep(59000);                               // 59秒間、低消費電力状態で待機
}
