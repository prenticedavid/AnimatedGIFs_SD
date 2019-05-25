/*
    Animated GIFs Display Code for SmartMatrix and 32x32 RGB LED Panels

    This file contains code to enumerate and select animated GIF files by name

    Written by: Craig A. Lindley
*/

#include "FilenameFunctions.h"    //defines USE_SPIFFS as reqd
#include <Adafruit_Arcada.h>

extern Adafruit_Arcada arcada;

File file;

int numberOfFiles;

extern uint32_t timeSpentFS;

bool fileSeekCallback(unsigned long position) {
  bool r;
  uint32_t t = millis();
  r = file.seek(position);
  timeSpentFS += millis() - t;
  return r;
}

unsigned long filePositionCallback(void) {
    return file.position();
}

int fileReadCallback(void) {
    return file.read();
}

int fileReadBlockCallback(void * buffer, int numberOfBytes) {
    uint32_t t = millis();
    int r = file.read((uint8_t*)buffer, numberOfBytes); //.kbv
    timeSpentFS += millis() - t;
    return r;
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

    numberOfFiles = 0;

    File directory = arcada.open(directoryName);
    File file;
    if (!directory) {
        return -1;
    }

    while (file = directory.openNextFile()) {
        char filename[80];
        file.getName(filename, 80);
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

    directory.close();

    return numberOfFiles;
}

// Get the full path/filename of the GIF file with specified index
void getGIFFilenameByIndex(const char *directoryName, int index, char *pnBuffer) {

  // Make sure index is in range
  if ((index < 0) || (index >= numberOfFiles))
      return;

  File directory = arcada.open(directoryName);
  if (!directory) {
      return;
  }

  while ((index >= 0)) {
     file = directory.openNextFile();
     if (!file) break;
        char filename[80];
        file.getName(filename, 80);
        if (isAnimationFile(filename)) {
            index--;

            // Copy the directory name into the pathname buffer
            strcpy(pnBuffer, directoryName);
            int len = strlen(pnBuffer);
            if (len == 0 || pnBuffer[len - 1] != '/') strcat(pnBuffer, "/");
            // Append the filename to the pathname
            strcat(pnBuffer, filename);
        }

        file.close();
    }

    file.close();
    directory.close();
}

int openGifFilenameByIndex(const char *directoryName, int index) {
    char pathname[255];

    getGIFFilenameByIndex(directoryName, index, pathname);

    Serial.print("Pathname: ");
    Serial.println(pathname);

    if (file)
        file.close();

    // Attempt to open the file for reading
    file = arcada.open(pathname, O_READ);
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
