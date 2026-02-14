#define IS_SEND
#define IS_WIFI

#include <Arduino.h>
#include <IRremoteESP8266.h>
#ifdef IS_SEND
#include <IRsend.h>
#else
#include <IRac.h>
#include <IRrecv.h>
#include <IRtext.h>
#include <IRutils.h>
#endif
#ifdef IS_WIFI
#include <WiFi.h>
#include <ESPmDNS.h>
#include "ssid.h"
#endif

#define RECVPIN 32
#define SENDPIN 27
#define SIGNPIN 25
#define KHZ 38

void setSerial();
void setSend();
void setRecv();
#ifdef IS_WIFI
void setWifi();
#endif
void loopSendIr();
void loopReceiveIr();
void loopWifi();
#ifdef IS_WIFI
void resLedOn(WiFiClient client, String request);
void resLedOf(WiFiClient client, String request);
void resIrNec(WiFiClient client, String request);
void resIrUnknown(WiFiClient client, String request);
#endif

#ifdef IS_SEND
// send
const uint16_t kFrequency = 38000; // 38kHz
IRsend irsend(SENDPIN);            // 送信オブジェクトを作成
#else
// recv
const uint16_t captureBufferSize = 1024;
const uint8_t timeout = 100;
const uint16_t minUnknownSize = 12;
IRrecv irrecv(RECVPIN, captureBufferSize, timeout, true);
decode_results results;
#endif
#ifdef IS_WIFI
const char *ssid = LOCALSSID; // Wi-FiのSSID
const char *password = PASS;  // Wi-Fiのパスワード
WiFiServer server(80);        // HTTPサーバーをポート80で起動
#endif

void setup()
{
  setSerial();
  pinMode(SIGNPIN, OUTPUT);
#ifdef IS_SEND
  setSend();
#else
  setRecv();
#endif
#ifdef IS_WIFI
  setWifi();
#endif
}
void setSerial()
{
  Serial.begin(115200);
  while (!Serial)
  {
    delay(50);
  }
}

#ifdef IS_SEND
void setSend()
{
  irsend.begin(); // 赤外線送信の初期化
}
#else
void setRecv()
{
  irrecv.enableIRIn(); // Start the receiver
}
#endif
#ifdef IS_WIFI
void setWifi()
{
  // Wi-Fi接続
  Serial.print("Connecting to ");
  Serial.println(ssid);
  // WiFi.disconnect(true, true);
  WiFi.begin(ssid, password);

  // Wi-Fi接続待機
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected.");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  // サーバーの起動
  server.begin();

  // mDNSの開始
  if (!MDNS.begin("esp32"))
  { // "esp32"がデバイス名となる
    Serial.println("Error setting up MDNS responder!");
  }
  Serial.println("MDNS responder started");
  digitalWrite(SIGNPIN, HIGH);
  delay(1000);
  digitalWrite(SIGNPIN, LOW);
  delay(1000);
  digitalWrite(SIGNPIN, HIGH);
  delay(1000);
  digitalWrite(SIGNPIN, LOW);
  delay(1000);
  digitalWrite(SIGNPIN, HIGH);
  delay(1000);
  digitalWrite(SIGNPIN, LOW);
  delay(1000);
}
#endif

void loop()
{

#ifdef IS_SEND
  loopSendIr();
#else
  loopReceiveIr();
#endif
#ifdef IS_WIFI
  loopWifi();
#endif
}

#ifdef IS_SEND
void loopSendIr()
{
  // digitalWrite(SIGNPIN, HIGH);
  // delay(1000);
  // digitalWrite(SIGNPIN, LOW);
  // delay(1000);
}
#else
void loopReceiveIr()
{
  // recv
  // Check if the IR code has been received.
  if (irrecv.decode(&results))
  {
    // digitalWrite(SENDPIN, HIGH);

    Serial.println("results.value");
    Serial.println(results.value);
    Serial.println(resultToSourceCode(&results));
    // digitalWrite(SENDPIN, LOW);
  }
}
#endif
#ifdef IS_WIFI
void resLedOn(WiFiClient client, String request)
{
  if (request.indexOf("GET /IR_ON") >= 0)
  {
    digitalWrite(SENDPIN, HIGH);
  }
}
void resLedOf(WiFiClient client, String request)
{
  if (request.indexOf("GET /IR_OFF") >= 0)
  {
    digitalWrite(SENDPIN, LOW);
  }
}
void resIrNec(WiFiClient client, String request)
{

  if (request.indexOf("/IR_NEC?code=") >= 0)
  {
    // クエリパラメータの開始位置を取得
    int startIndex = request.indexOf("code=") + 5;
    // パラメータの終了位置を取得（ここではスペースで終了と仮定）
    int endIndex = request.indexOf(' ', startIndex);

    // コードを抽出
    String irCodeStr = request.substring(startIndex, endIndex);
    unsigned long irCode4 = strtoul(irCodeStr.c_str(), NULL, 16); // 16進数として変換
    uint16_t rawData[68];                                         // NECはリーダー + 32ビットデータ + 終了パルスで68個のデータが必要
    int index = 0;

    // リーダーパルス
    // NECプロトコルのリーダー部分 (9msパルス、4.5msスペース)
    rawData[index++] = 9000; // 9ms
    rawData[index++] = 4500; // 4.5ms

    // データ部分（32ビット）
    for (int i = 0; i < 32; i++)
    {
      if (irCode4 & 0x80000000)
      {                          // ビット1
        rawData[index++] = 560;  // パルス: 560µs
        rawData[index++] = 1690; // スペース: 1690µs
      }
      else
      {                         // ビット0
        rawData[index++] = 560; // パルス: 560µs
        rawData[index++] = 560; // スペース: 560µs
      }
      irCode4 <<= 1; // 次のビットにシフト
    }

    // 終了ビット
    rawData[index++] = 560; // パルス: 560µs

#ifdef IS_SEND
    // NEC信号の送信
    irsend.sendRaw(rawData, index, 38); // 38kHzで送信
#endif
    Serial.println(irCodeStr);
  }
}
void resIrUnknown(WiFiClient client, String request)
{
  if (request.indexOf("/IR_UNKNOWN?data=") >= 0)
  {
    // クエリパラメータの開始位置を取得
    int startIndex = request.indexOf("data=") + 5;
    // パラメータの終了位置を取得（ここではスペースで終了と仮定）
    int endIndex = request.indexOf(' ', startIndex);

    // データを抽出
    String dataStr = request.substring(startIndex, endIndex);

    // カンマで区切られた数値をパース
    uint16_t values[650]; // 十分なサイズの配列を準備
    int count = 0;
    int index = 0;

    while (dataStr.length() > 0)
    {
      int commaIndex = dataStr.indexOf(',');
      if (commaIndex == -1)
      {
        // 最後の値を取得
        values[count++] = dataStr.toInt();
        break;
      }
      else
      {
        values[count++] = dataStr.substring(0, commaIndex).toInt();
        dataStr = dataStr.substring(commaIndex + 1);
      }
    }

// IR信号の送信 (例: 38kHz, 1msのパルス幅で500msの間隔)
#ifdef IS_SEND
    irsend.sendRaw(values, count, KHZ); // 38kHzで送信
#endif

    Serial.print("Sending raw IR data: ");
    for (int i = 0; i < count; i++)
    {
      Serial.print(values[i]);
      Serial.print(" ");
    }
    Serial.println();
  }
}

void loopWifi()
{
  WiFiClient client = server.available(); // listen for incoming clients

  if (client)
  {
    Serial.println("---------------");
    String currentLine = "";
    boolean requestProcessed = false; // リクエストが処理されたかどうかを示すフラグ
    String fullRequest = "";          // リクエスト全体を保持する変数

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);
        if (c == '\n')
        {

          if (currentLine.length() == 0 && !requestProcessed)
          {
            client.println("HTTP/1.1 200 OK");         // ステータスライン
            client.println("Content-Type: text/html"); // Content-Typeヘッダー
            client.println("Access-Control-Allow-Origin: *");  // すべてのオリジンからのアクセスを許可
            client.println("Connection: close");       // 接続の終了を示す
            client.println();                          // 空行でヘッダーとボディを区切る

            client.println("<html><body><h1>ESP32 Response</h1>"); // ボディの開始
            client.println("<p>" + fullRequest + "</p>");          // フルリクエストの内容を含める
            client.println("</body></html>");                      // ボディの終了
            client.println();

            // リクエストがすべて受信された後、ここで一度だけ処理を実行
            digitalWrite(SIGNPIN, HIGH);
            delay(200);
            digitalWrite(SIGNPIN, LOW);

            resLedOn(client, fullRequest);
            resLedOf(client, fullRequest);
            resIrNec(client, fullRequest);
            resIrUnknown(client, fullRequest);
            
            digitalWrite(SIGNPIN, HIGH);
            delay(100);
            digitalWrite(SIGNPIN, LOW);
            delay(50);
            digitalWrite(SIGNPIN, HIGH);
            delay(100);
            digitalWrite(SIGNPIN, LOW);
            delay(50);

            requestProcessed = true; // 処理が実行されたことを示すフラグをセット
            break;
          }
          else
          {
            currentLine = ""; // 次のリクエスト行のためにcurrentLineをリセット
          }
        }
        else if (c != '\r')
        {
          currentLine += c; // 改行以外の文字をcurrentLineに追加
          fullRequest += c; // fullRequestにもリクエスト全体を追加
        }
      }
    }

    client.stop();
    Serial.println("Client Disconnected.");
  }
}
#endif
