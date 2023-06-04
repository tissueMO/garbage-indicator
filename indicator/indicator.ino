#include <HTTPClient.h>
#include "settings.h"
#if TYPE_COUNT == 1
  // 1種類表示タイプ
  #include <M5StickC.h>
#elif TYPE_COUNT == 2
  // 2種類同時表示タイプ
  #include <M5Stack.h>
#elif TYPE_COUNT == 3
  // 3種類同時表示タイプ
  #include <M5Stack.h>
#endif

// 機器固有設定
#if TYPE_COUNT == 1
const char *types[TYPE_COUNT][2] = {
  {"recyclable", "Recyclable"}
};
#elif TYPE_COUNT == 2
const char *types[TYPE_COUNT][2] = {
  {"plastic", "Plastic"},
  {"burnable", "Burnable"}
};
#elif TYPE_COUNT == 3
const char *types[TYPE_COUNT][3] = {
  {"plastic", "Plastic"},
  {"burnable", "Burnable"},
  {"recyclable", "Recyclable"}
};
#endif

// ネットワーク設定
const char *ssid = SSID;
const char *password = PASSWORD;
const char *host = HOSTNAME;
const int port = PORT;
const int RESPONSE_TIMEOUT_MILLIS = 5000;

void updateLatest();
String getGarbageDay(int typeIndex);

/*
  初回処理
*/
void setup()
{
  // 初期化
  M5.begin();

  // LCD スクリーン初期化
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
#if TYPE_COUNT == 1
  M5.Axp.ScreenBreath(9);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(1);
#elif TYPE_COUNT >= 2
  M5.Lcd.setBrightness(24);
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(2);
#endif
  M5.Lcd.println("Garbage Indicator");

  // Wi-Fi 接続
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    // 接続待ち
    delay(500);
    Serial.print(".");
  }

  // 接続成功後
  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.print("IP address: ");
  M5.Lcd.println(WiFi.localIP());
  delay(3000);

  // 最新の状態を取得
  updateLatest();
  delay(10 * 60 * 1000);
}

/*
  メインループ
*/
void loop()
{
  M5.update();

  // 最新の状態を取得
  updateLatest();

  delay(10 * 60 * 1000);
}

/**
 * サーバーから自機の最新の状態を取得します。
 */
void updateLatest()
{
  M5.Lcd.fillScreen(BLACK);
#if TYPE_COUNT == 2
  M5.Lcd.drawLine(0, 20, 320, 20, WHITE);
  M5.Lcd.drawLine(320 / 2, 0, 320 / 2, 240, WHITE);
#elif TYPE_COUNT == 3
  M5.Lcd.drawLine(0, 20, 320, 20, WHITE);
  M5.Lcd.drawLine(0, 240 / 2 + 20, 320, 240 / 2 + 20, WHITE);
  M5.Lcd.drawLine(320 / 2, 0, 320 / 2, 240 / 2 + 20, WHITE);
#endif

  // サーバーから最新情報を取得
  for (int i = 0; i < TYPE_COUNT; i++)
  {
    String days = getGarbageDay(i);

    Serial.print("[");
    Serial.print(i);
    Serial.print("] ");
    Serial.print(types[i][1]);
    Serial.print("=");
    Serial.println(days);

#if TYPE_COUNT == 1
    M5.Lcd.drawLine(0, 80 / 2 + 10 / 2, 160 / 2, 80 / 2 + 10 / 2, WHITE);

    M5.Lcd.setTextSize(1);
    M5.Lcd.drawCentreString(types[i][1], 160 / 2 - 160 / 4, 80 / 2 - 10 / 2, 1);

    M5.Lcd.setTextSize(10);
    M5.Lcd.drawCentreString(days, 160 / 2 + 160 / 4, 80 / 2 - 80 / 4 - 2, 1);

#elif TYPE_COUNT == 2
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawCentreString(types[i][1], 320 / 2 - 320 / 2 / 2 + 320 / 2 * i, 0, 1);

    M5.Lcd.setTextSize(10);
    M5.Lcd.drawCentreString(days, 320 / 2 - 320 / 2 / 2 + 320 / 2 * i, 100, 1);

#elif TYPE_COUNT == 3
    if (i < 2)
    {
      M5.Lcd.setTextSize(2);
      M5.Lcd.drawCentreString(types[i][1], 320 / 2 - 320 / 2 / 2 + 320 / 2 * i, 0, 1);

      M5.Lcd.setTextSize(10);
      M5.Lcd.drawCentreString(days, 320 / 2 - 320 / 2 / 2 + 320 / 2 * i, 60, 1);
    }
    else
    {
      M5.Lcd.drawLine(0, 240 / 2 + 240 / 4 + 20, 320 / 2, 240 / 2 + 240 / 4 + 20, WHITE);

      M5.Lcd.setTextSize(2);
      M5.Lcd.drawCentreString(types[i][1], 320 / 2 - 320 / 4, 240 / 2 + 240 / 4, 1);

      M5.Lcd.setTextSize(10);
      M5.Lcd.drawCentreString(days, 320 / 2 + 320 / 4, 240 / 2 + 240 / 4 - 10, 1);
    }

#endif
  }

  Serial.println("Update Completed.");
}

/**
 * 次の収集日までの日数を取得します。
 */
String getGarbageDay(int typeIndex)
{
  HTTPClient client;
  client.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = "https://" + String(host) + ":" + String(port) + "/garbage/check/" + types[typeIndex][0];

  if (!client.begin(url))
  {
    Serial.println("Connection failed.");
    return "???";
  }

  int httpCode = client.GET();
  String days = client.getString();
  client.end();

  return days;
}
