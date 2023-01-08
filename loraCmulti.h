#ifndef LORACMULTI_H_INCLUDED
#define LORACMULTI_H_INCLUDED

#include <avr/io.h>
#include <stdbool.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <avr/sleep.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "LoRa.h"
#include "Cmulti2Buffer_v02.h"


#define RELAISINFONUM 6
#define LORAMESSAGELENGTH 60
#define LORACMULTIBUFFERR_LENGTH 55

struct relaisInfo
{
  char   target[3];
  char   source[3];
  char   function;
  char   address;
  char   job;
  char   endChar;
  char   content[LORAMESSAGELENGTH];
  uint8_t medium;
};
typedef struct relaisInfo RelaisInfo;

void test4Received();
void sendViaRelay(char *relayText);
void processRelaisInfos(Communication *cm);
void onTxDone();
void onReceive(int packetSize);
void LoRa_sendMessage(char *message);
void LoRa_txMode();
void LoRa_rxMode();
void evaluate(void);
void gotInfoFromLora();
void newInfo2Send();
bool decryptMessage(char *message, char *deMessage);

enum{VIA_CNET=1,VIA_LORA};

#endif // LORACMULTI_H_INCLUDED
