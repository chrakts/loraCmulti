#include "loraCmulti.h"
#include "../communication/Communication.h"
#include "myConstants.h"  // nur benötigt für LEDs
#include "External.h"     // nur benötigt für "Node"

RelaisInfo relInfo[RELAISINFONUM];
uint8_t relInfoToSend   = 0;
uint8_t relInfoFinished = 0;

volatile uint8_t txIsReady = false;
volatile uint8_t rxIsReady = false;

volatile int8_t rxRssi = 0;
char rxMessage[LORAMESSAGELENGTH];

char LoraCmultiBuffer[LORACMULTIBUFFERR_LENGTH];
Cmulti2Buffer_v02 loraCmulti(LoraCmultiBuffer,LORACMULTIBUFFERR_LENGTH,Node);

void test4Received()
{
    if(rxIsReady==true)
    {
      LED_BLAU_ON;
      gotInfoFromLora();
      rxIsReady=false;
    }
}

void gotInfoFromLora() // reicht, die über LORA empfangenene Daten an CNET weiter
{
  uint8_t l;
  char act_char;
  // hier sollte noch eine Checksummenüberprüfung
  if(rxMessage[0] == '#')
  {
    act_char = rxMessage[1];
    if( isxdigit(act_char) )
    {
      if( act_char<58)
        l = 16*(act_char-48);
      else
      {
        act_char = tolower(act_char);
        l = 16*(act_char-87);
      }

      act_char = rxMessage[2];
      if( isxdigit(act_char) )
      {
        if( act_char<58)
          l += (act_char-48);
        else
        {
          act_char = tolower(act_char);
          l += (act_char-87);
        }
        if(strlen(rxMessage)-3==l) // Längenüberprüfung
        {
          if(rxMessage[3] & WITH_AES256)
          {
            char newMessage[17],target[3],source[3];
            if(decryptMessage(rxMessage,newMessage))
            {
              target[0] = rxMessage[4];
              target[1] = rxMessage[5];
              target[2] = 0;
              source[0] = rxMessage[6];
              source[1] = rxMessage[7];
              source[2] = 0;
              loraCmulti.clearEncryption();
              loraCmulti.setAlternativeNode(source);
              loraCmulti.sendStandard(newMessage,target,rxMessage[9],rxMessage[10],rxMessage[11],rxMessage[12]);
              strcpy(relInfo[relInfoToSend].content, loraCmulti.get() );
              relInfo[relInfoToSend].medium = VIA_CNET; // Funk
              newInfo2Send();
              loraCmulti.broadcastInt16(rxRssi,'R','x','i');
              strcpy(relInfo[relInfoToSend].content, loraCmulti.get() );
              relInfo[relInfoToSend].medium = VIA_CNET; // Funk
              newInfo2Send();
              loraCmulti.resetNode();


            }
          }
          else // ohne Verschlüsselung
          {
            char source[3];
            source[0] = rxMessage[6];
            source[1] = rxMessage[7];
            source[2] = 0;

            strcpy(relInfo[relInfoToSend].content, rxMessage );
            relInfo[relInfoToSend].medium = VIA_CNET; // Funk
            newInfo2Send();
            loraCmulti.setAlternativeNode(source);
            loraCmulti.broadcastInt16(rxRssi,'R','x','i');
            strcpy(relInfo[relInfoToSend].content, loraCmulti.get() );
            relInfo[relInfoToSend].medium = VIA_CNET; // Funk
            newInfo2Send();
            loraCmulti.resetNode();
          }
        }
      }
    }
  }
}

void newInfo2Send()
{
  relInfoToSend++;
  if(relInfoToSend>=RELAISINFONUM)
    relInfoToSend = 0;
}

bool decryptMessage(char *message, char *deMessage)
{
  uint8_t key[16] = {AES256_KEY};
  uint8_t p,i;
  uint8_t result[16];
  if(strlen(message)>50)
  {
    i=0;
    for(p=13;p<45;p+=2)
    {
      if(message[p]>=65)
      {
        result[i] = (message[p]-65)*16+(message[p+1]-65);
        i += 1;
      }
      else
      {
        return false;
      }
    }
    loraCmulti.setEncryption(NULL);
    loraCmulti.encryptSetKey(key);
    loraCmulti.decryptData(result);
    loraCmulti.getEncryptData(result);
    cmulti.clearEncryption();
    for(i=0;i<16;i++)
    {
      deMessage[i] = (char) result[i];
      if(deMessage[i]==0)
        i=18;
    }
    return true;
  }
  else
    return false;
}

void sendViaRelay(char *relayText) // schickt die vom CNET empfangenen Daten über LORA weiter
{
  strcpy(relInfo[relInfoToSend].content,relayText);
  relInfo[relInfoToSend].medium = VIA_LORA; // Funk
  newInfo2Send();
  free(relayText);
}

bool processRelaisInfos(Communication *cm)
{
  if(relInfoToSend!=relInfoFinished)
  {
    switch(relInfo[relInfoFinished].medium)
    {
      case VIA_CNET:
        cm->print(relInfo[relInfoFinished].content);
        LED_BLAU_OFF;
      break;
      case VIA_LORA:

        LoRa_sendMessage(relInfo[relInfoFinished].content);
        //while(txIsReady==false)
        //{}
        //LoRa_rxMode();
        cm->print(relInfo[relInfoFinished].content);
      break;
    }
    relInfoFinished++;
    if(relInfoFinished>=RELAISINFONUM)
      relInfoFinished = 0;
  }
  return(relInfoFinished==relInfoToSend);
}


void LoRa_rxMode(){
  //LoRa.enableInvertIQ();                // active invert I and Q signals // war auskommentiert
  LoRa.receive();                       // set receive mode
}

void LoRa_txMode(){
  LoRa.idle();                          // set standby mode
  LoRa.disableInvertIQ();               // normal mode
}

void LoRa_sendMessage(char *message) {
  txIsReady = false;
  //LoRa_txMode();                        // set tx mode
  LoRa.beginPacket();                   // start packet
  LoRa.write((uint8_t*)message,strlen(message));                  // add payload
  LoRa.endPacket(false);                 // finish packet and send it !!!!!!!!!!!!! war true
}

void onReceive(int packetSize)
{
  uint8_t cointer=0;
  while (LoRa.available()) {
    rxMessage[cointer] = (char)LoRa.read();
    cointer++;
    if (cointer>=LORAMESSAGELENGTH-1)
      cointer--;
  }
  rxMessage[cointer]=0;
  rxRssi = LoRa.packetRssi();
  rxIsReady=true;
}

void onTxDone() {
  txIsReady = true;
}
