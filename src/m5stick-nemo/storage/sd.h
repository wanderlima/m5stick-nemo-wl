
bool sdcardMounted = false;
#if defined(SDCARD)
  #include <FS.h>
  #include <SD.h>
  #include <SPI.h>

  SPIClass sdcardSPI;
  SemaphoreHandle_t sdcardSemaphore;

  void appendToFile(fs::FS& fs, const char* path, const char* text) {
    if (xSemaphoreTake(sdcardSemaphore, portMAX_DELAY) == pdTRUE) {
      File file = fs.open(path, FILE_APPEND);
      if (!file) {
        Serial.println("Failed to open file for appending");
        xSemaphoreGive(sdcardSemaphore);
        return;
      }
      Serial.printf("Appending text '%s' to file: %s\n", text, path);
      if (file.println(text)) {
        Serial.println("Text appended");
      } else {
        Serial.println("Append failed");
      }
      file.close();
      xSemaphoreGive(sdcardSemaphore);
    }
  }
#endif

bool setupSdCard() {
#if defined(SDCARD)
  sdcardSemaphore = xSemaphoreCreateMutex();
  sdcardSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN); // start SPI communications

  delay(10);

  if (!SD.begin(SD_CS_PIN, sdcardSPI)) { //excluded * -> poiter indicator -
    sdcardSPI.end(); // Closes SPI connections and release pin header.
    Serial.println("Failed to mount SDCARD");
    
    return false;
  } else {
    Serial.println("SDCARD mounted successfully");
    sdcardMounted = true;
    return true;
  }
#else
  return false;
#endif
}

#if defined(SDCARD)
    void ToggleSDCard()
    {
      rstOverride = true;
      isSwitching = true;
      current_proc=1;
      DISP.fillScreen(BGCOLOR);
      DISP.setCursor(5, 1);
      if (sdcardMounted == true) {
        sdcardMounted = false;
        uint8_t cardType = 0;
        uint64_t cardSize = 0;
        SD.end();
        sdcardSPI.end(); // Closes SPI connections and release pins.
        DISP.println("SD Card Unmounted..");
        DISP.println("pin header released.");
        Serial.println("SD Card Unmounted...");
      } else {
        sdcardSPI.begin(SD_CLK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        delay(10);

        if (!SD.begin(SD_CS_PIN, sdcardSPI)) {
          Serial.println("Card Mount Failed");
          DISP.println("Card Mount Failed");
          sdcardSPI.end();

        } else {
          //
          sdcardMounted = true;
          uint8_t cardType = SD.cardType();
          if (cardType == CARD_NONE) {
            DISP.println("None SD Card");
            Serial.println("None SD Card");
          }
          DISP.print("SD Card Type: ");
          if (cardType == CARD_MMC) {
            DISP.println("MMC");
            Serial.println("MMC");
          } else if (cardType == CARD_SD) {
            DISP.println("SDSC");
            Serial.println("SDSC");
          } else if (cardType == CARD_SDHC) {
            DISP.println("SDHC");
            Serial.println("SDHC");
          } else {
            DISP.println("UNKNOWN");
            Serial.println("UNKNOWN");
          }
          uint64_t cardSize = SD.cardSize() / (1024 * 1024);
          DISP.printf("SD Card Size: %lluMB\n", cardSize);
          Serial.printf("SD Card Size: %lluMB\n", cardSize);
        }
      }
      delay(1500);
    }
#endif
