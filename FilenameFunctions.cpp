/*
    Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels

    This file contains code to enumerate and select animated GIF files by name

    Written by: Craig A. Lindley
*/

#include "FilenameFunctions.h"    //defines USE_SPIFFS as reqd

#if defined (ARDUINO)
#ifdef USE_SPIFFS
#if defined(ESP8266)
#include "FS.h"
#define USE_SPIFFS_DIR
#elif defined(ESP32)
#include <SPIFFS.h>
#define SD SPIFFS
#else
#error USE_SPIFFS only valid on Expressif controllers
#endif
#else
#include <SD.h>
#endif
#elif defined (SPARK)
#include "sd-card-library-photon-compat/sd-card-library-photon-compat.h"
#endif

File file;

int numberOfFiles;

bool fileSeekCallback(unsigned long position) {
#ifdef USE_SPIFFS
    return file.seek(position, SeekSet);
#else
    return file.seek(position);
#endif
}

unsigned long filePositionCallback(void) {
    return file.position();
}

int fileReadCallback(void) {
    return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    return file.read((uint8_t*)buffer, numberOfBytes); //.kbv
}

int initSdCard(int chipSelectPin) {
    // initialize the SD card at full speed
    pinMode(chipSelectPin, OUTPUT);
#ifdef USE_SPIFFS
    if (!SPIFFS.begin())
        return -1;
#else
    if (!SD.begin(chipSelectPin))
        return -1;
#endif
    return 0;
}

bool isAnimationFile(const char filename []) {
    if (filename[0] == '_')
        return false;

    if (filename[0] == '~')
        return false;

    if (filename[0] == '.')
        return false;

    String filenameString = String(filename);   //.kbv STM32 and ESP need separate statements
    filenameString.toUpperCase();
    if (filenameString.endsWith(".GIF") != 1)
        return false;

    return true;
}

// Enumerate and possibly display the animated GIF filenames in GIFS directory
int enumerateGIFFiles(const char *directoryName, bool displayFilenames) {

    char *filename;
    numberOfFiles = 0;
#ifdef USE_SPIFFS_DIR
    File file;
    Dir directory = SPIFFS.openDir(directoryName);
    //    if (!directory == 0) return -1;

    while (directory.next()) {
        file = directory.openFile("r");
        if (!file) break;
#else
    File file;
    File directory = SD.open(directoryName);
    if (!directory) {
        return -1;
    }

    while (file = directory.openNextFile()) {
#endif
        filename = (char*)file.name();
        if (isAnimationFile(filename)) {
            numberOfFiles++;
            if (displayFilenames) {
                Serial.print(numberOfFiles);
                Serial.print(":");
                Serial.print(filename);
                Serial.print("    size:");
                Serial.println(file.size());
            }
        }
        else Serial.println(filename);
        file.close();
    }

    //    file.close();
#ifdef USE_SPIFFS
#else
    directory.close();
#endif

    return numberOfFiles;
}

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer) {

    char* filename;

    // Make sure index is in range
    if ((index < 0) || (index >= numberOfFiles))
        return;

#ifdef USE_SPIFFS_DIR
    Dir directory = SPIFFS.openDir(directoryName);
    //    if (!directory) return;

    while (directory.next() && (index >= 0)) {
        file = directory.openFile("r");
        if (!file) break;
#else
    File directory = SD.open(directoryName);
    if (!directory) {
        return;
    }

    while ((index >= 0)) {
        file = directory.openNextFile();
        if (!file) break;
#endif

        filename = (char*)file.name();  //.kbv
        if (isAnimationFile(filename)) {
            index--;

            // Copy the directory name into the pathname buffer
            strcpy(pnBuffer, directoryName);
#if defined(ESP32) || defined(USE_SPIFFS)
            pnBuffer[0] = 0;
#else
            int len = strlen(pnBuffer);
            if (len == 0 || pnBuffer[len - 1] != '/') strcat(pnBuffer, "/");
#endif
            // Append the filename to the pathname
            strcat(pnBuffer, filename);
        }

        file.close();
    }

    file.close();
#ifdef USE_SPIFFS
#else
    directory.close();
#endif
}

int openGifFilenameByIndex(const char *directoryName, int index) {
    char pathname[40];

    getGIFFilenameByIndex(directoryName, index, pathname);

    Serial.print("Pathname: ");
    Serial.println(pathname);

    if (file)
        file.close();

    // Attempt to open the file for reading
#ifdef USE_SPIFFS
    file = SPIFFS.open(pathname, "r");
#else
    file = SD.open(pathname);
#endif
    if (!file) {
        Serial.println("Error opening GIF file");
        return -1;
    }

    return 0;
}


// Return a random animated gif path/filename from the specified directory
void chooseRandomGIFFilename(const char *directoryName, char *pnBuffer) {

    int index = random(numberOfFiles);
    getGIFFilenameByIndex(directoryName, index, pnBuffer);
}

