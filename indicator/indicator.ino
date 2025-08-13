#include <HTTPClient.h>
#include <M5StickC.h>
#include <WiFi.h>
#include "settings.h"
#include "resources.h"

// ネットワーク設定
const char *ssid = SSID;
const char *password = PASSWORD;
const char *host = HOSTNAME;
const int port = PORT;
const int RESPONSE_TIMEOUT_MILLIS = 5000;

// ゴミ種別
typedef struct {
  const char* name;
  const unsigned short* icon;
} GarbageType;

const int TYPE_COUNT = 3;
GarbageType types[TYPE_COUNT] = {
  {"plastic", PLASTIC_ICON},
  {"burnable", BURNABLE_ICON},
  {"recyclable", RECYCABLE_ICON}
};

// 描画
TFT_eSprite canvas = TFT_eSprite(&M5.Lcd);

// 動作設定
const int FONT_NUMBER = 7;
const int INTERVAL = 10 * 60 * 1000;

// 関数
void updateLatest();
int getGarbageDay(int typeIndex);

/**
 * 初回処理
 */
void setup()
{
  // 初期化
  M5.begin();

  // LCD スクリーン初期化
  M5.Lcd.fillScreen(BLACK);
  M5.Lcd.setTextColor(WHITE);
  M5.Axp.ScreenBreath(48);
  M5.Lcd.setRotation(1);
  M5.Lcd.setTextSize(1);
  M5.Lcd.println("Garbage Indicator");

  // ダブルバッファ用スプライト作成
  canvas.createSprite(M5.Lcd.width(), M5.Lcd.height());
  canvas.setSwapBytes(true);

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

  Serial.println();
  Serial.println("Wi-Fi connected.");
  Serial.print("IP address: ");
  M5.Lcd.print("IP address: ");
  Serial.println(WiFi.localIP());
  M5.Lcd.println(WiFi.localIP());

  // 時刻同期
  configTzTime("JST-9", "ntp.nict.jp");
  Serial.println("NTP OK.");
  M5.Lcd.println("NTP OK.");

  delay(3000);

  // 最新の状態を取得
  M5.Lcd.setTextFont(FONT_NUMBER);
  M5.Lcd.setTextSize(7);
  updateLatest();

  delay(INTERVAL);
}

/**
 * メインループ
 */
void loop()
{
  M5.update();
  updateLatest();
  delay(INTERVAL);
}

/**
 * サーバーから自機の最新の状態を取得します。
 */
void updateLatest()
{
  M5.Lcd.startWrite();
  canvas.fillSprite(BLACK);

  // 現在時刻を取得
  struct tm currentTime;
  if (!getLocalTime(&currentTime)) {
    Serial.println("CurrentTime: (Failed)");
    canvas.drawCentreString("---", 160 / 2, 16, FONT_NUMBER);
    canvas.pushSprite(0, 0);
    M5.Lcd.endWrite();
    return;
  }
  Serial.printf("CurrentTime: %04d/%02d/%02d %02d:%02d\n", currentTime.tm_year + 1900, currentTime.tm_mon + 1, currentTime.tm_mday, currentTime.tm_hour, currentTime.tm_min);

  // 次の収集ゴミの種別を取得
  int hour = currentTime.tm_hour;
  int targetDays = (0 <= hour && hour < 9) ? 0 : 1;
  GarbageType* selectedType = nullptr;

  for (int i = 0; i < TYPE_COUNT; i++) {
    int days = getGarbageDay(i);

    Serial.print("[");
    Serial.print(i);
    Serial.print("] ");
    Serial.print(types[i].name);
    Serial.print("=");
    Serial.println(days);

    if (selectedType == nullptr && days == targetDays) {
      selectedType = &types[i];
    }
  }

  Serial.print("TargetDays=");
  Serial.println(targetDays);

  // ゴミ種別に従ってアイコンを描画
  if (selectedType != nullptr) {
    canvas.pushImage(8, 0, 80, 80, selectedType->icon);
    canvas.drawCentreString(String(targetDays), 160 / 2 + 160 / 4 + 6, 18, FONT_NUMBER);
    Serial.print("Draw: [");
    Serial.print(selectedType->name);
    Serial.println("]");
  } else {
    canvas.drawCentreString("---", 160 / 2, 16, FONT_NUMBER);
    Serial.println("Draw: (No garbage collection)");
  }

  canvas.pushSprite(0, 0);
  M5.Lcd.endWrite();

  Serial.println("Update Completed.");
}

/**
 * 次の収集日までの日数を取得します。
 */
int getGarbageDay(int typeIndex)
{
  HTTPClient client;
  client.setTimeout(RESPONSE_TIMEOUT_MILLIS);
  String url = "https://" + String(host) + ":" + String(port) + "/garbage/check/" + types[typeIndex].name;

  if (!client.begin(url))
  {
    Serial.println("Connection failed.");
    return -1;
  }

  int httpCode = client.GET();
  String days = client.getString();
  client.end();

  return days.toInt();
}
