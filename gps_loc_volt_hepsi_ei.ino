/*//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
/*  Devrenin GPS kodları ON4CDU çağrı işaretli amatöre aittir.
/*  Arduino Micro 
 *    2    LCD SDA
 *    3    LCD SCL
 *    RX    GPS TX
 *    A0    ADC V input
 *    Bu yazılım, Tarihi, UTC olarak saati, yüksekliği, Uydu sayısını, enlem ve boylamı, grid locator'u, pil gerilimini
 *    ve gerilimin 10 v.'un altına düşmesi hâlinde şarj ihtiyacını gösterir.
 */

/// Included Libraries ///////////////////////

#include <Wire.h>               
#include <LiquidCrystal_I2C.h> 
LiquidCrystal_I2C lcd(0x27,20,4);

//GPS için define'lar
#define DEG_TO_RAD 0.01745329
#define PI 3.141592654
#define TWOPI 6.28318531
// NMEA fields
#define RMC_TIME 1
#define RMC_STATUS 2
#define RMC_LAT 3
#define RMC_NS 4
#define RMC_LONG 5
#define RMC_EW 6
#define RMC_DATE 9
#define GGA_FIX 6
#define GGA_SATS 7
#define GGA_HDOP 8
#define GGA_ALTITUDE 9
#define HEADER_RMC "$GPRMC"
#define HEADER_GGA "$GPGGA"


/// SYSTEM VARIABLES ////////////////////

//*** GPS için değişkenler
const int       GPSbitrate = 9600;        //GPS cihazının haberleşme hızı
const int       sentenceSize = 82;
const int       max_nr_words = 20;
char            sentence[sentenceSize], sz[20],regel[20],diger[10];
char*           words [max_nr_words];
char            sts, latdir, longdir;
int             alternate;
int             hours, minutes, seconds, year, yeard, month, day;
int             gps_fixval, gps_sats, alti, hdopi;
int             latdeg, latmin, latsec;
int             londeg, lonmin, lonsec;
float           time, date, latf, lonf, hdopf;
boolean         status, gps_fix, hdop;
char            char_string [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
char            num_string[]= "0123456789";

//some variables for sun position calculation
float           Lon, Lat;
float           T,JD_frac,L0,M,e,C,L_true,f,R,GrHrAngle,Obl,RA,Decl,HrAngle;
long            JD_whole,JDx;

// Voltmetre değişkenleri
float           temp=0.0;
float           r1=9960.0;
float           r2=3895.0;
float           input_voltage = 0.0;
// buraya kadar

/// System Setup /////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void setup() {
Serial1.begin(GPSbitrate);      // Uno veya nano kullanacaksanız serial1'i serial yapın.
lcd.init();                     // Initialize the LCD display
lcd.backlight(); 
lcd.clear();
lcd.begin(20,4); 

// Splash Screen: Credit / Version / Site
lcd.setCursor(0,0);   lcd.print(" QCX MiNi YARDIMCI  ");
lcd.setCursor(0,1);   lcd.print(" GPS  - ON4CDU      ");
lcd.setCursor(0,2);   lcd.print(" Mod. - TA2EI       ");
lcd.setCursor(0,3);   lcd.print(" RECEP AYDIN GULEC  ");
delay(4000);
//lcd.blink();
lcd.clear();
while (!Serial1.available())
{
  yok();
}
}
/////////////////////////////////// Operation //////////////////////////////////////////////////////////////////////////////////////

void loop() 
{
  gps();
 
//volc();
}  // End Main Loop /////

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void gps()
{
   
  static int i = 0;
  if (Serial1.available())       // Uno veya nano kullanacaksanız serial1'i serial yapın.
  {
    char ch = Serial1.read();   // Uno veya nano kullanacaksanız serial1'i serial yapın.
    if (ch != '\n' && i < sentenceSize)
    {
      sentence[i] = ch;
      i++;
    }
    else
    {
      sentence[i] = '\0';
      i = 0;
      displayGPS();
    }  
  }
 
  }
/////////////////////////////////////////////////////////////////////////////////////////
void displayGPS()
{
  
  getField ();
  if (strcmp(words [0], HEADER_RMC) == 0)
  {
    processRMCstring();
    printfirstrow ();
    locator();
  }
  else if (strcmp(words[0], HEADER_GGA) == 0)
  {
    processGGAstring ();
  }
  printGGAinfo ();
}
///////////////////////////////////////////////////////////////////////////////////////////
void getField () // split string was probably a better name
{
  char* p_input = &sentence[0];
  //words [0] = strsep (&p_input, ",");
  for (int i=0; i<max_nr_words; ++i)
  {
    words [i] = strsep (&p_input, ",");
  }
}
//////////////////////////////////////////////////////////////////////////////////////////////
void processRMCstring ()
{
    time = atof(words [RMC_TIME]);
    hours = (int)(time * 0.0001);
    minutes = int((time * 0.01) - (hours * 100));
    seconds = int(time - (hours * 10000) - (minutes * 100));
    sts = words[RMC_STATUS][0]; // status
    status = (sts == 'A');
    latf = atof(words [RMC_LAT]); // latitude
    latdeg = latf/100;
    latmin = (latf - (int(latf/100))*100);
    latsec = (((latf -int(latf))*100)*0.6);
    latf = int (latf * 0.01) + (((latf * 0.01) - int(latf * 0.01)) / 0.6);
    latdir = words[RMC_NS][0]; // N/S
    lonf= atof(words [RMC_LONG]);// longitude 
    londeg = lonf/100;
    lonmin = (lonf - (int(lonf/100))*100);
    lonsec = (((lonf -int(lonf))*100)*0.6);
    lonf = int (lonf * 0.01) + ((lonf * 0.01 - int(lonf * 0.01)) / 0.6);
    longdir = words[RMC_EW][0]; // E/W      
    date = atof(words [RMC_DATE]); // date
    day = int(date * 0.0001);
    month = int ((date * 0.01) - (day * 100));
    year = int (date - (day * 10000) - (month * 100));
    year = year + 2000;    
}
///////////////////////////////////////////////////////////////////////////////////////
void processGGAstring ()
{
    gps_fixval = words[GGA_FIX][0] - 48;
    gps_fix = (gps_fixval > 0);
    gps_sats = atoi(words[GGA_SATS]);    
    //hdopf = atof (words[GGA_HDOP]);
    alti = atoi(words[GGA_ALTITUDE]); 
}
////////////////////////////////////////////////////////////////////////////////////////
void printfirstrow ()
{
    sprintf(sz, "%02d.%02d.%04d %02d:%02d UTC",
            day, month , year, hours , minutes );
    lcd.setCursor(0, 0);
    lcd.print(sz); // Saat ve tarih LCD'de gösterilecek  
}
//////////////////////////////////////////////////////////////////////////////////////
void printGGAinfo ()
{
  if (seconds == 30)
    sunpos();
  if (status && gps_fix)
  {
        sprintf (sz, " ");
        sprintf(regel, "Lt:%2d %2d %2d %1c", latdeg, latmin , latsec ,latdir);
        lcd.setCursor(0, 2); lcd.print(regel); // Enlem LCD'de gösterilecek
        sprintf(regel, "Ln:%2d %2d %2d %1c", londeg, lonmin , lonsec ,longdir);
        lcd.setCursor(0, 3); lcd.print(regel); // Boylam LCD'de gösterilecek
        lcd.setCursor (7,1);
        sprintf(diger, "R:%4d m", alti);         
        lcd.print(diger);
        volc();
        lcd.setCursor(16,1);
        lcd.print("U:");
        lcd.print(gps_sats);  
  } 
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void locator()                                       // locator hesabı ve LCD'de gösterilmesi
{
  float loclong = lonf * 1000000;
  float loclat = latf * 1000000;
  float scrap;
  if (longdir == 'E') 
  {
    loclong = (loclong) + 180000000;
  }
  if (longdir == 'W') 
  {
    loclong = 180000000 - (loclong);
  }
  if (latdir == 'N') 
  {
    loclat = loclat + 90000000;
  }
  if (latdir == 'S') 
  {
    loclat = 90000000 - loclat;
  }
  lcd.setCursor(0, 1); // printing QTH locator
  lcd.print(char_string[int(loclong / 20000000)]);        // First Character - longitude based (every 20° = 1 gridsq)  
  lcd.print(char_string[int(loclat / 10000000)]);         // Second Character - latitude based (every 10° = 1 gridsq) 
  scrap = loclong - (20000000 * int(loclong / 20000000)); // Third Character - longitude based (every 2° = 1 gridsq)
  lcd.print(num_string[int(scrap * 10 / 20 / 1000000)]);
  scrap = loclat - (10000000 * int(loclat / 10000000));   // Fourth Character - latitude based (every 1° = 1 gridsq)
  lcd.print(num_string[int(scrap / 1000000)]); 
  scrap = (loclong / 2000000) - (int(loclong / 2000000)); // Fifth Character - longitude based (every 5' = 1 gridsq)
  lcd.print(char_string[int(scrap * 24)]);
  scrap = (loclat / 1000000) - (int(loclat / 1000000));   // Sixth Character - longitude based (every 2.5' = 1 gridsq)
  lcd.print(char_string[int(scrap * 24)]); 
 }
  
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void sunpos()
{
int A, B;  
    Lat = latf*DEG_TO_RAD;
    if (latdir == 'S')
    {
      Lat = -Lat;
    } 
    Lon = lonf*DEG_TO_RAD;
    if (longdir == 'W')
    {
      Lon = -Lon;
    }
    if(month <= 2) 
    { 
      year--;
      month+=12;
    }  
    A = year/100; 
    B = 2 - A + A/4;
    JD_whole = (long)(365.25*(year + 4716)) + (int)(30.6001 * (month+1)) + day + B - 1524;
    JD_frac=(hours + minutes/60. + seconds/3600.)/24. - .5;
    T = JD_whole - 2451545; 
    T = (T + JD_frac)/36525.;
    L0 = DEG_TO_RAD * fmod(280.46645 + 36000.76983 * T,360);
    M = DEG_TO_RAD * fmod(357.5291 + 35999.0503 * T,360);
    e = 0.016708617 - 0.000042037 * T;
    C=DEG_TO_RAD*((1.9146 - 0.004847 * T) * sin(M) + (0.019993 - 0.000101 * T) * sin(2 * M) + 0.00029 * sin(3 * M));
    f = M + C;
    Obl = DEG_TO_RAD * (23 + 26/60. + 21.448/3600. - 46.815/3600 * T);     
    JDx = JD_whole - 2451545;  
    GrHrAngle = 280.46061837 + (360 * JDx)%360 + .98564736629 * JDx + 360.98564736629 * JD_frac;
    GrHrAngle = fmod(GrHrAngle,360.);    
    L_true = fmod(C + L0,TWOPI);
    R=1.000001018 * (1 - e * e)/(1 + e * cos(f));
    RA = atan2(sin(L_true) * cos(Obl),cos(L_true));
    Decl = asin(sin(Obl) * sin(L_true));
    HrAngle = DEG_TO_RAD * GrHrAngle + Lon - RA;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////

void volc()
{
   int analog_value = analogRead(A0);
    temp = (analog_value * 5.0) / 1024.0; 
    input_voltage = temp / (r2/(r1+r2)); 
    lcd.setCursor(14, 3);
    if(input_voltage<10)
    {
    lcd.print("DOLDUR");
    }
 if(input_voltage>10)
    {
    lcd.print(input_voltage);
    lcd.print("v");
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////
void bekle()
{
   lcd.setCursor(0,1);
    lcd.print("     GPS SiNYALi    ");
    lcd.setCursor(0,2);
    lcd.print("     BEKLENiYOR     ");
    delay(1000);
    //lcd.clear();
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////
void yok()

{
    lcd.setCursor(0,1);
    lcd.print("     GPS CiHAZI     ");
    lcd.setCursor(0,2);
    lcd.print("   TAKILI DEGiL!    ");
    delay(1000);
    lcd.clear();
  } 
