// **********************************************************************************
// ESP8266 Teleinfo WEB Client, web client functions
// **********************************************************************************
// Creative Commons Attrib Share-Alike License
// You are free to use/extend this library but please abide with the CC-BY-SA license:
// Attribution-NonCommercial-ShareAlike 4.0 International License
// http://creativecommons.org/licenses/by-nc-sa/4.0/
//
// For any explanation about teleinfo ou use, see my blog
// http://hallard.me/category/tinfo
//
// This program works with the Wifinfo board
// see schematic here https://github.com/hallard/teleinfo/tree/master/Wifinfo
//
// Written by Charles-Henri Hallard (http://hallard.me)
//
// History : V1.00 2015-12-04 - First release
//
// Modifié par theGressier
//       Version 1.0.7 2019-08-02 Changement fonction jeedomPost et httpPost
//
// All text above must be included in any redistribution.
//
// **********************************************************************************

#include "webclient.h"

/* ======================================================================
Function: httpPost
Purpose : Do a http post
Input   : hostname
          port
          url
          data
Output  : true if received 200 OK
Comments: -
====================================================================== */
boolean httpPost(char * host, uint16_t port, char * url, char * data)
{
  HTTPClient http;
  bool ret = false;
  int httpCode=0;

  unsigned long start = millis();

  // configure traged server and url
  http.begin(host, port, url); 

  sprintf(buff,"http%s://%s:%d%s => ", port==443?"s":"", host, port, url);
  Debug(buff);

  // start connection and send HTTP header
  if ( *data ) { // data string is not null, use POST instead of GET
    http.addHeader("Content-Type", "application/json");  //Specify content-type header
    httpCode = http.POST(data);
  } else {
    httpCode = http.GET();
  }
  
  if(httpCode) {
      // HTTP header has been send and Server response header has been handled
      Debug(httpCode);
      Debug(" ");
      // file found at server
      if(httpCode >= 200 && httpCode < 300) {
        String payload = http.getString();
        Debug(payload);
        ret = true;
      }
  } else {
      DebugF("failed!");
  }

  http.end();  //Close connection
  
  sprintf(buff," in %d ms\r\n",millis()-start);
  Debug(buff);
  return ret;
}
/* ======================================================================
Function: build_mqtt_json string (usable by webserver.cpp)
Purpose : construct the json part of mqtt url
Input   : -
Output  : String if some Teleinfo data available
Comments: -
====================================================================== */
String build_mqtt_json(void)
{
  boolean first_item = true;
  String url = "{\"" ;

  ValueList * me = tinfo.getList();
  if (me) {
      // Loop thru the node
      while (me->next) {
         if(! first_item) 
          // go to next node
          me = me->next;
         
         if( ! me->free ) {
                
             
            // On first item, do not add , separator
            if (first_item)
              first_item = false;
            else
              url += "\",\"";
              
            if(validate_value_name(me->name)) {
              url +=  me->name ;
              url += "\":\"" ;
      
              // MQTT ne sait traiter que des valeurs numériques, donc ici il faut faire une 
              // table de mappage, tout à fait arbitraire, mais c"est celle-ci dont je me sers 
              // depuis mes débuts avec la téléinfo
              if (!strcmp(me->name, "OPTARIF")) {
                // L'option tarifaire choisie (Groupe "OPTARIF") est codée sur 4 caractères alphanumériques 
                /* J'ai pris un nombre arbitraire codé dans l'ordre ci-dessous
                je mets le 4eme char à 0, trop de possibilités
                BASE => Option Base. 
                HC.. => Option Heures Creuses. 
                EJP. => Option EJP. 
                BBRx => Option Tempo
                */
                char * p = me->value;
                  
                     if (*p=='B'&&*(p+1)=='A'&&*(p+2)=='S') url += "1";
                else if (*p=='H'&&*(p+1)=='C'&&*(p+2)=='.') url += "2";
                else if (*p=='E'&&*(p+1)=='J'&&*(p+2)=='P') url += "3";
                else if (*p=='B'&&*(p+1)=='B'&&*(p+2)=='R') url += "4";
                else url +="0";
              } else if (!strcmp(me->name, "HHPHC")) {
                // L'horaire heures pleines/heures creuses (Groupe "HHPHC") est codé par un caractère A à Y 
                // J'ai choisi de prendre son code ASCII
                int code = *me->value;
                url += String(code);
              } else if (!strcmp(me->name, "PTEC")) {
                // La période tarifaire en cours (Groupe "PTEC"), est codée sur 4 caractères 
                /* J'ai pris un nombre arbitraire codé dans l'ordre ci-dessous
                TH.. => Toutes les Heures. 
                HC.. => Heures Creuses. 
                HP.. => Heures Pleines. 
                HN.. => Heures Normales. 
                PM.. => Heures de Pointe Mobile. 
                HCJB => Heures Creuses Jours Bleus. 
                HCJW => Heures Creuses Jours Blancs (White). 
                HCJR => Heures Creuses Jours Rouges. 
                HPJB => Heures Pleines Jours Bleus. 
                HPJW => Heures Pleines Jours Blancs (White). 
                HPJR => Heures Pleines Jours Rouges. 
                */
                     if (!strcmp(me->value, "TH..")) url += "1";
                else if (!strcmp(me->value, "HC..")) url += "2";
                else if (!strcmp(me->value, "HP..")) url += "3";
                else if (!strcmp(me->value, "HN..")) url += "4";
                else if (!strcmp(me->value, "PM..")) url += "5";
                else if (!strcmp(me->value, "HCJB")) url += "6";
                else if (!strcmp(me->value, "HCJW")) url += "7";
                else if (!strcmp(me->value, "HCJR")) url += "8";
                else if (!strcmp(me->value, "HPJB")) url += "9";
                else if (!strcmp(me->value, "HPJW")) url += "10";
                else if (!strcmp(me->value, "HPJR")) url += "11";
                else url +="0";
              } else {
                url += me->value;
              }
            } else {
              //Value name not valid : ignore this value, and
              //  force Teleinfo to reinit on next loop !
              need_reinit=true;
            }
         } //not free entry

      } // While next

  } //if me
  // Json end
  url += "\"}";
      
  return url;
}


/* ======================================================================
Function: jeedomPost
Purpose : Do a http post to jeedom server
Input   : 
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean jeedomPost(void)
{
  boolean ret = false;

  // Some basic checking
  if (*config.jeedom.host && *config.jeedom.adco) {
    ValueList * me = tinfo.getList();
    // Got at least one ?
    if (me && me->next) {
      String url ; 
      url = *config.jeedom.url ? config.jeedom.url : "/";
      url += "?";     
      if (*config.jeedom.apikey) {
        url += F("apikey=") ;
        url += config.jeedom.apikey;
      }
      
      String data = "{\"device\":{\"";
      data += config.jeedom.adco;
      data += "\":{\"device\":\"";
      data += config.jeedom.adco;
      data += "\"";
            
      // Loop thru the node
      while (me->next) {
        // go to next node
        me = me->next;

        if( ! me->free ) {
          if(validate_value_name(me->name)) {  
            data += ",\"";
            data += me->name;
            data += "\":\"";
            data += me->value;
            data += "\"";
          }
        }
      } // While me
      // Json end
      data += "}}}";

      //trace dcr le 23/2/2020
      DebuglnF("===== jeedomPost URL "); 
      //DebugF("URL     :"); Debugln(url.c_str()); 
      //DebugF("Data     :"); Debugln(data); 
      //Debug(data);

      ret = httpPost( config.jeedom.host, config.jeedom.port, (char *) url.c_str(), (char *) data.c_str()) ;
    } // if me
  } // if host
  return ret;
}

/* ======================================================================
Function: HTTP Request
Purpose : Do a http request
Input   : 
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean httpRequest(void)
{
  boolean ret = false;

  // Some basic checking
  if (*config.httpReq.host)
  {
    ValueList * me = tinfo.getList();
    // Got at least one ?
    if (me && me->next)
    {
      String url ; 
      boolean skip_item;

      url = *config.httpReq.path ? config.httpReq.path : "/";
      url += "?";

      // Loop thru the node
      while (me->next) {
        // go to next node
        me = me->next;
        skip_item = false;

        // Si Item virtuel, on le met pas
        if (*me->name =='_')
          skip_item = true;

        // On doit ajouter l'item ?
        if (!skip_item)
        {
          String valName = String(me->name);
          if (valName == "HCHP")
          {
            url.replace("%HCHP%", me->value);
          }
          if (valName == "HCHC")
          {
            url.replace("%HCHC%", me->value);
          }
          if (valName == "PAPP")
          {
            url.replace("%PAPP%", me->value);
          }
		      if (valName == "ADCO")
          {
            url.replace("%ADCO%", me->value);
          }
		      if (valName == "OPTARIF")
          {
            url.replace("%OPTARIF%", me->value);
          }
		      if (valName == "ISOUC")
          {
            url.replace("%ISOUC%", me->value);
          }
		      if (valName == "PTEC")
          {
            url.replace("%PTEC%", me->value);
          }
		      if (valName == "IINST")
          {
            url.replace("%IINST%", me->value);
          }
		      if (valName == "IMAX")
          {
            url.replace("%IMAX%", me->value);
          }
		      if (valName == "HHPHC")
          {
            url.replace("%HHPHC%", me->value);
          }
		      if (valName == "MOTDETAT")
          {
            url.replace("%MOTDETAT%", me->value);
          }
		      if (valName == "BASE")
          {
            url.replace("%BASE%", me->value);
          }
	      }
      } // While me

      ret = httpPost( config.httpReq.host, config.httpReq.port, (char *) url.c_str(), NULL) ;
    } // if me
  } // if host
  return ret;
}

#ifdef SENSOR
/* ======================================================================
Function: UPD_switch
Purpose : Do a http request to update Switch state into Domoticz
Input   : 
Output  : true if post returned 200 OK
Comments: -
====================================================================== */
boolean UPD_switch(void)
{
  boolean ret = false;

  // Some basic checking
  if (*config.httpReq.host && (config.httpReq.swidx != 0) )
  {
      char url[128]; 
      char State[5];
      uint16_t port = config.httpReq.port;

      if(port == 0)
        port = 80;

      if(SwitchState)
        sprintf(State,"Off");  //switch ouvert
      else
        sprintf(State,"On");   //switch fermé : portail fermé 

      sprintf(url,"/json.htm?type=command&param=switchlight&idx=%d&switchcmd=%s",(int)config.httpReq.swidx, State);
      //Debugf("Updating switch: <%s>\n",  url );
      ret = httpPost( config.httpReq.host, port, url, NULL) ;
   
  } // if host & idx
  return ret;
}
#endif
