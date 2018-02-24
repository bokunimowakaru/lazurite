#include "practice02_lcd_ide.h"     // Additional Header

/*******************************************************************************
Practice 02: Lazurite用 LCD 受信機

                                                Copyright (c) 2018 Wataru KUNINO
*******************************************************************************/

#include "lcd_drv_hd44780.h"                    // LCD用ドライバの組み込み
#define CH              36                      // チャンネル24～61
#define BPS             SUBGHZ_100KBPS          // 50K or 100Kbps(32ch 61ch以外)
#define POW             SUBGHZ_PWR_20MW         // 出力 1MW (1mW) or 20MW (20mW)
#define PAN             0xABCD                  // PAN ID
#define TO              0xFFFF                  // 宛先
#define PROMISCUOUS     true                    // PAN IDが一致しなくても受信
#define PIN_LED_BLUE    26                      // 青色LED(動作確認用)
#define PIN_LED_ORANGE  25                      // オレンジ色のLED(エラー表示用)
SUBGHZ_MSG msg;                                 // SubGHz命令の応答メッセージ用

void setup(){
    pinMode(PIN_LED_BLUE,OUTPUT);               // 青色LED用ポートを出力に設定
    pinMode(PIN_LED_ORANGE,OUTPUT);             // オレンジLED用ポートを出力に
    digitalWrite(PIN_LED_BLUE,1);               // 青色LED(動作確認用)を消灯
    digitalWrite(PIN_LED_ORANGE,1);             // オレンジ色LED(エラー用)を消灯
    Serial.begin(115200);                       // 動作確認用シリアルレート設定
    
    /* 起動時にLCDへタイトル、チャンネル、PAN IDを表示する */
    Serial.println("Practice 02 LCD RX");       // 起動表示をシリアルモニタ出力
    lcd_init();                                 // LCDドライバの初期化
    lcd_putstr("SubGHz Snif ");                 // タイトル12字以内   (累計12字)
    lcd_disp_2((uint8_t)CH);                        // チャンネル表示 2字 (累計14字)
    lcd_putstr("Ch");                           // Ch表示 2字         (累計16字)
    lcd_goto_line(2);                           // LCDの2行名へ移動
    if(BPS==SUBGHZ_50KBPS) lcd_putstr(" 50k "); // 速度表示 5字
    else lcd_putstr("100k ");                   // 速度表示 5字        (累計5字)
    if(POW==SUBGHZ_PWR_1MW)lcd_putstr(" 1mW "); // 送信出力 5字
    else lcd_putstr("20mW ");                   // 送信出力 5字       (累計10字)
    lcd_disp_hex((uint8_t)(PAN>>8));            // PAN ID上位2桁表示  (累計12字)
    lcd_disp_hex((uint8_t)(PAN));               // PAN ID下位2桁表示  (累計14字)
    msg=SubGHz.init();                          // 初期化を実行する
    if(!msg){                                   // 初期化エラーが無いとき
        SubGHz.setPromiscuous(PROMISCUOUS);     // プロミスキャスモードの設定
        msg=SubGHz.begin(CH, PAN, BPS, POW);    // 指定した条件で通信を開始する
    }
    if(!msg) msg=SubGHz.rxEnable(NULL);         // エラー無し時に受信を開始する
    if(msg){                                    // エラーが発生した場合に
        digitalWrite(PIN_LED_ORANGE,0);         // エラー表示用LEDを点灯する
        SubGHz.msgOut(msg);                     // エラーメッセージを表示する
        while(1) sleep(1000);                   // 永久に停止する(起動に失敗)
    }
}

void loop(){
    uint8_t rx[65];                             // 受信データ格納用の変数定義
    SUBGHZ_STATUS status;                       // 受信状態保持用の変数定義
    SUBGHZ_MAC_PARAM mac;                       // MACデータ保持用の変数定義
    short rx_len=0;                             // 受信データ長
    
    rx_len = SubGHz.readData(rx,64);            // 受信データを変数へ取得
    if(!rx_len) return;                         // 受信が無い時にloop先頭へ戻る
    digitalWrite(PIN_LED_BLUE,0);               // 青色LEDを点灯
    rx[rx_len]=0;                               // 受信データ末尾を表示用に終端
    lcd_cls();                                  // LCDを消去
    SubGHz.getStatus(NULL,&status);             // 受信状態を取得
    SubGHz.decMac(&mac,rx,rx_len);              // 受信データを解析
    lcd_disp_hex(mac.src_addr[1]);              // 送信アドレス末尾2桁目を表示
    lcd_disp_hex(mac.src_addr[0]);              // 送信アドレス末尾の表示
    lcd_putch(' ');                             // 空白を表示
    lcd_disp_hex((uint8_t)(mac.mac_header.fc16>>8));    // MACヘッダ上位桁を表示
    lcd_disp_hex((uint8_t)(mac.mac_header.fc16));       // MACヘッダ下位桁を表示
    lcd_putch(' ');                             // 空白を表示
    lcd_disp_hex((mac.seq_num));                // 送信シーケンス番号を表示
    lcd_putch(' ');                             // 空白を表示
    lcd_disp_3(status.rssi);                    // RSSI(受信レベル)を表示
    lcd_putch(' ');                             // 空白を表示
    lcd_goto_line(2);                           // LCDカーソル位置を2行目へ移動
    lcd_putstr(mac.payload);                    // 受信データを表示
    digitalWrite(PIN_LED_BLUE, 1);              // 青色LEDを消灯
}
