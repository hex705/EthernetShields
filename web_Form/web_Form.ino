
/*
   Listen for commands to trip relays for a enternet shield equiped arduino
   
   Some is from the Arduino Ethernet
   WebServer example and some from Limor Fried (Adafruit) so its probably under GPL
  
  Tutorial is at http://www.ladyada.net/learn/arduino/ethfiles.html
*/


#include <Ethernet.h>
#include <SPI.h>

// mac address
 byte  mac[] = {  0x90,0xA2,0xDA,0x00,0x60,0xF1 }; // should be on shield
// if shield is arduino -- its on the bakc
// if shield is white -- its on the front
//  if the shield is your own -- then come see me

// byte ip[] = { 141, 117, 45, 176 };

EthernetServer server(80);

/************ light stuff **************/
boolean auth= false;

int numlights = 4; // number of elements of the arrays below
boolean states[] = {
  1,1,1,0}; // list of the light/relay states
// int  birghtness[]={1,1,1,0};  // list of which lights/relays should be in the blink cycle
int pins[] = {
  3,5,6,4}; // list of the light/relay pins
String lights[] = {"red","green","blue","beacon"}; // list of the "light" lables

String secret = String (""); // change this to your own, better, password.

int blinkc=1;
int blinkmax=20000;


/*********** light/relay routines ****************/

void setlights() { // turn the lights on or off
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    setone(states[i],pins[i]);
  }
}

void getlights() { // turn the lights on or off
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    states[i]=digitalRead(pins[i]);
  }
}


void setone(boolean light, int pin) { //turn on one light
  if (light) {
    digitalWrite(pin,HIGH);
    Serial.print("turning on ");
    Serial.println(pin,DEC);
  } 
  else {
    digitalWrite(pin,LOW);
  }
}

void setstates(boolean newstate) { // set all the states to be on or off, used in blinking
  for (int i = 0; i < numlights; i++) { // loop through all the lights...
    states[i]=newstate && states[i]; // only turn on lights listed in "blinkstates"
  }
}

void createForm(EthernetClient client) {  // draw the form
  client.println("<form  action='get'>");
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    dobox(client,lights[i],states[i]);
  }
  client.println("password: <input type='password' name='a' ><br>");
  client.println("</form>");
}

void dobox(EthernetClient client, String item, boolean checked) {  // draw a single checkbox
  client.print("<input type='checkbox' name='c' value='");
  client.print(item);
  client.print("'");
  if (checked) {
    client.print(" checked");
  }
  client.print(">");
  client.print(item);
  client.println("<br>");
}

// How big our line buffer should be. 100 is plenty!
#define BUFSIZ 100

void setup() {

  Serial.begin(9600);

  Serial.println("Starting");
  //  Serial.println(FreeRam());  

  // set pins as output and turn on all the lights to show that we don't have access yet.
  for (int i = 0; i < numlights; i++) { // loop through all the lights
    pinMode(pins[i], OUTPUT);
  }

  setlights();

  pinMode(10, OUTPUT);                       // set the SS pin as an output (necessary!)
  digitalWrite(10, HIGH);                    // but turn off the W5100 chip!
  // Debugging complete, we start the server!
  Ethernet.begin(mac);


  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore:
    for(;;)
      ;
  }
  // print your local IP address:

  Serial.print("My IP address: ");

  for (byte thisByte = 0; thisByte < 4; thisByte++) {
    // print the value of each byte of the IP address:
    Serial.print(Ethernet.localIP()[thisByte], DEC);
    Serial.print('.');
  }

  delay(1000);
  Serial.println();


  server.begin();
}


void loop()
{
  char clientline[BUFSIZ];
  int index = 0;

  EthernetClient client = server.available();
  if (client) 
  {
    // an http request ends with a blank line
    boolean current_line_is_blank = true;

    // reset the input buffer
    index = 0;

    while (client.connected()) 
    {
      if (client.available()) 
      {
        char c = client.read();

        // If it isn't a new line, add the character to the buffer
        if (c != '\n' && c != '\r') 
        {
          clientline[index] = c;
          index++;
          // are we too big for the buffer? start tossing out data
          if (index >= BUFSIZ) index = BUFSIZ -1;

          // continue to read more data!
          continue;
        }

        // got a \n or \r new line, which means the string is done
        clientline[index] = 0;

        // Print it out for debugging
        Serial.println("\nRaw client request " );
        Serial.println(clientline);
        Serial.println("\n");


        // Look for substring such as a request 
        if (strstr(clientline, "GET / ") != 0) { // the SPACE after the slash '/' is critical
          // send a standard http response header
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Server: Arduino");
          client.println();
          createForm(client);
       
        } 
        
        else if (strstr(clientline, "GET /") != 0) { // this time no space after the /, so a request!
          char *request;
          request = clientline + 5; // look after the "GET /" (5 chars)
          // a little trick, look for the " HTTP/1.1" string and 
          // turn the first character of the substring into a 0 to clear it out.
          (strstr(clientline, " HTTP"))[0] = 0;
          
          // print the requested string
          Serial.println("\nClient request " );
          Serial.println(request);
          Serial.println();
          
          client.println("HTTP/1.1 200 OK");
          client.println("Content-Type: text/html");
          client.println("Server: Arduino - request");
          client.println();
          
          auth= false;  // assume, until proven different, that the request is not authenticated
         
          for (int i = 0; i < numlights; i++) { // loop through all the lights...
            states[i]=false;  // ...and assume they are off
          }

         
         // check for password
          if (strstr(request, getChars(secret)) !=0) {
            client.println("<h2>Authenticated</h2>");
            auth= true;
          } 
          else{
            client.println("<h2>Not Authenticated, so ... NOT</h2>");
          }
          
          for (int i = 0; i < numlights; i++) { // loop through all the lights...
          
           // states[i]=false;  // ...and assume they are off

            if (strstr(request, getChars(lights[i])) != 0){ // if they asked for this light
            Serial.println("request to change ");
            Serial.println(lights[i]);
            Serial.println("\t" + i);
              client.print("Turning ");
              client.print(lights[i]);
              client.println(" on<br>");
             
              if (auth){ //actually do it
                states[i]=true;
              } // end of "auth" 
            }  // end of "asked for this light"   
         
          }   // end of light loop

          if (auth) { // now that all the colors are set, change the lights
            setlights();
            blinkc=0; // stop blinking
            /*  if "error" is sent as part of the request, ignore everything and start the error blink */
            if (strstr(request, "error") != 0) {
              client.println("<h1>The request reported an error:<br>'");
              client.print(request);
              client.println("'</h1>");
              blinkc=1; // restart the blinking

              for (int i = 0; i < numlights; i++) { // loop through all the lights...
               // blinkstates[i]=false;  // ...and assume they are off
              }

              if (strstr(request, "error=1") != 0) { // if they reported an error state 1
               // blinkstates[1]=1; //yellow on
              }

              if (strstr(request, "error=2") != 0) { // if they reported an error state 2
               // blinkstates[0]=1; //red on
              }

//              if (!(blinkstates[0] || blinkstates[1])) { // if they reported an error that wasn't listed above
//                blinkstates[0]=1; //red on
//                blinkstates[1]=1; // yellow on
//                blinkstates[2]=1; // green off
//              }
              blinkmax=10000; // blink twice as fast
            }

          }
          getlights(); // get the current settings before...
          createForm(client); // ... drawing the form
          // client.println("<img src =\"http://74.12.146.18/latestFrame.jpg\" alt=\"LEDS\" >");  
        } 
        else {
          // everything else is a 404
          client.println("HTTP/1.1 404 Not Found");
          client.println("Content-Type: text/html");
          client.println();
          client.println("<h2>404: HUH?</h2>");
        }
        break;
      }
    }
    // give the web browser time to receive the data
    delay(1);
    client.stop();
  }
  /*******************************************
   *  when first turned on, blink the lights
   *    once blinkc is set to 0, it won't blink any more.
   ******************************************/
  if (blinkc == 1 ) { // turn on
    Serial.println("On");
    setstates(true);
    //    states[3]=false; // don't flash on the beacon, it's obnoxious.
    setlights();
    blinkc++;
  } 
  else if (blinkc == (blinkmax/2)) { // half way through, turn off
    Serial.println("Off");
    setstates(false);
    setlights();
  } 
  else if (blinkc > blinkmax) { // reset loop
    blinkc=1;
  }
  if (blinkc >1 ) { // increment loop if counter is set
    blinkc++;
  }
}




char* getChars(String s) {

  //Serial.println(s);
  char c[s.length()+1];

  s.toCharArray(c, s.length()+1);
  Serial.println(c);
  return c;


}

