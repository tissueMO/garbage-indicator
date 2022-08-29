#include <WiFi.h>
#include <HTTPClient.h>
#include "settings.h"
#if TYPE_COUNT == 1
  // 1種類表示タイプ
  #include <M5StickC.h>
#elif TYPE_COUNT == 2
  // 2種類同時表示タイプ
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
#endif

// ネットワーク設定
const char *ssid = SSID;
const char *password = PASSWORD;
const char *host = HOSTNAME;
const int port = PORT;
WiFiClient client;
const int RESPONSE_TIMEOUT_MILLIS = 5000;

void updateLatest();
bool connectWiFi();
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
  M5.Lcd.setRotation(3);
  M5.Lcd.setTextSize(1);
#elif TYPE_COUNT == 2
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
  if (!connectWiFi())
  {
    return;
  }

  M5.Lcd.fillScreen(BLACK);
#if TYPE_COUNT == 2
  M5.Lcd.drawLine(0, 20, 320, 20, WHITE);
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
    if (i > 0)
    {
      M5.Lcd.drawLine(320 / TYPE_COUNT * i, 0, 320 / TYPE_COUNT * i, 240, WHITE);
    }
    M5.Lcd.setTextSize(2);
    M5.Lcd.drawCentreString(types[i][1], 320 / TYPE_COUNT - 320 / TYPE_COUNT / 2 + 320 / TYPE_COUNT * i, 0, 1);
    M5.Lcd.setTextSize(10);
    M5.Lcd.drawCentreString(days, 320 / TYPE_COUNT - 320 / TYPE_COUNT / 2 + 320 / TYPE_COUNT * i, 100, 1);
#endif
  }

  Serial.println("Update Completed.");
}

/**
 * Wi-Fi への接続を行います。
 */
bool connectWiFi()
{
  Serial.print("Connecting to ");
  Serial.println(host);

  if (!client.connect(host, port))
  {
    Serial.println("Connection failed.");
    return false;
  }

  return true;
}

/**
 * 次の収集日までの日数を取得します。
 */
String getGarbageDay(int typeIndex)
{
  HTTPClient httpClient;
  httpClient.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = String("/check/") + types[typeIndex][0];

  if (!httpClient.begin(host, port, url))
  {
    Serial.println("Connection failed.");
    return "???";
  }

  int httpCode = httpClient.GET();
  String days = httpClient.getString();
  httpClient.end();

  return days;
}
