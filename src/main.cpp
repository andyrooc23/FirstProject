#include <M5Core2.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <NTPClient.h>
#include "EGR425_Phase1_weather_bitmap_images.h"
#include "WiFi.h"

////////////////////////////////////////////////////////////////////
// Variables
////////////////////////////////////////////////////////////////////
// TODO 3: Register for openweather account and get API key
String urlOpenWeather = "https://api.openweathermap.org/data/2.5/weather?";
String apiKey = "033baa727a3dbdbcbbf8df1e22f6bf28";

// TODO 1: WiFi variables
String wifiNetworkName = "Point-B";
String wifiPassword = "";
boolean imp = true;
boolean screen1 = true;

int firstHeight = 20;
int secondHeight = 140;
int startWidth = 20;
int blockSize = 30;
int offset = 60;
int zipcode [5] = {9,2,5,0,4};

// Time variables
unsigned long lastTime = 0;
unsigned long timerDelay = 5000;  // 5000; 5 minutes (300,000ms) or 5 seconds (5,000ms)

// LCD variables
int sWidth;
int sHeight;

// Time Client
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP);

////////////////////////////////////////////////////////////////////
// Method header declarations
////////////////////////////////////////////////////////////////////
String httpGETRequest(const char* serverName);
void drawWeatherImage(String iconId, int resizeMult);

///////////////////////////////////////////////////////////////
// Put your setup code here, to run once
///////////////////////////////////////////////////////////////
void setup() {
    // Initialize the device
    M5.begin();
    
    // Set screen orientation and get height/width 
    sWidth = M5.Lcd.width();
    sHeight = M5.Lcd.height();

    // TODO 2: Connect to WiFi
    WiFi.begin(wifiNetworkName.c_str(), wifiPassword.c_str());
    Serial.printf("Connecting");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n\nConnected to WiFi network with IP address: ");
    Serial.println(WiFi.localIP());
    timeClient.begin();
    timeClient.setTimeOffset(-28800);
}

void resetZipcodeScreen(){
  M5.Lcd.fillScreen(TFT_WHITE);
  // Draw blocks and initial numbers
  for (int i = 0; i < 5; i++){
    M5.Lcd.fillRect(startWidth + offset*i, firstHeight, blockSize, blockSize, TFT_LIGHTGREY);
    M5.Lcd.fillTriangle(startWidth + offset*i + blockSize/2, firstHeight, startWidth + offset*i, firstHeight + blockSize, startWidth + offset*i + blockSize, firstHeight + blockSize, TFT_BLUE);
  }
  for (int i = 0; i < 5; i++){
    M5.Lcd.fillRect(startWidth + offset*i, secondHeight, blockSize, blockSize, TFT_LIGHTGREY);
    M5.Lcd.fillTriangle(startWidth + offset*i + blockSize/2, secondHeight + blockSize, startWidth + offset*i, secondHeight, startWidth + offset*i + blockSize, secondHeight, TFT_BLUE);
  }

  M5.Lcd.fillRect(sWidth / 4, 7 * sHeight / 8, sWidth / 2, 30, TFT_DARKGREEN);
  M5.Lcd.setCursor(sWidth / 4 + 10, 7 * sHeight / 8 + 5);
  M5.Lcd.setTextColor(BLACK);
  M5.Lcd.print("ENTER");
}
///////////////////////////////////////////////////////////////
// Put your main code here, to run repeatedly
///////////////////////////////////////////////////////////////
void loop() {
    M5.update();

    if(M5.BtnA.wasPressed()){
        imp = !imp;
    }

    if(M5.BtnC.wasPressed()){
      screen1 = !screen1;
      if(!screen1){
        resetZipcodeScreen();
      }
    }

    if(!screen1){ // zipcode screen
      // Listen for stuff
      if (M5.Touch.ispressed()){
        Point tp = M5.Touch.getPressPoint();
        if(tp.x != -1){
          if(firstHeight < tp.y && tp.y < firstHeight + blockSize){
            // Serial.println("Upper buttons at " + ((String) tp.x) + " " +  ((String) tp.y));   
            int x = tp.x;
            for (int i = 0; i < 5; i++){
              if(startWidth + offset * i < x && x < startWidth + offset * i + blockSize){
                resetZipcodeScreen();
                zipcode[i]++;
                delay(200);
              }
            }
          }
          if(secondHeight < tp.y && tp.y < secondHeight + blockSize){
            // Serial.println("Lower buttons at " + ((String) tp.x) + " " + ((String) tp.y));   
            int x = tp.x;
            for (int i = 0; i < 5; i++){
              if(startWidth + offset * i < x && x < startWidth + offset * i + blockSize){
                resetZipcodeScreen();
                zipcode[i]--;
                delay(200);
              }
            }
          }
          // ENTER BUTTON
          if(tp.y > 7 * sHeight / 8 && tp.x > sWidth / 4 && tp.x < 3*sWidth / 4){
            screen1 = !screen1;
          }
        }
      }

      for(int i = 0; i < 5; i++){
        if(zipcode[i] < 0){
          zipcode[i] = 9;
        }else if(zipcode[i] > 9){
          zipcode[i] = 0;
        }
      }


      //Draw Numbers every time
      int numY = (firstHeight + secondHeight) / 2;
      int numX = startWidth;
      for(int i = 0; i < 5; i++){
        M5.Lcd.setTextSize(5);
        M5.Lcd.setCursor(numX + offset * i, numY);
        M5.Lcd.print(zipcode[i]);
      }


    }else{

      // Only execute every so often
      if ((millis() - lastTime) > timerDelay) {
        if (WiFi.status() == WL_CONNECTED) {

          //////////////////////////////////////////////////////////////////
          // TODO 4: Hardcode the specific city,state,country into the query
          // Examples: https://api.openweathermap.org/data/2.5/weather?q=riverside,ca,usa&units=imperial&appid=YOUR_API_KEY
          //////////////////////////////////////////////////////////////////
          String zipString = "";
          for(int i = 0; i < 5; i++){
            zipString += (String) zipcode[i];
          }
          String imperial = urlOpenWeather + "zip="+zipString+",us&units=imperial&appid=" + apiKey;
          String celcius = urlOpenWeather + "zip="+zipString+",us&units=metric&appid=" + apiKey;

          String serverURL = imp ? imperial : celcius;
          //Serial.println(serverURL); // Debug print

          //////////////////////////////////////////////////////////////////
          // TODO 5: Make GET request and store reponse
          //////////////////////////////////////////////////////////////////
          String response = httpGETRequest(serverURL.c_str());
          //Serial.print(response); // Debug print
          
          //////////////////////////////////////////////////////////////////
          // TODO 6: Import ArduinoJSON Library and then use arduinojson.org/v6/assistant to
          // compute the proper capacity (this is a weird library thing) and initialize
          // the json object
          //////////////////////////////////////////////////////////////////
          const size_t jsonCapacity = 768+250;
          DynamicJsonDocument objResponse(jsonCapacity);

          //////////////////////////////////////////////////////////////////
          // TODO 7: (uncomment) Deserialize the JSON document and test if parsing succeeded
          //////////////////////////////////////////////////////////////////
          DeserializationError error = deserializeJson(objResponse, response);
          if (error) {
              Serial.print(F("deserializeJson() failed: "));
              Serial.println(error.f_str());
              return;
          }
          //serializeJsonPretty(objResponse, Serial); // Debug print

          //////////////////////////////////////////////////////////////////
          // TODO 8: Parse Response to get the weather description and icon
          //////////////////////////////////////////////////////////////////
          JsonArray arrWeather = objResponse["weather"];
          JsonObject objWeather0 = arrWeather.getElement(0);
          String strWeatherDesc = objWeather0["main"];
          String strWeatherIcon = objWeather0["icon"];
          String cityName = objResponse["name"];

          // TODO 9: Parse response to get the temperatures
          JsonObject objMain = objResponse["main"];
          double tempNow = objMain["temp"];
          double tempMin = objMain["temp_min"];
          double tempMax = objMain["temp_max"];
          Serial.printf("NOW: %.1f F and %s\tMIN: %.1f F\tMax: %.1f F\n", tempNow, strWeatherDesc, tempMin, tempMax);

          //////////////////////////////////////////////////////////////////
          // TODO 10: We can download the image directly, but there is no easy
          // way to work with a PNG file on the ESP32 (in Arduino) so we will
          // take another route - see EGR425_Phase1_weather_bitmap_images.h
          // for more details
          //////////////////////////////////////////////////////////////////
          // String imagePath = "http://openweathermap.org/img/wn/" + strWeatherIcon + "@2x.png";
          // Serial.println(imagePath);
          // response = httpGETRequest(imagePath.c_str());
          // Serial.print(response);

          //////////////////////////////////////////////////////////////////
          // TODO 12: Draw background - light blue if day time and navy blue of night
          //////////////////////////////////////////////////////////////////
          uint16_t primaryTextColor;
          if (strWeatherIcon.indexOf("d") >= 0) {
              M5.Lcd.fillScreen(TFT_CYAN);
              primaryTextColor = TFT_DARKGREY;
          } else {
              M5.Lcd.fillScreen(TFT_NAVY);
              primaryTextColor = TFT_WHITE;
          }
          
          //////////////////////////////////////////////////////////////////
          // TODO 13: Draw the icon on the right side of the screen - the built in 
          // drawBitmap method works, but we cannot scale up the image
          // size well, so we'll call our own method
          //////////////////////////////////////////////////////////////////
          //M5.Lcd.drawBitmap(0, 0, 100, 100, myBitmap, TFT_BLACK);
          drawWeatherImage(strWeatherIcon, 3);
          
          //////////////////////////////////////////////////////////////////
          // This code will draw the temperature centered on the screen; left
          // it out in favor of another formatting style
          //////////////////////////////////////////////////////////////////
          /*M5.Lcd.setTextColor(TFT_WHITE);
          textSize = 10;
          M5.Lcd.setTextSize(textSize);
          int iTempNow = tempNow;
          tWidth = textSize * String(iTempNow).length() * 5;
          tHeight = textSize * 5;
          M5.Lcd.setCursor(sWidth/2 - tWidth/2,sHeight/2 - tHeight/2);
          M5.Lcd.print(iTempNow);*/

          //////////////////////////////////////////////////////////////////
          // TODO 14: Draw the temperatures and city name
          //////////////////////////////////////////////////////////////////
          int pad = 10;
          M5.Lcd.setCursor(pad, pad);
          M5.Lcd.setTextColor(TFT_BLUE);
          M5.Lcd.setTextSize(3);
          if(imp){
              M5.Lcd.printf("LO:%0.fF\n", tempMin);
          }else{
              M5.Lcd.printf("LO:%0.fC\n", tempMin);
          }
          
          M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
          M5.Lcd.setTextColor(TFT_DARKGREEN);
          M5.Lcd.setTextSize(8);
          if(imp){
              M5.Lcd.printf("NOW:%0.fF\n", tempNow);
          }else{
              M5.Lcd.printf("NOW:%0.fC\n", tempNow);
          }

          M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
          M5.Lcd.setTextColor(TFT_RED);
          M5.Lcd.setTextSize(3);
          if(imp){
              M5.Lcd.printf("HI:%0.fF\n\n", tempMax);
          }else{
              M5.Lcd.printf("HI:%0.fC\n\n", tempMax);
          }

          // My added on code
          timeClient.update();
          M5.Lcd.setCursor(pad,M5.Lcd.getCursorY());
          M5.Lcd.setTextColor(TFT_DARKGREY);
          M5.Lcd.setTextSize(3);

          int hours = timeClient.getHours();
          int mins = timeClient.getMinutes();
          int seconds = timeClient.getSeconds();
          String timeFormat = "";
          if (hours < 12){
            if (hours == 0){
              timeFormat += "12";
            }else{
              timeFormat += (String) hours;
            }
            timeFormat += ":";
            if(mins < 10){
              timeFormat += "0" + (String) mins;
            }else{
              timeFormat += (String) mins;
            }
            timeFormat += ":";
            timeFormat += (String) seconds;
            timeFormat += "AM";
          }else{
            timeFormat += (String) (hours - 12);
            timeFormat += ":";
            if(mins < 10){
              timeFormat += "0" + (String) mins;
            }else{
              timeFormat += (String) mins;
            }
            timeFormat += ":";
            timeFormat += (String) seconds;
            timeFormat += "PM";
          }
          M5.Lcd.printf("%s\n\n", timeFormat);

          M5.Lcd.setCursor(pad, M5.Lcd.getCursorY());
          M5.Lcd.setTextColor(primaryTextColor);
          int textSize = 8 - (int) (((float) cityName.length() / 20.0) * 10);
          M5.Lcd.setTextSize(textSize);
          M5.Lcd.printf("%s\n", cityName.c_str());



        } else {
            Serial.println("WiFi Disconnected");
        }

        // M5.Lcd.printf("Time: ", timeClient.getFormattedTime());

        // Stop my code

        // Update the last time to NOW
        lastTime = millis();
      }
    }
}
/////////////////////////////////////////////////////////////////
// This method takes in a URL and makes a GET request to the
// URL, returning the response.
/////////////////////////////////////////////////////////////////
String httpGETRequest(const char* serverURL) {
    
    // Initialize client
    HTTPClient http;
    http.begin(serverURL);

    // Send HTTP GET request and obtain response
    int httpResponseCode = http.GET();
    String response = http.getString();

    // Check if got an error
    if (httpResponseCode > 0)
        Serial.printf("HTTP Response code: %d\n", httpResponseCode);
    else {
        Serial.printf("HTTP Response ERROR code: %d\n", httpResponseCode);
        Serial.printf("Server Response: %s\n", response);
    }

    // Free resources and return response
    http.end();
    return response;
}

/////////////////////////////////////////////////////////////////
// This method takes in an image icon string (from API) and a 
// resize multiple and draws the corresponding image (bitmap byte
// arrays found in EGR425_Phase1_weather_bitmap_images.h) to scale (for 
// example, if resizeMult==2, will draw the image as 200x200 instead
// of the native 100x100 pixels) on the right-hand side of the
// screen (centered vertically). 
/////////////////////////////////////////////////////////////////
void drawWeatherImage(String iconId, int resizeMult) {

    // Get the corresponding byte array
    const uint16_t * weatherBitmap = getWeatherBitmap(iconId);

    // Compute offsets so that the image is centered vertically and is
    // right-aligned
    int yOffset = -(resizeMult * imgSqDim - M5.Lcd.height()) / 2;
    int xOffset = sWidth - (imgSqDim*resizeMult*.8); // Right align (image doesn't take up entire array)
    //int xOffset = (M5.Lcd.width() / 2) - (imgSqDim * resizeMult / 2); // center horizontally
    
    // Iterate through each pixel of the imgSqDim x imgSqDim (100 x 100) array
    for (int y = 0; y < imgSqDim; y++) {
        for (int x = 0; x < imgSqDim; x++) {
            // Compute the linear index in the array and get pixel value
            int pixNum = (y * imgSqDim) + x;
            uint16_t pixel = weatherBitmap[pixNum];

            // If the pixel is black, do NOT draw (treat it as transparent);
            // otherwise, draw the value
            if (pixel != 0) {
                // 16-bit RBG565 values give the high 5 pixels to red, the middle
                // 6 pixels to green and the low 5 pixels to blue as described
                // here: http://www.barth-dev.de/online/rgb565-color-picker/
                byte red = (pixel >> 11) & 0b0000000000011111;
                red = red << 3;
                byte green = (pixel >> 5) & 0b0000000000111111;
                green = green << 2;
                byte blue = pixel & 0b0000000000011111;
                blue = blue << 3;

                // Scale image; for example, if resizeMult == 2, draw a 2x2
                // filled square for each original pixel
                for (int i = 0; i < resizeMult; i++) {
                    for (int j = 0; j < resizeMult; j++) {
                        int xDraw = x * resizeMult + i + xOffset;
                        int yDraw = y * resizeMult + j + yOffset;
                        M5.Lcd.drawPixel(xDraw, yDraw, M5.Lcd.color565(red, green, blue));
                    }
                }
            }
        }
    }
}
//////////////////////////////////////////////////////////////////////////////////
// For more documentation see the following links:
// https://github.com/m5stack/m5-docs/blob/master/docs/en/api/
// https://docs.m5stack.com/en/api/core2/lcd_api
//////////////////////////////////////////////////////////////////////////////////