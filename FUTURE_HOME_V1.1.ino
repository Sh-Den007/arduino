#include <EEPROM.h>
#include <Wire.h>                   //Подключаем библиотеку для использования однопроводного интерфейса                                     
#include <OneWire.h>                //Подключаем библиотеку для температурного датчика DS18B20
#include <LiquidCrystal_I2C.h>      //Подключаем I2C библиотеку экрана
LiquidCrystal_I2C lcd(0x27,20,4);   //Устанавливаем LCD-адрес 0x27 для отображения 20 символов и 4 линии
#define LED1_PIN 9          //Используем цифровой ШИМ ПОРТ D9 для розовых полноспектральных сетодиодов
#define LED2_PIN 10         //Используем цифровой ШИМ ПОРТ D10 для белых сетодиодных полосок
#define LAMP_PIN 5          //Используем цифровой ПОРТ D2 для реле люминесцентных ламп
#define FILTER_PIN 2        //Используем цифровой ПОРТ D3 для реле системы фильтрации
#define HEATER_PIN 3        //Используем цифровой ПОРТ D4 для реле системы обогрева
#define CO2_PIN 4           //Используем цифровой ПОРТ D5 для реле системы подачи CO2
#define TEMP_PIN A0         //Используем аналоговый ПОРТ A0 для подключения датчика температуры DS18B20
#define BUTTONS_PIN A7      //Используем аналоговый ПОРТ A7 для подключения кнопок управления контроллером
#define BUZZER_PIN 11       //Используем цифровой ПОРТ D11 для пьезоизлучателя
//================================== Rtc_Clock часы реального времени*
#define DS3231_I2C_ADDRESS 104 
byte seconds, minutes, hours, day, date, month, year;
byte decToBcd (byte val){return((val/10*16)+(val%10));}
//================================== Температурный датчик*
OneWire ds(TEMP_PIN);               //Создаем объект для работы с термометром
float temp, temp_pred;              //Температура воды в аквариуме, текущая и предыдущая
unsigned long lastUpdateTime = 0;   // Переменная для хранения времени последнего считывания с датчика
const int TEMP_UPDATE_TIME = 5000;  // Определяем периодичность проверок
const bool OFF    = false;
const bool ON     = true;
// Пользовательский символ градуса 
byte degree[8] = { 
0b00110, 
0b01001, 
0b01001, 
0b00110, 
0b00000, 
0b00000, 
0b00000, 
0b00000, 
}; 
// Пользовательский символ "черный прямоугольник"
byte progressbar[8] = { 
0b00000, 
0b00000, 
0b11111, 
0b11111, 
0b11111, 
0b11111, 
0b00000, 
0b00000, 
};
//================================== Метод отображающий версию ПО и разработчика
void ShowVersion() {
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print("WELLCOME to");    //Выводим версию и прочее
  lcd.setCursor(0,2);
  lcd.print("FUTURE HOME V1.1"); // 20
  delay(5000);  
}
void setup() {
  Wire.begin();                     //Инициируем I2C интерфейс
  //Устанавливаем ШИМ 8 разрядов, 62,5 кГц на выходах D9 и D10.
  TCCR1A = TCCR1A & 0xe0 | 1;
  TCCR1B = TCCR1B & 0xe0 | 0x09;  
   
  pinMode(LED1_PIN, OUTPUT);        // устанавливает режим работы - выход
  analogWrite(LED1_PIN, 0);
  pinMode(LED2_PIN, OUTPUT);        // устанавливает режим работы - выход
  analogWrite(LED2_PIN, 0);
  pinMode(LAMP_PIN, OUTPUT);        // устанавливает режим работы - выход
  //digitalWrite(LAMP_PIN, HIGH);     // выключает
  setLAMP(OFF);
  pinMode(FILTER_PIN, OUTPUT);      // устанавливает режим работы - выход
  //digitalWrite(FILTER_PIN, LOW);    // включает
  setFILTER(ON);
  pinMode(HEATER_PIN, OUTPUT);      // устанавливает режим работы - выход
  //digitalWrite(HEATER_PIN, HIGH);   // выключает
  setHEATER(OFF);
  pinMode(CO2_PIN, OUTPUT);         // устанавливает режим работы - выход
  //digitalWrite(CO2_PIN, HIGH) ;     // выключает
  setCO2(OFF);
 
  pinMode(TEMP_PIN, INPUT);         // устанавливает режим работы - вход
  pinMode(BUTTONS_PIN, INPUT);      // устанавливает режим работы - вход
   
  lcd.init();                       // запускаем библиотеку экрана 
  lcd.backlight();
  lcd.clear();                      // Очищаем на всякий случай дисплей
  lcd.createChar(1, degree);        // Создаем пользовательский символ (градус)
  lcd.createChar(2, progressbar);   // Создаем пользовательский символ (ProgressBar)
  LoadSettings();                   // ЗАГРУЖАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ ЭНЕРГОНЕЗАВИСИМОЙ ПАМЯТИ
  firstRUN();
  ShowVersion();
  lcd.clear();                      //Очистка дисплея
}
//================================== Функция переводит число в строку и дополняет ее ведущими нулями
String ByteToStrFormat(byte val, byte digits = 1) {
  String result;
  result = val;
  while (result.length()<digits) result = "0"+result;
  return (String)result;
}
//-----------------------------------------------------------------------------------
// Нажатые кнопки
const byte BUTTON_NONE   = 0;
const byte BUTTON_UP     = 1;
const byte BUTTON_DOWN   = 2;
const byte BUTTON_OK     = 3;
const byte BUTTON_MENU   = 4;
int getPressedButton()
{
 byte KeyNum=0;
 int KeyValue1=0;
 int KeyValue2=0;
  
//Читаем в цикле аналоговый вход, для подавления дребезга и нестабильности читаем по два раза подряд, пока значения не будут равны.
//Если значения равны 1023 – значит не была нажата ни одна кнопка.
do {
 // считываем значения с аналогового входа (A7)  
 KeyValue1=analogRead(BUTTONS_PIN);
 delay(10);
 KeyValue2=analogRead(BUTTONS_PIN);
 delay(5);
 } while (KeyValue1!=KeyValue2);
//Интерпретируем полученное значение и определяем код нажатой клавиши
 if (KeyValue2 > 900) 
  {KeyNum = BUTTON_NONE;}
 else if (KeyValue2 > 450) 
       {KeyNum = BUTTON_MENU;}
      else if (KeyValue2 > 250) 
            {KeyNum = BUTTON_OK;}
           else if (KeyValue2 > 100) 
                 {KeyNum = BUTTON_UP;}
                else {KeyNum = BUTTON_DOWN;}
return KeyNum;    //Возвращаем код нажатой клавиши
}
//================================== Считывание температуры
int detectTemperature(){
  byte data[2];
  ds.reset();// Начинаем взаимодействие со сброса всех предыдущих команд и параметров
  ds.write(0xCC);// Даем датчику DS18b20 команду пропустить поиск по адресу. В нашем случае только одно устрйоство 
  ds.write(0x44);// Даем датчику DS18b20 команду измерить температуру. 
  if ((abs(millis() - lastUpdateTime)) > TEMP_UPDATE_TIME) {
    lastUpdateTime = millis();
    ds.reset();// Теперь готовимся получить значение измеренной температуры
    ds.write(0xCC);
    ds.write(0xBE);// Просим передать нам значение регистров со значением температуры
    // Получаем и считываем ответ
    data[0] = ds.read(); // Читаем младший байт значения температуры
    data[1] = ds.read(); // А теперь старший
    // Формируем итоговое значение: 
    //    - сперва "склеиваем" значение, 
    //    - затем умножаем его на коэффициент, соответсвующий разрешающей способности (для 12 бит по умолчанию - это 0,0625)
    temp_pred = temp; // Запоминаем предыдущую температуру.
    temp =  ((data[1] << 8) | data[0]) * 0.0625;  // и вычисляем текущую.
  }
}
//================================== Обработка установки RTC часов
void setRtc(byte seconds, byte minutes, byte hours, byte day, byte date, byte month, byte year){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);        //104 is DS3231 device address
  Wire.write(0x00);                                  //Start at register 0
  Wire.write(decToBcd(seconds));
  Wire.write(decToBcd(minutes));
  Wire.write(decToBcd(hours));
  Wire.write(decToBcd(day));
  Wire.write(decToBcd(date));
  Wire.write(decToBcd(month));
  Wire.write(decToBcd(year));
  Wire.endTransmission();
}
//================================== Обработка RTC часов
void timeRtc(){
  Wire.beginTransmission(DS3231_I2C_ADDRESS);        //104 is DS3231 device address
  Wire.write(0x00);                                  //Start at register 0
  Wire.endTransmission();
  Wire.requestFrom(DS3231_I2C_ADDRESS, 7);           //Request seven bytes
  if(Wire.available()){ 
    seconds = Wire.read();                           //Get second
    minutes = Wire.read();                           //Get minute
    hours   = Wire.read();                           //Get hour
    day     = Wire.read();
    date    = Wire.read();
    month   = Wire.read();                           //Get month
    year    = Wire.read();
        
    seconds = (((seconds & B11110000)>>4)*10 + (seconds & B00001111));   //Convert BCD to decimal
    minutes = (((minutes & B11110000)>>4)*10 + (minutes & B00001111));
    hours   = (((hours & B00110000)>>4)*10 + (hours & B00001111));       //Convert BCD to decimal (assume 24 hour mode)
    day     = (day & B00000111);                                         // 1-7
    date    = (((date & B00110000)>>4)*10 + (date & B00001111));         //Convert BCD to decimal  1-31
    month   = (((month & B00010000)>>4)*10 + (month & B00001111));       //msb7 is century overflow
    year    = (((year & B11110000)>>4)*10 + (year & B00001111));
  }
}
//=============================== Переменные первого таймера для LED1 (розовый свет)
bool rassvet1_LED1;               // флаг первого рассвета (утреннего)
bool zakat1_LED1;                 // флаг первого заката (дневного)
bool rassvet2_LED1;               // флаг второго рассвета (дневного)
bool zakat2_LED1;                 // флаг второго заката (вечернего)
byte hours_rassvet1_LED1;         // час начала первого рассвета (утреннего)
byte minutes_rassvet1_LED1;       // минута начала первого рассвета (утреннего)
byte duration_rassvet1_LED1;      // длительность первого рассвета (утреннего) (в минутах)
byte hours_zakat1_LED1;           // час начала первого заката (дневного)
byte minutes_zakat1_LED1;         // минута начала первого заката (дневного)
byte duration_zakat1_LED1;        // длительность первого заката (дневного) (в минутах)
byte hours_rassvet2_LED1;         // час начала второго рассвета (дневного)
byte minutes_rassvet2_LED1;       // минута начала второго рассвета (дневного)
byte duration_rassvet2_LED1;      // длительность второго рассвета (дневного) (в минутах)
byte hours_zakat2_LED1;           // час начала второго заката (вечернего)
byte minutes_zakat2_LED1;         // минута начала второго заката (вечернего)
byte duration_zakat2_LED1;        // длительность второго заката (вечернего) (в минутах)
byte bright_LED1;                 // Текущее значение яркости LED1
byte PWM_LED1_MIN = 0;            // Если необходим ток покоя на LED - изменить это значение
byte PWM_LED1_MAX = 255;          // Если необходимо ограничить максимальную яркость - уменьшить значение 
byte PWM_LED1_MIN_DAY = 15;       // Минимальное значение ШИМ для дневного затенения
//=============================== Переменные второго таймера для LED2 (белый свет)
bool rassvet1_LED2;               // флаг первого рассвета (утреннего)
bool zakat1_LED2;                 // флаг первого заката (дневного)
bool rassvet2_LED2;               // флаг второго рассвета (дневного)
bool zakat2_LED2;                 // флаг второго заката (вечернего)
byte hours_rassvet1_LED2;         // час начала первого рассвета (утреннего)
byte minutes_rassvet1_LED2;       // минута начала первого рассвета (утреннего)
byte duration_rassvet1_LED2;      // длительность первого рассвета (утреннего) (в минутах)
byte hours_zakat1_LED2;           // час начала первого заката (дневного)
byte minutes_zakat1_LED2;         // минута начала первого заката (дневного)
byte duration_zakat1_LED2;        // длительность первого заката (дневного) (в минутах)
byte hours_rassvet2_LED2;         // час начала второго рассвета (дневного)
byte minutes_rassvet2_LED2;       // минута начала второго рассвета (дневного)
byte duration_rassvet2_LED2;      // длительность второго рассвета (дневного) (в минутах)
byte hours_zakat2_LED2;           // час начала второго заката (вечернего)
byte minutes_zakat2_LED2;         // минута начала второго заката (вечернего)
byte duration_zakat2_LED2;        // длительность второго заката (вечернего) (в минутах)
byte bright_LED2;                 // Текущее значение яркости LED2
byte PWM_LED2_MIN = 0;            // Если необходим ток покоя на LED - изменить это значение
byte PWM_LED2_MAX = 255;          // Если необходимо ограничить максимальную яркость - уменьшить значение 
byte PWM_LED2_MIN_DAY = 15;       // Минимальное значение ШИМ для дневного затенения
//=============================== Переменные третьего таймера для LAMP (Люминесцентные лампы)
bool LAMP_ON;                     // флаг состояния ламп, true - включены, false - выключены
byte hours_ON1_LAMP;              // час начала первого рассвета (утреннего)
byte minutes_ON1_LAMP;            // минута начала первого рассвета (утреннего)
byte hours_OFF1_LAMP;             // час начала первого заката (дневного)
byte minutes_OFF1_LAMP;           // минута начала первого заката (дневного)
byte hours_ON2_LAMP;              // час начала второго рассвета (дневного)
byte minutes_ON2_LAMP;            // минута начала второго рассвета (дневного)
byte hours_OFF2_LAMP;             // час начала второго заката (вечернего)
byte minutes_OFF2_LAMP;           // минута начала второго заката (вечернего)
//=============================== Переменные четвертого таймера для CO2 (или чего-то еще, например аэрация)
bool CO2_ON;                      // флаг состояния, true - включен, false - выключен
byte hours_ON1_CO2;               // час начала первого рассвета (утреннего)
byte minutes_ON1_CO2;             // минута начала первого рассвета (утреннего)
byte hours_OFF1_CO2;              // час начала первого заката (дневного)
byte minutes_OFF1_CO2;            // минута начала первого заката (дневного)
byte hours_ON2_CO2;               // час начала второго рассвета (дневного)
byte minutes_ON2_CO2;             // минута начала второго рассвета (дневного)
byte hours_OFF2_CO2;              // час начала второго заката (вечернего)
byte minutes_OFF2_CO2;            // минута начала второго заката (вечернего)
#define mn 60UL         // Дополнительные константы для удобства 
#define hr 3600UL       // Отражают соответствующие количества секунд 
#define d 86400UL       // в минуте, в часе, в сутках (дне)
//============================== Метод вызывается один раз из секции setup()
void firstRUN() {
  byte pwm_LED;         // Значение для записи в порт ШИМ от 0 до 255
  long CurrTimeInSecs;   // Текущее время в секундах
  long Rassvet1TimeInSecs; // Время первого рассвета в секундах
  long Zakat2TimeInSecs;   // Время второго заката в секундах
  timeRtc();
  // LED1 Проверяем не ночь ли сейчас? отрабатывает при внезапной загрузке (включении) ардуины ночью, чтобы не включился свет на всю
  // если ночь то выставляем минимальный ШИМ, если день, то максимальный.
  CurrTimeInSecs = hours*hr + minutes*mn +seconds;
  Rassvet1TimeInSecs = hours_rassvet1_LED1*hr + minutes_rassvet1_LED1*mn;
  Zakat2TimeInSecs = hours_zakat2_LED1*hr + minutes_zakat2_LED1*mn + duration_zakat2_LED1*mn;
  if ((CurrTimeInSecs > Zakat2TimeInSecs)||(CurrTimeInSecs < Rassvet1TimeInSecs)) {
    pwm_LED = 0; //PWM_LED1_MIN;     // Устанавливаем минимальное значение для ШИМ
  }
  else {
    pwm_LED = PWM_LED1_MAX;     // Устанавливаем максимальное значение для ШИМ
  }
  analogWrite(LED1_PIN, pwm_LED);
  bright_LED1 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
  // LED2 Проверяем не ночь ли сейчас? отрабатывает при внезапной загрузке (включении) ардуины ночью, чтобы не включился свет на всю
  // если ночь то выставляем минимальный ШИМ, если день, то максимальный.
  CurrTimeInSecs = hours*hr + minutes*mn +seconds;
  Rassvet1TimeInSecs = hours_rassvet1_LED2*hr + minutes_rassvet1_LED2*mn;
  Zakat2TimeInSecs = hours_zakat2_LED2*hr + minutes_zakat2_LED2*mn + duration_zakat2_LED2*mn;
  if ((CurrTimeInSecs > Zakat2TimeInSecs)||(CurrTimeInSecs < Rassvet1TimeInSecs)) {
    pwm_LED = 0; //PWM_LED2_MIN;     // Устанавливаем минимальное значение для ШИМ
  }
  else {
    pwm_LED = PWM_LED2_MAX;     // Устанавливаем максимальное значение для ШИМ
  }
  analogWrite(LED2_PIN, pwm_LED);
  bright_LED2 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
}
//============================== Первый таймер для LED1 (розовый свет)
void timerLED1()
{
  byte pwm_LED;         // Значение для записи в порт ШИМ от 0 до 255
  long duration;         // Текущая длительность с момента начала рассвета в секундах
  //----- Обрабатываем первый рассвет (утренний)
  if (rassvet1_LED1) {    
    duration = (hours-hours_rassvet1_LED1)*hr + (minutes-minutes_rassvet1_LED1)*mn + seconds;
    if (duration < (duration_rassvet1_LED1*mn)) {
      pwm_LED = (PWM_LED1_MAX*duration)/(duration_rassvet1_LED1*mn);
      if (pwm_LED < PWM_LED1_MIN) {
        pwm_LED = PWM_LED1_MIN;
      }
    }
    else {
      rassvet1_LED1 = false;     // Рассвет закончился
      pwm_LED = PWM_LED1_MAX;    // Устанавливаем максимальное значение для ШИМ
    }
    analogWrite(LED1_PIN, pwm_LED);
    bright_LED1 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
  }
  else {
    if ((hours == hours_rassvet1_LED1)&&(minutes == minutes_rassvet1_LED1)) { 
      rassvet1_LED1 = true;       // Если время соответствует началу рассвета, то устанавливаем флаг рассвета
    }
  }
  //ДНЕВНОЙ ЗАКАТ И РАССВЕТ будет в случае если ЧАС дневного заката БОЛЬШЕ чем ЧАС утреннего рассвета
  if (hours_zakat1_LED1 > hours_rassvet1_LED1) { 
    //----- Обрабатываем первый закат (дневной)
    if (zakat1_LED1) {    
      duration = (hours-hours_zakat1_LED1)*hr + (minutes-minutes_zakat1_LED1)*mn + seconds;
      if (duration < (duration_zakat1_LED1*mn)) {
        pwm_LED = PWM_LED1_MAX-((PWM_LED1_MAX*duration)/(duration_zakat1_LED1*mn));
        if (pwm_LED < PWM_LED1_MIN_DAY) {
          pwm_LED = PWM_LED1_MIN_DAY;
        }
      }
      else {
        zakat1_LED1 = false;        // Закат закончился
        pwm_LED = PWM_LED1_MIN_DAY; // Устанавливаем минимальное значение для ШИМ
      }
      analogWrite(LED1_PIN, pwm_LED);
      bright_LED1 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
    }
    else {
      if ((hours == hours_zakat1_LED1)&&(minutes == minutes_zakat1_LED1)) { 
        zakat1_LED1 = true;         // Если время соответствует началу заката, то устанавливаем флаг заката
      }
    }
    //----- Обрабатываем второй рассвет (дневной)
    if (rassvet2_LED1) {    
      duration = (hours-hours_rassvet2_LED1)*hr + (minutes-minutes_rassvet2_LED1)*mn + seconds;
      if (duration < (duration_rassvet2_LED1*mn)) {
        pwm_LED = (PWM_LED1_MAX*duration)/(duration_rassvet2_LED1*mn);
        if (pwm_LED < PWM_LED1_MIN_DAY) {
          pwm_LED = PWM_LED1_MIN_DAY;
        }
      }
      else {
        rassvet2_LED1 = false;     // Рассвет закончился
        pwm_LED = PWM_LED1_MAX;    // Устанавливаем максимальное значение для ШИМ
      }
      analogWrite(LED1_PIN, pwm_LED);
      bright_LED1 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
    }
    else {
      if ((hours == hours_rassvet2_LED1)&&(minutes == minutes_rassvet2_LED1)) { 
        rassvet2_LED1 = true;       // Если время соответствует началу рассвета, то устанавливаем флаг рассвета
      }
    }
  }
  //----- Обрабатываем второй закат (вечерний)
  if (zakat2_LED1) {    
    duration = (hours-hours_zakat2_LED1)*hr + (minutes-minutes_zakat2_LED1)*mn + seconds;
    if (duration < (duration_zakat2_LED1*mn)) {
      pwm_LED = PWM_LED1_MAX-((PWM_LED1_MAX*duration)/(duration_zakat2_LED1*mn));
      if (pwm_LED < PWM_LED1_MIN) {
        pwm_LED = PWM_LED1_MIN;
      }
    }
    else {
      zakat2_LED1 = false;        // Закат закончился
      pwm_LED = PWM_LED1_MIN;     // Устанавливаем минимальное значение для ШИМ
    }
    analogWrite(LED1_PIN, pwm_LED);
    bright_LED1 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
  }
  else {
    if ((hours == hours_zakat2_LED1)&&(minutes == minutes_zakat2_LED1)) { 
      zakat2_LED1 = true;         // Если время соответствует началу заката, то устанавливаем флаг заката
    }
  }
}
//============================== Второй таймер для LED2 (белый свет)
void timerLED2()
{
  byte pwm_LED;         // Значение для записи в порт ШИМ от 0 до 255
  long duration;         // Текущая длительность с момента начала рассвета в секундах
  //----- Обрабатываем первый рассвет (утренний)
  if (rassvet1_LED2) {    
    duration = (hours-hours_rassvet1_LED2)*hr + (minutes-minutes_rassvet1_LED2)*mn + seconds;
    if (duration < (duration_rassvet1_LED2*mn)) {
      pwm_LED = (PWM_LED2_MAX*duration)/(duration_rassvet1_LED2*mn);
      if (pwm_LED < PWM_LED2_MIN) {
        pwm_LED = PWM_LED2_MIN;
      }
    }
    else {
      rassvet1_LED2 = false;     // Рассвет закончился
      pwm_LED = PWM_LED2_MAX;    // Устанавливаем максимальное значение для ШИМ
    }
    analogWrite(LED2_PIN, pwm_LED);
    bright_LED2 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
  }
  else {
    if ((hours == hours_rassvet1_LED2)&&(minutes == minutes_rassvet1_LED2)) { 
      rassvet1_LED2 = true;       // Если время соответствует началу рассвета, то устанавливаем флаг рассвета
    }
  }
  //ДНЕВНОЙ ЗАКАТ И РАССВЕТ будет в случае если ЧАС дневного заката БОЛЬШЕ чем ЧАС утреннего рассвета
  if (hours_zakat1_LED2 > hours_rassvet1_LED2) { 
    //----- Обрабатываем первый закат (дневной)
    if (zakat1_LED2) {    
      duration = (hours-hours_zakat1_LED2)*hr + (minutes-minutes_zakat1_LED2)*mn + seconds;
      if (duration < (duration_zakat1_LED2*mn)) {
        pwm_LED = PWM_LED2_MAX-((PWM_LED2_MAX*duration)/(duration_zakat1_LED2*mn));
        if (pwm_LED < PWM_LED2_MIN_DAY) {
          pwm_LED = PWM_LED2_MIN_DAY;
        }
      }
      else {
        zakat1_LED2 = false;        // Закат закончился
        pwm_LED = PWM_LED2_MIN_DAY; // Устанавливаем минимальное значение для ШИМ
      }
      analogWrite(LED2_PIN, pwm_LED);
      bright_LED2 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
    }
    else {
      if ((hours == hours_zakat1_LED2)&&(minutes == minutes_zakat1_LED2)) { 
        zakat1_LED2 = true;         // Если время соответствует началу заката, то устанавливаем флаг заката
      }
    }
    //----- Обрабатываем второй рассвет (дневной)
    if (rassvet2_LED2) {    
      duration = (hours-hours_rassvet2_LED2)*hr + (minutes-minutes_rassvet2_LED2)*mn + seconds;
      if (duration < (duration_rassvet2_LED2*mn)) {
        pwm_LED = (PWM_LED2_MAX*duration)/(duration_rassvet2_LED2*mn);
        if (pwm_LED < PWM_LED2_MIN_DAY) {
          pwm_LED = PWM_LED2_MIN_DAY;
        }
      }
      else {
        rassvet2_LED2 = false;     // Рассвет закончился
        pwm_LED = PWM_LED2_MAX;    // Устанавливаем максимальное значение для ШИМ
      }
      analogWrite(LED2_PIN, pwm_LED);
      bright_LED2 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
    }
    else {
      if ((hours == hours_rassvet2_LED2)&&(minutes == minutes_rassvet2_LED2)) { 
        rassvet2_LED2 = true;       // Если время соответствует началу рассвета, то устанавливаем флаг рассвета
      }
    }
  }
  //----- Обрабатываем второй закат (вечерний)
  if (zakat2_LED2) {    
    duration = abs((hours-hours_zakat2_LED2)*hr + (minutes-minutes_zakat2_LED2)*mn + seconds);
    if (duration < (duration_zakat2_LED2*mn)) {
      pwm_LED = PWM_LED2_MAX-((PWM_LED2_MAX*duration)/(duration_zakat2_LED2*mn));
      if (pwm_LED < PWM_LED2_MIN) {
        pwm_LED = PWM_LED2_MIN;
      }
    }
    else {
      zakat2_LED2 = false;        // Закат закончился
      pwm_LED = PWM_LED2_MIN;     // Устанавливаем минимальное значение для ШИМ
    }
    analogWrite(LED2_PIN, pwm_LED);
    bright_LED2 = map(pwm_LED, 0, 255, 0, 100); // Вычисляем значение текущей яркости
  }
  else {
    if ((hours == hours_zakat2_LED2)&&(minutes == minutes_zakat2_LED2)) { 
      zakat2_LED2 = true;         // Если время соответствует началу заката, то устанавливаем флаг заката
    }
  }
}
//======== Метод производящий включение-выключение люминесцентных ламп
void setLAMP(bool state) {
  if (state == OFF) {                 // если OFF
    digitalWrite(LAMP_PIN, HIGH);     // то выключаем лампы
    LAMP_ON = false;                  // и сбрасываем флаг включенных ламп в false
  }
  else {                              // если ON
    digitalWrite(LAMP_PIN, LOW);      // то включаем лампы
    LAMP_ON = true;                   // и устанавливаем флаг включенных ламп в true
  }
}
//======== Метод производящий включение-выключение CO2 (или чего-то еще, например аэрация)
void setCO2(bool state) {
  if (state == OFF) {                // если OFF
    digitalWrite(CO2_PIN, HIGH);     // то выключаем CO2
    CO2_ON = false;                  // и сбрасываем флаг включения в false
  }
  else {                             // если ON
    digitalWrite(CO2_PIN, LOW);      // то включаем CO2
    CO2_ON = true;                   // и устанавливаем флаг включения в true
  }
}
//============================== Третий таймер для LAMP (люминесцентные лампы)
void timerLAMP()
{
  //----- Обрабатываем первый интервал включения-выключения.
  // Если час и минута включения равны часу и минуте выключения, значит этот интервал обрабатываться не будет.
  if ((hours_ON1_LAMP != hours_OFF1_LAMP) || (minutes_ON1_LAMP != minutes_OFF1_LAMP)) {
    if ((hours == hours_ON1_LAMP)&&(minutes == minutes_ON1_LAMP)) { 
      if (!LAMP_ON) setLAMP(ON);   // Если наступило время включения ламп, то включаем и устанавливаем флаг включения
    }
    if ((hours == hours_OFF1_LAMP)&&(minutes == minutes_OFF1_LAMP)) { 
      if (LAMP_ON) setLAMP(OFF); // Если наступило время выключения ламп, то выключаем и устанавливаем флаг выключения
    }
  }
  //----- Обрабатываем второй интервал включения-выключения.
  // Если час и минута включения равны часу и минуте выключения, значит этот интервал обрабатываться не будет.
  if ((hours_ON2_LAMP != hours_OFF2_LAMP) || (minutes_ON2_LAMP != minutes_OFF2_LAMP)) {
    if ((hours == hours_ON2_LAMP)&&(minutes == minutes_ON2_LAMP)) { 
      if (!LAMP_ON) setLAMP(ON);   // Если наступило время включения ламп, то включаем и устанавливаем флаг включения
    }
    if ((hours == hours_OFF2_LAMP)&&(minutes == minutes_OFF2_LAMP)) { 
      if (LAMP_ON) setLAMP(OFF); // Если наступило время выключения ламп, то выключаем и устанавливаем флаг выключения
    }
  }
}
//============================== Четвертый таймер для СО2 (или чего-то еще, например аэрация)
void timerCO2()
{
  //----- Обрабатываем первый интервал включения-выключения.
  // Если час и минута включения равны часу и минуте выключения, значит этот интервал обрабатываться не будет.
  if ((hours_ON1_CO2 != hours_OFF1_CO2) || (minutes_ON1_CO2 != minutes_OFF1_CO2)) {
    if ((hours == hours_ON1_CO2)&&(minutes == minutes_ON1_CO2)) { 
      if (!CO2_ON) setCO2(ON);   // Если наступило время включения CO2, то включаем и устанавливаем флаг включения
    }
    if ((hours == hours_OFF1_CO2)&&(minutes == minutes_OFF1_CO2)) { 
      if (CO2_ON) setCO2(OFF); // Если наступило время выключения CO2, то выключаем и устанавливаем флаг выключения
    }
  }
  //----- Обрабатываем второй интервал включения-выключения.
  // Если час и минута включения равны часу и минуте выключения, значит этот интервал обрабатываться не будет.
  if ((hours_ON2_CO2 != hours_OFF2_CO2) || (minutes_ON2_CO2 != minutes_OFF2_CO2)) {
    if ((hours == hours_ON2_CO2)&&(minutes == minutes_ON2_CO2)) { 
      if (!CO2_ON) setCO2(ON);   // Если наступило время включения CO2, то включаем и устанавливаем флаг включения
    }
    if ((hours == hours_OFF2_CO2)&&(minutes == minutes_OFF2_CO2)) { 
      if (CO2_ON) setCO2(OFF); // Если наступило время выключения CO2, то выключаем и устанавливаем флаг выключения
    }
  }
}
///
//=============================== Переменные системы обогрева аквариума и фильтрации
bool HEATER_ON;                     // флаг включения системы обогрева аквариума
byte temp_AVR;                      // заданная средняя температура воды аквариума
float hysteresis = 0.5f;            // гистерезис температур в большую или меньшую сторону
bool FILTER_ON;                     // флаг включения системы фильтрации аквариума
//=============================== Таймер подачи удобрения МИКРО



/*#define NOTE_C4  262
#define NOTE_D4  294
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_G4  392
#define NOTE_A4  440
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_D5  587
#define NOTE_E5  659
*/
#define NOTE_C6  1047
//#define NOTE_CS6 1109
#define NOTE_D6  1175
//#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
//#define NOTE_FS6 1480
#define NOTE_G6  1568
//#define NOTE_GS6 1661
#define NOTE_A6  1760
//#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
//#define NOTE_CS7 2217
#define NOTE_D7  2349
//#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_2000  2000
int melody[] = { NOTE_E7, NOTE_D7, NOTE_F6, NOTE_G6, NOTE_C7, NOTE_B6, NOTE_D6, NOTE_E6, NOTE_B6, NOTE_A6, NOTE_C6, NOTE_E6, NOTE_A6, NOTE_2000 };
int noteDurations[] = { 8, 8, 4, 4, 8, 8, 4, 4, 8, 8, 4, 4, 1, 2 };
int amountNotes = 14;
//======== Метод производящий подачу звукового оповещения при срабатывании будильника
void beep() {
  for (int thisNote = 0; thisNote < amountNotes; thisNote++) {
    int noteDuration = 1000/noteDurations[thisNote];
    tone(BUZZER_PIN, melody[thisNote], noteDuration);
    //int pauseBetweenNotes = noteDuration * 1.30;
    delay(noteDuration * 1.30);
    noTone(BUZZER_PIN);
  }
}
//======== Метод производящий подачу звукового оповещения при включении фильтра
void beepON() {
   tone(BUZZER_PIN, 2000, 300); // издаем звук частотой 2000 Гц и длительностью 300 мс.
   delay(500);
}
//======== Метод производящий подачу звукового оповещения при выключении фильтра
void beepOFF() {
   tone(BUZZER_PIN, 3500, 300);
   delay(500);
}
//======== Метод производящий включение-выключение системы обогрева аквариума
void setHEATER(bool state) {
  if (state == OFF) {                 // если OFF
    digitalWrite(HEATER_PIN, HIGH);   // то выключаем обогреватель
    HEATER_ON = false;                // и сбрасываем флаг включенного обогрева в false
  }
  else {                              // если ON
    digitalWrite(HEATER_PIN, LOW);    // то включаем обогреватель
    HEATER_ON = true;                 // и устанавливаем флаг включенного обогрева в true
  }
}
//======== Метод производящий включение-выключение системы фильтрации аквариума
void setFILTER(bool state) {
  if (state == OFF) {                 // если OFF
    digitalWrite(FILTER_PIN, LOW);   // то выключаем фильтр
    FILTER_ON = false;                // и сбрасываем флаг включенного фильтра в false
    beepOFF();                        // подаем звуковой сигнал
  }
  else {                              // если ON
    digitalWrite(FILTER_PIN, HIGH);    // то включаем фильтр
    FILTER_ON = true;                 // и устанавливаем флаг включенного фильтра в true
    beepON();                         // подаем звуковой сигнал
  }
}
//======== Метод производящий включение-выключение системы обогрева аквариума в зависимости от температуры воды
void HEATER() {
  // Если текущая температура отличается от предыдцщей меньше чем на 10 градусов, то обрабатываем, иначе нет.
  if (abs(temp-temp_pred)<10) {
    if (HEATER_ON) {
      if (temp > (temp_AVR+hysteresis)) { // если текущая температура больше средней заданной + гистерезис,
        setHEATER(OFF);                   // то выключаем обогреватель и сбрасываем флаг включенного обогрева в false
      }
    }
    else {
      if (temp < (temp_AVR-hysteresis)) { // если текущая температура меньше средней заданной + гистерезис,
        setHEATER(ON);                    // то включаем обогреватель и устанавливаем флаг включенного обогрева в true
      }
    }
  }
}
//=============================== Переменные системы кормления
bool FEEDING1;             // флаг 1 кормления
bool FEEDING2;             // флаг 2 кормления
byte hours_feed1;         // час начала первого кормления (утреннего)
byte minutes_feed1;       // минута начала первого кормления (утреннего)
byte duration_feed1;      // длительность первого кормления (утреннего) (в минутах)
byte hours_feed2;         // час начала второго кормления (вечернего)
byte minutes_feed2;       // минута начала второго кормления (вечернего)
byte duration_feed2;      // длительность первого кормления (утреннего) (в минутах)
//=============================== Переменные 2х Будильников
bool ALARM1;              // флаг 1 будильника
bool ALARM2;              // флаг 2 будильника
byte hours_alarm1;        // час первого будильника
byte minutes_alarm1;      // минута первого будильника
byte duration_alarm1;     // длительность звучания первого будильника (в секундах)
byte hours_alarm2;        // час второго будильника
byte minutes_alarm2;      // минута второго будильника
byte duration_alarm2;     // длительность звучания второго будильника (в секундах)
//======== Метод производящий подачу корма в аквариум
void feeding() {
   
}
//============================== Таймер для системы кормления
void timerFeeding() {
  long duration;           // Текущая длительность с момента начала кормления в секундах
  //----- Обрабатываем первое кормление (утреннее)
  if (hours_feed1 != 0) { // Если час первого кормления равен нулю, то кормления не будет (выкл)
    if (FEEDING1) {    
      duration = (hours-hours_feed1)*hr + (minutes-minutes_feed1)*mn + seconds;
      if (duration > (duration_feed1*mn)) {  // если текущаяя длительность больше заданной, то
        FEEDING1 = false;     // Кормление закончилось
        setFILTER(ON);        // включаем фильтр
      }
    }
    else {
      if ((hours == hours_feed1)&&(minutes == minutes_feed1)) { 
        FEEDING1 = true;      // Если время соответствует началу кормления, то устанавливаем флаг кормления,
        setFILTER(OFF);       // выключаем фильтр
        delay(30000);         // и ждем 30 секунд, пока вода в аквариуме успокоится
        feeding();            // и затем подаем корм рыбам.
      }
    }
  }
  //----- Обрабатываем второе кормление (вечернее)
  if (hours_feed2 != 0) { // Если час второго кормления равен нулю, то кормления не будет (выкл)
    if (FEEDING2) {    
      duration = (hours-hours_feed2)*hr + (minutes-minutes_feed2)*mn + seconds;
      if (duration > (duration_feed2*mn)) {  // если текущаяя длительность больше заданной, то
        FEEDING2 = false;     // Кормление закончилось
        setFILTER(ON);        // включаем фильтр
      }
    }
    else {
      if ((hours == hours_feed2)&&(minutes == minutes_feed2)) { 
        FEEDING2 = true;      // Если время соответствует началу кормления, то устанавливаем флаг кормления,
        setFILTER(OFF);       // выключаем фильтр
        delay(30000);         // и ждем 30 секунд, пока вода в аквариуме успокоится
        feeding();            // и затем подаем корм рыбам.
      }
    }
  }
}
void showAlarmOFF() {
  ClearDisplay();
  lcd.setCursor(0,1);
  lcd.print("   -=ALARM STOP=-");
  delay(5000);
  ClearDisplay();  
}
//============================== Таймер для Будильников
void timerAlarm() {
  //----- Обрабатываем первый будильник
  if (hours_alarm1 != 0) { // Если час первого будильника равен нулю, то сигнала не будет (выкл)
    if ((hours == hours_alarm1)&&(minutes == minutes_alarm1)) {    
      if (!ALARM1) {
        for (int i = 0; i < duration_alarm1; i++) {
          delay(3000);
          beep();          // Подаем звуковой сигнал i количество раз в цикле
          if (getPressedButton() != BUTTON_NONE) { // Если нажата любая кнопка, то выходим из цикла
            showAlarmOFF();
            break;
          }
        }
        ALARM1 = true;     
      }
    }
    else {
      if (ALARM1) { 
        ALARM1 = false;   
      }
    }
  }
  //----- Обрабатываем второй будильник
  if (hours_alarm2 != 0) { // Если час второго будильника равен нулю, то сигнала не будет (выкл)
    if ((hours == hours_alarm2)&&(minutes == minutes_alarm2)) {    
      if (!ALARM2) {
        for (int i = 0; i < duration_alarm2; i++) {
          delay(3000);
          beep();          // Подаем звуковой сигнал i количество раз в цикле
          if (getPressedButton() != BUTTON_NONE) { // Если нажата любая кнопка, то выходим из цикла
            showAlarmOFF();
            break;
          }
        }
        ALARM2 = true;     
      }
    }
    else {
      if (ALARM2) { 
        ALARM2 = false;   
      }
    }
  }
}
//=============================== Переменные для навигации по меню и выводу на дисплей информации
byte buttonPressed;               // Код нажатой кнопки
byte pMenu;                       // Пункт меню (номер меню)
byte pos;                         // Позиция текущего изменяемого значения в этом пункте меню
String display0;                  // Первая строка дисплея
String display1;                  // Вторая строка дисплея
String display2;                  // Третья строка дисплея
String display3;                  // Четвертая строка дисплея
//=============================== Метод отвечающий за очистку дисплея и переменных дисплея
void ClearDisplay() {
  lcd.clear();                                    // Очищаем дисплей
  display0 = display1 = display2 = display3 = ""; // Стираем строки дисплея (переменные)
  pos = 0;                                        // Сбрасываем позицию изменяемого значения на начальную
}
//=============================== Метод отвечающий за вывод информации на дисплей.
void PrintDisplay(String mdisp0, String mdisp1, String mdisp2, String mdisp3) {
  //while (mdisp0.length()<19) mdisp0.concat(" "); // Удлинняем строки до 20 символов
  //while (mdisp1.length()<19) mdisp1.concat(" ");
  //while (mdisp2.length()<19) mdisp2.concat(" ");
  //while (mdisp3.length()<19) mdisp3.concat(" ");
  if (display0 != mdisp0) {          // Если содержимое строки изменилось, то выводим ее на дисплей
    display0 = mdisp0;
    lcd.setCursor(0,0);
    lcd.print(display0);
  }
  if (display1 != mdisp1) {
    display1 = mdisp1;
    lcd.setCursor(0,1);
    lcd.print(display1);
  }
  if (display2 != mdisp2) {
    display2 = mdisp2;
    lcd.setCursor(0,2);
    lcd.print(display2);
  }
  if (display3 != mdisp3) {
    display3 = mdisp3;
    lcd.setCursor(0,3);
    lcd.print(display3);
  }
}
//=============================== Метод отвечающий за установку часов реального времени
void SetTime() {
  int plus_minus;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  //String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  //String mdisplay3;       // Четвертая строка дисплея
  delay(500);
  while(buttonPressed != BUTTON_MENU) {
    buttonPressed = getPressedButton();               // Считываем нажатую кнопку
    plus_minus = 0;
    switch(buttonPressed){
      case BUTTON_UP:   plus_minus = 1;  break;
      case BUTTON_DOWN: plus_minus = -1; break;
      case BUTTON_OK:
        pos++;              // Переходим к следующему изменяемому значению
        if (pos>7) pos = 0; // и по кругу
      break;    
    }
    switch (pos) {
      case 0:
        date += plus_minus;              // Крутим значения длительности
        if (date > 31) date = 1;         // Ограничиваем значения
        msg = "    "; msg += "<<Set"; msg += " Date"; msg += ">>"; msg += "    "; // 20
        //msg = "    <<Set Date>>    ";
      break;
      case 1:
        month += plus_minus;             // Крутим значения длительности
        if (month > 12) month = 1;       // Ограничиваем значения
        msg = "   "; msg += "<<Set"; msg += " Month"; msg += ">>"; msg += "    "; // 20
        //msg = "   <<Set Month>>    ";
      break;
      case 2:
        year += plus_minus;              // Крутим значения длительности
        if (year > 99) year = 0;         // Ограничиваем значения
        msg = "    "; msg += "<<Set"; msg += " Year"; msg += ">>"; msg += "    "; // 20
        //msg = "    <<Set Year>>    ";
      break;
      case 3:
        day += plus_minus;               // Крутим значения длительности
        if (day > 7) day = 1;            // Ограничиваем значения
        if (day < 1) day = 7;            // Ограничиваем значения
        msg = "    "; msg += "<<Set"; msg += " Day"; msg += ">>"; msg += "    "; // 19
        //msg = "    <<Set Day>>     ";
      break;
      case 4:
        hours += plus_minus;             // Крутим значения часов
        if (hours > 23) hours = 0;       // Ограничиваем значения
        msg = "   "; msg += "<<Set"; msg += " Hours"; msg += ">>"; msg += "    "; // 20
        //msg = "   <<Set Hours>>    ";
      break;
      case 5:
        minutes += plus_minus;           // Крутим значения минут
        if (minutes > 59) minutes = 0;   // Ограничиваем значения
        msg = "  "; msg += "<<Set"; msg += " Minutes"; msg += ">>"; msg += "   "; // 20
        //msg = "  <<Set Minutes>>   ";
      break;
      case 6:
        seconds += plus_minus;           // Крутим значения длительности
        if (seconds > 59) seconds = 0;   // Ограничиваем значения
        msg = "  "; msg += "<<Set"; msg += " Seconds"; msg += ">>"; msg += "   "; // 20
        //msg = "  <<Set Seconds>>   ";
      break;
      case 7:
        setRtc(seconds, minutes, hours, day, date, month, year);  // Устанавливаем часы и дату
        lcd.clear();
        lcd.setCursor(0,2);
        lcd.print("      -=Save"); lcd.print("d"); lcd.print("=- "); // lcd.print("    ");
        lcd.setCursor(0,3);
        lcd.print("      -=Save"); lcd.print("d"); lcd.print("=- "); // lcd.print("    ");
        buttonPressed = BUTTON_MENU;
        delay(2000);
        continue;
      break;   
    }
    lcd.setCursor(0,0);
    lcd.print("Set Date  / Set Time");   //20                 // Формируем строки с информацией для вывода на дисплей
    mdisplay1 = ByteToStrFormat(date, 2) + "/" + ByteToStrFormat(month, 2) + "/20" +  ByteToStrFormat(year, 2) + "  ";  //12
    mdisplay1 +=  ByteToStrFormat(hours, 2) + ":" + ByteToStrFormat(minutes, 2) + ":" + ByteToStrFormat(seconds, 2);  //8
    mdisplay2 = "Day=" + ByteToStrFormat(day, 1) + "      -=Save" + "=- ";
    lcd.setCursor(0,1);       // Выводим строки на дисплей
    lcd.print(mdisplay1);
    lcd.setCursor(0,2);
    lcd.print(mdisplay2);
    lcd.setCursor(0,3);
    lcd.print(msg);
    //PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
  } 
}
void Menu_UDO_Test() {
  int plus_minus;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  
   
  delay(500);
  mdisplay1 = "    "; mdisplay1 += "-=UDO TEST=-"; mdisplay1 += "    ";   //20
  lcd.setCursor(0,1);       // Выводим строки на дисплей
  lcd.print(mdisplay1);
  delay(3000);
  ClearDisplay();       // Очищаем дисплей и переменные дисплея
   
  
  ClearDisplay();       // Очищаем дисплей и переменные дисплея
  mdisplay1 = "    "; mdisplay1 += "-=UDO TEST=-"; mdisplay1 += "    ";   //20
  mdisplay2 = "    "; mdisplay2 += "    "; mdisplay2 += "EXIT"; mdisplay2 += "    "; mdisplay2 += "    ";   //20
  lcd.setCursor(0,1);       // Выводим строки на дисплей
  lcd.print(mdisplay1);
  lcd.setCursor(0,2);       // Выводим строки на дисплей
  lcd.print(mdisplay2);
  delay(3000);
  pMenu = 0;
}
//=============================== Метод отвечающий за режим профилактики (ручная настройка выходов реле 220В)
void SetProfilactika() {
  int plus_minus;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
   
  mdisplay1 = "    "; mdisplay1 += "Profilactika"; mdisplay1 += "    ";   //20
  lcd.setCursor(0,1);       // Выводим строки на дисплей
  lcd.print(mdisplay1);
  delay(3000);
  ClearDisplay();       // Очищаем дисплей и переменные дисплея
  while(buttonPressed != BUTTON_MENU) {
    buttonPressed = getPressedButton();               // Считываем нажатую кнопку
    plus_minus = 0;
    switch(buttonPressed){
      case BUTTON_UP:   plus_minus = 1;  break;
      case BUTTON_DOWN: plus_minus = -1; break;
      case BUTTON_OK:
        pos++;              // Переходим к следующему изменяемому значению
        if (pos>5) pos = 0; // и по кругу
      break;    
    }
    switch (pos) {
      case 0:
        if (plus_minus == 1) {
          rassvet1_LED1 = true;
          timerLED1();
        }
        else if (plus_minus == -1) {
          zakat2_LED1 = true;
          timerLED1();
        }
        msg = "    "; msg += "<<Set"; msg += " LED1"; msg += ">>"; msg += "    "; // 20
      break;
      case 1:
        if (plus_minus == 1) {
          rassvet1_LED2 = true;
          timerLED2();
        }
        else if (plus_minus == -1) {
          zakat2_LED2 = true;
          timerLED2();
        }
        msg = "    "; msg += "<<Set"; msg += " LED2"; msg += ">>"; msg += "    "; // 20
      break;
      case 2:
        if (plus_minus == 1) {
          if (!LAMP_ON) setLAMP(ON);
        }
        else if (plus_minus == -1) {
          if (LAMP_ON) setLAMP(OFF);
        }
        msg = "  "; msg += "<<Set"; msg += " LAMP LUM"; msg += ">>"; msg += "  "; // 20
      break;
      case 3:
        if (plus_minus == 1) {
          if (!FILTER_ON) setFILTER(ON);
        }
        else if (plus_minus == -1) {
          if (FILTER_ON) setFILTER(OFF);
        }
        msg = "   "; msg += "<<Set"; msg += " FILTER"; msg += ">>"; msg += "   "; // 20
      break;
      case 4:
          if (plus_minus == 1) {
          if (!HEATER_ON) setHEATER(ON);
        }
        else if (plus_minus == -1) {
          if (HEATER_ON) setHEATER(OFF);
        }
        msg = "   "; msg += "<<Set"; msg += " HEATER"; msg += ">>"; msg += "   "; // 20
      break;
      case 5:
          if (plus_minus == 1) {
          if (!CO2_ON) setCO2(ON);
        }
        else if (plus_minus == -1) {
          if (CO2_ON) setCO2(OFF);
        }
        msg = "    "; msg += "<<Set"; msg += " CO2"; msg += ">>"; msg += "    "; // 19
      break;   
    }
    //lcd.setCursor(0,0);
    //lcd.print("    Profilactika    ");   //20                 // Формируем строки с информацией для вывода на дисплей
    mdisplay0 = "LED1=" + ByteToStrFormat(bright_LED1, 3) + "% " + "LED2=" + ByteToStrFormat(bright_LED2, 3) + "% "; //20
    mdisplay1 = (LAMP_ON == true) ? "LAMP=ON  " : "LAMP=OFF ";  //9
    mdisplay1 += (FILTER_ON == true) ? "Filter=ON " : "Filter=OFF"; //10
    mdisplay2 = (HEATER_ON == true) ? "Heater=ON " : "Heater=OFF"; //10
    mdisplay2 += (CO2_ON == true) ? " CO2=ON  " : " CO2=OFF "; //9
    mdisplay3 = msg;
    /*lcd.setCursor(0,0);
    lcd.print(mdisplay0);
    lcd.setCursor(0,1);       // Выводим строки на дисплей
    lcd.print(mdisplay1);
    lcd.setCursor(0,2);
    lcd.print(mdisplay2);
    lcd.setCursor(0,3);
    lcd.print(msg);*/
    PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
    delay(100);
  }
  ClearDisplay();       // Очищаем дисплей и переменные дисплея
  mdisplay1 = "    "; mdisplay1 += "Profilactika"; mdisplay1 += "    ";   //20
  mdisplay2 = "    "; mdisplay2 += "    "; mdisplay2 += "EXIT"; mdisplay2 += "    "; mdisplay2 += "    ";   //20
  lcd.setCursor(0,1);       // Выводим строки на дисплей
  lcd.print(mdisplay1);
  lcd.setCursor(0,2);       // Выводим строки на дисплей
  lcd.print(mdisplay2);
  delay(3000);
  pMenu = 0; 
}
//=============================== Метод отвечающий за навигацию по меню контроллера
void Menu() {
  buttonPressed = getPressedButton();               // Считываем нажатую кнопку
  if ((pMenu == 0)&&(buttonPressed != BUTTON_NONE)) // Нулевое меню - вывод информации о работе контроллера
    switch(buttonPressed){
        case BUTTON_MENU:
          pMenu++;              // Переходим к следующему меню
          ClearDisplay();       // Очищаем дисплей и переменные дисплея
          ShowSetMenu();
        break;
        case BUTTON_UP:
          ClearDisplay();       // Очищаем дисплей и переменные дисплея
          SetTime();            // Установка часов реального времени
        break;
        case BUTTON_DOWN:
          ClearDisplay();       // Очищаем дисплей и переменные дисплея
          SetProfilactika();    // Переходим в режим профилактики для чистки аквариума
        break;
        case BUTTON_OK:
          ShowVersion();
          ClearDisplay();       // Очищаем дисплей и переменные дисплея
          ShowSetMenu();
        break;
  }
  else if (buttonPressed == BUTTON_MENU) {
    pMenu++;                    // Переходим к следующему меню
    if (pMenu > 20) pMenu = 0;  // Если прошлись по всем меню, то пререходим в информационное меню 
    ClearDisplay();             // Очищаем дисплей и переменные дисплея
    ShowSetMenu();
  }
  else if (buttonPressed == BUTTON_NONE) {
    ShowSetMenu();
    return;
  }
  else
    ShowSetMenu();
  if (pMenu>0) delay(100);
}
void ShowSetMenu() {
    String s;
    switch (pMenu) {
      case 0:  Menu_INFO();
      break;
      case 1:  
        s = "Menu"; s += "0"; s += "1"; s += " LED"; s += "1"; s += " Rassvet"; s += "1"; // "Menu01 LED1 Rassvet1"
        Menu_LED_UDO(s, hours_rassvet1_LED1, minutes_rassvet1_LED1, duration_rassvet1_LED1, " Minutes");
      break; 
      case 2:  
        s = "Menu"; s += "0"; s += "2"; s += " LED"; s += "1"; s += " Zakat"; s += "1"; // "Menu02 LED1 Zakat1"
        Menu_LED_UDO(s, hours_zakat1_LED1, minutes_zakat1_LED1, duration_zakat1_LED1, " Minutes");
      break;
      case 3:  
        s = "Menu"; s += "0"; s += "3"; s += " LED"; s += "1"; s += " Rassvet"; s += "2"; // "Menu03 LED1 Rassvet2"
        Menu_LED_UDO(s, hours_rassvet2_LED1, minutes_rassvet2_LED1, duration_rassvet2_LED1, " Minutes");
      break; 
      case 4:  
        s = "Menu"; s += "0"; s += "4"; s += " LED"; s += "1"; s += " Zakat"; s += "2"; // "Menu04 LED1 Zakat2"
        Menu_LED_UDO(s, hours_zakat2_LED1, minutes_zakat2_LED1, duration_zakat2_LED1, " Minutes");
      break;
      case 5:  
        s = "Menu"; s += "0"; s += "5"; s += " LED"; s += "1"; s += " PWM Sett";  // "Menu05 LED1 PWM Sett"
        Menu_LED_Add(s, PWM_LED1_MIN, PWM_LED1_MAX, PWM_LED1_MIN_DAY); 
      break;
      case 6:  
        s = "Menu"; s += "0"; s += "6"; s += " LED"; s += "2"; s += " Rassvet"; s += "1"; // "Menu06 LED2 Rassvet1"
        Menu_LED_UDO(s, hours_rassvet1_LED2, minutes_rassvet1_LED2, duration_rassvet1_LED2, " Minutes");
      break; 
      case 7:
        s = "Menu"; s += "0"; s += "7"; s += " LED"; s += "2"; s += " Zakat"; s += "1"; // "Menu07 LED2 Zakat1" 
        Menu_LED_UDO(s, hours_zakat1_LED2, minutes_zakat1_LED2, duration_zakat1_LED2, " Minutes");
      break;
      case 8:
        s = "Menu"; s += "0"; s += "8"; s += " LED"; s += "2"; s += " Rassvet"; s += "2"; // "Menu08 LED2 Rassvet2"  
        Menu_LED_UDO(s, hours_rassvet2_LED2, minutes_rassvet2_LED2, duration_rassvet2_LED2, " Minutes");
      break; 
      case 9:  
        s = "Menu"; s += "0"; s += "9"; s += " LED"; s += "2"; s += " Zakat"; s += "2"; // "Menu09 LED2 Zakat2"
        Menu_LED_UDO(s, hours_zakat2_LED2, minutes_zakat2_LED2, duration_zakat2_LED2, " Minutes");
      break;
      case 10: 
        s = "Menu"; s += "1"; s += "0"; s += " LED"; s += "2"; s += " PWM Sett"; // "Menu10 LED2 PWM Sett"
        Menu_LED_Add(s, PWM_LED2_MIN, PWM_LED2_MAX, PWM_LED2_MIN_DAY); 
      break;
      case 11: 
        s = "Menu"; s += "1"; s += "1"; s += " LAMP"; s += " Period"; s += "1"; // "Menu11 LAMP Period1"
        Menu_LAMP_CO2(s, hours_ON1_LAMP, minutes_ON1_LAMP, hours_OFF1_LAMP, minutes_OFF1_LAMP);
      break;
      case 12: 
        s = "Menu"; s += "1"; s += "2"; s += " LAMP"; s += " Period"; s += "2";  // "Menu12 LAMP Period2"
        Menu_LAMP_CO2(s, hours_ON2_LAMP, minutes_ON2_LAMP, hours_OFF2_LAMP, minutes_OFF2_LAMP);
      break;
      case 13: 
        s = "Menu"; s += "1"; s += "3"; s += " CO2"; s += " Period"; s += "1"; // "Menu13 CO2 Period1"
        Menu_LAMP_CO2(s, hours_ON1_CO2, minutes_ON1_CO2, hours_OFF1_CO2, minutes_OFF1_CO2);
      break;
      case 14: 
        s = "Menu"; s += "1"; s += "4"; s += " CO2"; s += " Period"; s += "2";  // "Menu14 CO2 Period2"
        Menu_LAMP_CO2(s, hours_ON2_CO2, minutes_ON2_CO2, hours_OFF2_CO2, minutes_OFF2_CO2);
      break;
      case 15: 
        s = "Menu"; s += "1"; s += "5"; s += " Heater"; // "Menu15 Heater"
        Menu_HEATER(s, temp_AVR, hysteresis);
      break;
      case 16:  
        s = "Menu"; s += "1"; s += "6"; s += " Feeding "; s += "1"; // "Menu16 Feeding 1"
        Menu_LED_UDO(s, hours_feed1, minutes_feed1, duration_feed1, " Minutes");
      break; 
      case 17:  
        s = "Menu"; s += "1"; s += "7"; s += " Feeding "; s += "2"; // "Menu17 Feeding 2"
        Menu_LED_UDO(s, hours_feed2, minutes_feed2, duration_feed2, " Minutes");
      break;
      case 18:  
        s = "Menu"; s += "1"; s += "8"; s += " Alarm "; s += "1"; // "Menu18 Alarm 1"
        Menu_LED_UDO(s, hours_alarm1, minutes_alarm1, duration_alarm1, " Raz");
      break;
      case 19:  
        s = "Menu"; s += "1"; s += "9"; s += " Alarm "; s += "2"; // "Menu19 Alarm 2"
        Menu_LED_UDO(s, hours_alarm2, minutes_alarm2, duration_alarm2, " Raz");
      break;
      case 20: 
        s = "Menu"; s += "2"; s += "0"; s += " Save"; s += " Settings"; // "Menu20 Save Settings"
        Menu_Save(s);
      break;
    } 
}
//=============================== Метод отвечающий за вывод Информационного меню (часы, дата, свет, температура)
void Menu_INFO() {
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  String t;
  //mdisplay0 = "FUTURE HOME V1.1";      // Формируем строки с информацией для вывода на дисплей
  mdisplay0 = ByteToStrFormat(hours, 2) + ":" + ByteToStrFormat(minutes, 2) + ":" + ByteToStrFormat(seconds, 2) + "  ";  //10
  mdisplay0 += ByteToStrFormat(date, 2) + "/" + ByteToStrFormat(month, 2) + "/20" +  ByteToStrFormat(year, 2);  //10
  mdisplay1 = "LED1=" + ByteToStrFormat(bright_LED1, 3) + "% " + "LED2=" + ByteToStrFormat(bright_LED2, 3) + "% "; //20
  t = (String)temp;
  //t = t.substring(0,t.indexOf('.')+2);
  t = t.substring(0, 4);
  mdisplay2 = (LAMP_ON == true) ? "LAMP=ON  " : "LAMP=OFF ";  //9
  mdisplay2 += "Temp=" + t + "\1C"; //11
  mdisplay3 = (FILTER_ON == true) ? "Flr=ON " : "Flr=OF "; //7
  mdisplay3 += (HEATER_ON == true) ? "Htr=ON " : "Htr=OF "; //7
  mdisplay3 += (CO2_ON == true) ? "CO2=ON" : "CO2=OF"; //6
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
//============ Метод отвечающий за вывод меню настроек светодиодного освещения с ШИМ и меню настроек Подачи УДОбрений
void Menu_LED_UDO(String mName, byte &mHours, byte &mMinutes, byte &mDuration, String ed) {
  int plus_minus = 0;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  //Serial.print(mName);
  switch(buttonPressed){
    case BUTTON_UP:   plus_minus = 1;  break;
    case BUTTON_DOWN: plus_minus = -1; break;
    case BUTTON_OK:
      pos++;              // Переходим к следующему изменяемому значению
      if (pos>2) pos = 0; // и по кругу
    break;    
  }
  switch (pos) {
    case 0:
      mHours += plus_minus;             // Крутим значения часов
      if (mHours > 23) mHours = 0;      // Ограничиваем значения
      msg = "   "; msg += "<<Set"; msg += " Hours"; msg += ">>"; msg += "    "; // 20
    break;
    case 1:
      mMinutes += plus_minus;           // Крутим значения минут
      if (mMinutes > 59) mMinutes = 0;  // Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " Minutes"; msg += ">>"; msg += "   "; // 20
    break;
    case 2:
      mDuration += plus_minus;          // Крутим значения длительности
      //if (mDuration > 59) mDuration = 0;// Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " "; msg += "Duration"; msg += ">>"; msg += "  "; // 20
    break;   
  }
  mdisplay0 = mName;                    // Формируем строки с информацией для вывода на дисплей
  mdisplay1 = "Start Time: " + ByteToStrFormat(mHours, 2) + ":" + ByteToStrFormat(mMinutes, 2) + " ";
  mdisplay2 = "Duration"; mdisplay2 += " " + ByteToStrFormat(mDuration, 2) + ed;
  mdisplay3 = msg;
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
//=============================== Метод отвечающий за вывод меню дополнительных настроек светодиодного освещения с ШИМ
void Menu_LED_Add(String mName, byte &mPWM_LED_MIN, byte &mPWM_LED_MAX, byte &mPWM_LED_MIN_DAY) {
  int plus_minus = 0;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  switch(buttonPressed){
    case BUTTON_UP:   plus_minus = 1;  break;
    case BUTTON_DOWN: plus_minus = -1; break;
    case BUTTON_OK:
      pos++;              // Переходим к следующему изменяемому значению
      if (pos>2) pos = 0; // и по кругу
    break;    
  }
  switch (pos) {
    case 0:
      mPWM_LED_MIN += plus_minus;                               // Задаем минимальное значение ШИМ
      if (mPWM_LED_MIN > mPWM_LED_MIN_DAY) mPWM_LED_MIN = 0;    // Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " PWM_MIN"; msg += ">>"; msg += "   "; // 20
    break;
    case 1:
      mPWM_LED_MAX += plus_minus;                               // Задаем максимальное значение ШИМ
      if (mPWM_LED_MAX < mPWM_LED_MIN_DAY) mPWM_LED_MAX = 255;  // Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " PWM_MAX"; msg += ">>"; msg += "   "; // 20
    break;
    case 2:
      mPWM_LED_MIN_DAY += plus_minus;         // Задаем минимальное значение ШИМ для дневного затенения
      if (mPWM_LED_MIN_DAY > mPWM_LED_MAX) mPWM_LED_MIN_DAY = mPWM_LED_MAX; // Ограничиваем значения
      if (mPWM_LED_MIN_DAY < mPWM_LED_MIN) mPWM_LED_MIN_DAY = mPWM_LED_MIN; // Ограничиваем значения
      msg = "<<Set"; msg += " PWM_MIN_DAY"; msg += ">>"; msg += " "; // 20
    break;   
  }
   
  mdisplay0 = mName;                    // Формируем строки с информацией для вывода на дисплей
  mdisplay1 = "PWM: MIN=" + ByteToStrFormat(map(mPWM_LED_MIN, 0, 255, 0, 100), 1) + " MAX=" + 
              ByteToStrFormat(map(mPWM_LED_MAX, 0, 255, 0, 100), 2) + "%";
  mdisplay2 = "  "; mdisplay2 += "MIN_DAY_STORM=" + ByteToStrFormat(map(mPWM_LED_MIN_DAY, 0, 255, 0, 100), 2) + "%";
  mdisplay3 = msg;
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
//=============================== Метод отвечающий за вывод меню настроек Люминесцентных ламп & CO2
void Menu_LAMP_CO2(String menuName, byte &mHoursON, byte &mMinutesON, byte &mHoursOFF, byte &mMinutesOFF) {
  int plus_minus = 0;     // Инкремент или декремент (+1 или -1)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  switch(buttonPressed){
    case BUTTON_UP:   plus_minus = 1;  break;
    case BUTTON_DOWN: plus_minus = -1; break;
    case BUTTON_OK:
      pos++;              // Переходим к следующему изменяемому значению
      if (pos>3) pos = 0; // и по кругу
    break;    
  }
  switch (pos) {
    case 0:
      mHoursON += plus_minus;             // Крутим значения часов
      if (mHoursON > 23) mHoursON = 0;      // Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " Hours"; msg += "ON"; msg += ">>"; msg += "   "; // 20
    break;
    case 1:
      mMinutesON += plus_minus;           // Крутим значения минут
      if (mMinutesON > 59) mMinutesON = 0;  // Ограничиваем значения
      msg = " "; msg += "<<Set"; msg += " Minutes"; msg += "ON"; msg += ">>"; msg += "  "; // 20
    break;
    case 2:
      mHoursOFF += plus_minus;             // Крутим значения часов
      if (mHoursOFF > 23) mHoursOFF = 0;      // Ограничиваем значения
      msg = "  "; msg += "<<Set"; msg += " Hours"; msg += "OFF"; msg += ">>"; msg += "  "; // 20
    break;
    case 3:
      mMinutesOFF += plus_minus;           // Крутим значения минут
      if (mMinutesOFF > 59) mMinutesOFF = 0;  // Ограничиваем значения
      msg = " "; msg += "<<Set"; msg += " Minutes"; msg += "OFF"; msg += ">>"; msg += " "; // 20
    break;   
  }
  mdisplay0 = menuName;                 // Формируем строки с информацией для вывода на дисплей
  mdisplay1 = "ON"; mdisplay1 += " Time"; mdisplay1 += " "; mdisplay1 += ":"; mdisplay1 += " "; // 10 
  mdisplay1 += ByteToStrFormat(mHoursON, 2) + ":" + ByteToStrFormat(mMinutesON, 2) + "   "; // 8-10
  mdisplay2 = "OFF"; mdisplay2 += " Time"; mdisplay2 += ":"; mdisplay2 += " "; // 10
  mdisplay2 += ByteToStrFormat(mHoursOFF, 2) + ":" + ByteToStrFormat(mMinutesOFF, 2) + "   "; // 8-10
  mdisplay3 = msg;
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
//=============================== Метод отвечающий за вывод меню настроек Системы обогрева
void Menu_HEATER(String menuName, byte &mTemp, float &mHysteresis) {
  int plus_minus = 0;     // Инкремент или декремент (+1 или -1)
  float fplus_minus = 0;  // Инкремент или декремент (+0.5 или -0.5)
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  switch(buttonPressed){
    case BUTTON_UP:   plus_minus = 1;  fplus_minus = 0.5; break;
    case BUTTON_DOWN: plus_minus = -1; fplus_minus = -0.5; break;
    case BUTTON_OK:
      pos++;              // Переходим к следующему изменяемому значению
      if (pos>1) pos = 0; // и по кругу
    break;    
  }
  switch (pos) {
    case 0:
      mTemp += plus_minus;             // Крутим значения часов
      if (mTemp > 28) mTemp = 20;      // Ограничиваем значения
      if (mTemp < 20) mTemp = 28;      // Ограничиваем значения
      msg = "<<Set"; msg += " "; msg += "Temperature"; msg += ">>"; msg += " "; // 20
    break;
    case 1:
      mHysteresis += fplus_minus;              // Крутим значения минут
      if (mHysteresis > 2) mHysteresis = 0.5;  // Ограничиваем значения
      if (mHysteresis < 0.5) mHysteresis = 2;  // Ограничиваем значения
      msg = " "; msg += "<<Set"; msg += " "; msg += "Hysteresis"; msg += ">>"; msg += " "; // 20
    break;   
  }
  mdisplay0 = menuName;                    // Формируем строки с информацией для вывода на дисплей
  mdisplay1 = "Temperature"; mdisplay1 += ":"; mdisplay1 += " "; mdisplay1 += ByteToStrFormat(mTemp, 2);
  mdisplay2 = "Hysteresis"; mdisplay2 += ":"; mdisplay2 += "  "; mdisplay2 += (String) mHysteresis;
  mdisplay3 = msg;
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
//=============================== Метод отвечающий за вывод меню Сохранения настроек
void Menu_Save(String menuName) {
  String msg;             // Сообщение редактируемого параметра
  String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  String mdisplay3;       // Четвертая строка дисплея
  switch(buttonPressed){
    case BUTTON_UP:   SaveSettings(); return; break;
    case BUTTON_DOWN: CancelSettings(); return; break;
    case BUTTON_OK: break;    
  }
  mdisplay0 = menuName;                    // Формируем строки с информацией для вывода на дисплей
  mdisplay1 = "Press \""; mdisplay1 += "+"; mdisplay1 += "\" to"; mdisplay1 += " Save";
  mdisplay2 = "Press \""; mdisplay2 += "-"; mdisplay2 += "\" to"; mdisplay2 += " Cancel";
  mdisplay3 = msg;
  PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
}
void LoadSettings() {
  // ЗАГРУЖАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ ЭНЕРГОНЕЗАВИСИМОЙ ПАМЯТИ
  //=============================== Переменные первого таймера для LED1 (розовый свет)
  hours_rassvet1_LED1 = EEPROM.read(0x00);         // час начала первого рассвета (утреннего)
  minutes_rassvet1_LED1 = EEPROM.read(0x01);       // минута начала первого рассвета (утреннего)
  duration_rassvet1_LED1 = EEPROM.read(0x02);      // длительность первого рассвета (утреннего) (в минутах)
  hours_zakat1_LED1 = EEPROM.read(0x03);           // час начала первого заката (дневного)
  minutes_zakat1_LED1 = EEPROM.read(0x04);         // минута начала первого заката (дневного)
  duration_zakat1_LED1 = EEPROM.read(0x05);        // длительность первого заката (дневного) (в минутах)
  hours_rassvet2_LED1 = EEPROM.read(0x06);         // час начала второго рассвета (дневного)
  minutes_rassvet2_LED1 = EEPROM.read(0x07);       // минута начала второго рассвета (дневного)
  duration_rassvet2_LED1 = EEPROM.read(0x08);      // длительность второго рассвета (дневного) (в минутах)
  hours_zakat2_LED1 = EEPROM.read(0x09);           // час начала второго заката (вечернего)
  minutes_zakat2_LED1 = EEPROM.read(0x0A);         // минута начала второго заката (вечернего)
  duration_zakat2_LED1 = EEPROM.read(0x0B);        // длительность второго заката (вечернего) (в минутах)
  PWM_LED1_MIN = EEPROM.read(0x0C);            // Если необходим ток покоя на LED - изменить это значение
  PWM_LED1_MAX = EEPROM.read(0x0D);            // Если необходимо ограничить максимальную яркость - уменьшить значение 
  PWM_LED1_MIN_DAY = EEPROM.read(0x0E);        // Минимальное значение ШИМ для дневного затенения
  PWM_LED1_MIN_DAY = PWM_LED1_MIN_DAY < 255 ? PWM_LED1_MIN_DAY : 15;
  PWM_LED1_MIN = PWM_LED1_MIN < PWM_LED1_MIN_DAY ? PWM_LED1_MIN : 0;
  PWM_LED1_MAX = PWM_LED1_MAX > PWM_LED1_MIN_DAY ? PWM_LED1_MAX : 255;
   
  //=============================== Переменные второго таймера для LED2 (белый свет)
  hours_rassvet1_LED2 = EEPROM.read(0x10);         // час начала первого рассвета (утреннего)
  minutes_rassvet1_LED2 = EEPROM.read(0x11);       // минута начала первого рассвета (утреннего)
  duration_rassvet1_LED2 = EEPROM.read(0x12);      // длительность первого рассвета (утреннего) (в минутах)
  hours_zakat1_LED2 = EEPROM.read(0x13);           // час начала первого заката (дневного)
  minutes_zakat1_LED2 = EEPROM.read(0x14);         // минута начала первого заката (дневного)
  duration_zakat1_LED2 = EEPROM.read(0x15);        // длительность первого заката (дневного) (в минутах)
  hours_rassvet2_LED2 = EEPROM.read(0x16);         // час начала второго рассвета (дневного)
  minutes_rassvet2_LED2 = EEPROM.read(0x17);       // минута начала второго рассвета (дневного)
  duration_rassvet2_LED2 = EEPROM.read(0x18);      // длительность второго рассвета (дневного) (в минутах)
  hours_zakat2_LED2 = EEPROM.read(0x19);           // час начала второго заката (вечернего)
  minutes_zakat2_LED2 = EEPROM.read(0x1A);         // минута начала второго заката (вечернего)
  duration_zakat2_LED2 = EEPROM.read(0x1B);        // длительность второго заката (вечернего) (в минутах)
  PWM_LED2_MIN = EEPROM.read(0x1C);            // Если необходим ток покоя на LED - изменить это значение
  PWM_LED2_MAX = EEPROM.read(0x1D);            // Если необходимо ограничить максимальную яркость - уменьшить значение 
  PWM_LED2_MIN_DAY = EEPROM.read(0x1E);        // Минимальное значение ШИМ для дневного затенения
  PWM_LED2_MIN_DAY = PWM_LED2_MIN_DAY < 255 ? PWM_LED2_MIN_DAY : 15;
  PWM_LED2_MIN = PWM_LED2_MIN < PWM_LED2_MIN_DAY ? PWM_LED2_MIN : 0;
  PWM_LED2_MAX = PWM_LED2_MAX > PWM_LED2_MIN_DAY ? PWM_LED2_MAX : 255;
   
  //=============================== Переменные третьего таймера для LAMP (Люминесцентные лампы)
  hours_ON1_LAMP = EEPROM.read(0x20);             // час начала первого периода
  minutes_ON1_LAMP = EEPROM.read(0x21);           // минута начала первого периода
  hours_OFF1_LAMP = EEPROM.read(0x22);            // час конца первого периода
  minutes_OFF1_LAMP = EEPROM.read(0x23);          // минута конца первого периода
  hours_ON2_LAMP = EEPROM.read(0x24);             // час начала второго периода
  minutes_ON2_LAMP = EEPROM.read(0x25);           // минута начала второго периода
  hours_OFF2_LAMP = EEPROM.read(0x26);            // час конца второго периода
  minutes_OFF2_LAMP = EEPROM.read(0x27);          // минута конца второго периода
  //=============================== Переменные четвертого таймера для CO2 (или чего-то еще, например аэрация)
  hours_ON1_CO2 = EEPROM.read(0x30);             // час начала первого периода
  minutes_ON1_CO2 = EEPROM.read(0x31);           // минута начала первого периода
  hours_OFF1_CO2 = EEPROM.read(0x32);            // час конца первого периода
  minutes_OFF1_CO2 = EEPROM.read(0x33);          // минута конца первого периода
  hours_ON2_CO2 = EEPROM.read(0x34);             // час начала второго периода
  minutes_ON2_CO2 = EEPROM.read(0x35);           // минута начала второго периода
  hours_OFF2_CO2 = EEPROM.read(0x36);            // час конца второго периода
  minutes_OFF2_CO2 = EEPROM.read(0x37);          // минута конца второго периода
   
 
  //=============================== Переменные системы обогрева аквариума
  temp_AVR = EEPROM.read(0x50);                      // заданная средняя температура воды аквариума
  hysteresis = (float)EEPROM.read(0x51) / 10;        // гистерезис температур в большую или меньшую сторону
  hysteresis = hysteresis < 5 ? hysteresis : 1.0f;
  //=============================== Переменные системы кормления
  hours_feed1 = EEPROM.read(0x60);         // час начала первого кормления (утреннего)
  minutes_feed1 = EEPROM.read(0x61);       // минута начала первого кормления (утреннего)
  duration_feed1 = EEPROM.read(0x62);      // длительность первого кормления (утреннего) (в минутах)
  hours_feed2 = EEPROM.read(0x63);           // час начала второго кормления (вечернего)
  minutes_feed2 = EEPROM.read(0x64);         // минута начала второго кормления (вечернего)
  duration_feed2 = EEPROM.read(0x65);        // длительность второго кормления (вечернего) (в минутах)
  //=============================== Переменные будильников
  hours_alarm1 = EEPROM.read(0x70);         // час начала первого будильника
  minutes_alarm1 = EEPROM.read(0x71);       // минута начала первого будильника
  duration_alarm1 = EEPROM.read(0x72);      // длительность первого будильника (в секундах)
  hours_alarm2 = EEPROM.read(0x73);           // час начала второго будильника
  minutes_alarm2 = EEPROM.read(0x74);         // минута начала второго будильника
  duration_alarm2 = EEPROM.read(0x75);        // длительность второго будильника (в секундах)
}
void SaveSettings() {
  //String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  //String mdisplay3;       // Четвертая строка дисплея
  //mdisplay0 = mdisplay3 = "";
  lcd.clear();
  mdisplay1 = "   "; mdisplay1 += "-"; mdisplay1 += "="; mdisplay1 += " Save"; mdisplay1 += " NOW "; 
  mdisplay1 += "="; mdisplay1 += "-"; mdisplay1 += "   ";
  lcd.setCursor(0,1);
  lcd.print(mdisplay1);
  for (int i=0; i<21; i++) {            // Формируем Прогресс Бар и выводим его
    mdisplay2 = "";
    for (int j=0; j<i; j++)
      mdisplay2.concat("\2");
    lcd.setCursor(0,2);
    lcd.print(mdisplay2);
    //PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
    delay(250);
  }
  // СОХРАНЯЕМ ВСЕ ПЕРЕМЕННЫЕ В ЭНЕРГОНЕЗАВИСИМУЮ ПАМЯТЬ!
  //=============================== Переменные первого таймера для LED1 (розовый свет)
  EEPROM.write(0x00, hours_rassvet1_LED1);         // час начала первого рассвета (утреннего)
  EEPROM.write(0x01, minutes_rassvet1_LED1);       // минута начала первого рассвета (утреннего)
  EEPROM.write(0x02, duration_rassvet1_LED1);      // длительность первого рассвета (утреннего) (в минутах)
  EEPROM.write(0x03, hours_zakat1_LED1);           // час начала первого заката (дневного)
  EEPROM.write(0x04, minutes_zakat1_LED1);         // минута начала первого заката (дневного)
  EEPROM.write(0x05, duration_zakat1_LED1);        // длительность первого заката (дневного) (в минутах)
  EEPROM.write(0x06, hours_rassvet2_LED1);         // час начала второго рассвета (дневного)
  EEPROM.write(0x07, minutes_rassvet2_LED1);       // минута начала второго рассвета (дневного)
  EEPROM.write(0x08, duration_rassvet2_LED1);      // длительность второго рассвета (дневного) (в минутах)
  EEPROM.write(0x09, hours_zakat2_LED1);           // час начала второго заката (вечернего)
  EEPROM.write(0x0A, minutes_zakat2_LED1);         // минута начала второго заката (вечернего)
  EEPROM.write(0x0B, duration_zakat2_LED1);        // длительность второго заката (вечернего) (в минутах)
  EEPROM.write(0x0C, PWM_LED1_MIN);            // Если необходим ток покоя на LED - изменить это значение
  EEPROM.write(0x0D, PWM_LED1_MAX);            // Если необходимо ограничить максимальную яркость - уменьшить значение 
  EEPROM.write(0x0E, PWM_LED1_MIN_DAY);        // Минимальное значение ШИМ для дневного затенения
   
  //=============================== Переменные второго таймера для LED2 (белый свет)
  EEPROM.write(0x10, hours_rassvet1_LED2);         // час начала первого рассвета (утреннего)
  EEPROM.write(0x11, minutes_rassvet1_LED2);       // минута начала первого рассвета (утреннего)
  EEPROM.write(0x12, duration_rassvet1_LED2);      // длительность первого рассвета (утреннего) (в минутах)
  EEPROM.write(0x13, hours_zakat1_LED2);           // час начала первого заката (дневного)
  EEPROM.write(0x14, minutes_zakat1_LED2);         // минута начала первого заката (дневного)
  EEPROM.write(0x15, duration_zakat1_LED2);        // длительность первого заката (дневного) (в минутах)
  EEPROM.write(0x16, hours_rassvet2_LED2);         // час начала второго рассвета (дневного)
  EEPROM.write(0x17, minutes_rassvet2_LED2);       // минута начала второго рассвета (дневного)
  EEPROM.write(0x18, duration_rassvet2_LED2);      // длительность второго рассвета (дневного) (в минутах)
  EEPROM.write(0x19, hours_zakat2_LED2);           // час начала второго заката (вечернего)
  EEPROM.write(0x1A, minutes_zakat2_LED2);         // минута начала второго заката (вечернего)
  EEPROM.write(0x1B, duration_zakat2_LED2);        // длительность второго заката (вечернего) (в минутах)
  EEPROM.write(0x1C, PWM_LED2_MIN);            // Если необходим ток покоя на LED - изменить это значение
  EEPROM.write(0x1D, PWM_LED2_MAX);            // Если необходимо ограничить максимальную яркость - уменьшить значение 
  EEPROM.write(0x1E, PWM_LED2_MIN_DAY);        // Минимальное значение ШИМ для дневного затенения
  //=============================== Переменные третьего таймера для LAMP (Люминесцентные лампы)
  EEPROM.write(0x20, hours_ON1_LAMP);             // час начала первого периода
  EEPROM.write(0x21, minutes_ON1_LAMP);           // минута начала первого периода
  EEPROM.write(0x22, hours_OFF1_LAMP);            // час конца первого периода
  EEPROM.write(0x23, minutes_OFF1_LAMP);          // минута конца первого периода
  EEPROM.write(0x24, hours_ON2_LAMP);             // час начала второго периода
  EEPROM.write(0x25, minutes_ON2_LAMP);           // минута начала второго периода
  EEPROM.write(0x26, hours_OFF2_LAMP);            // час конца второго периода
  EEPROM.write(0x27, minutes_OFF2_LAMP);          // минута конца второго периода
  //=============================== Переменные четвертого таймера для CO2 (или чего-то еще, например аэрация)
  EEPROM.write(0x30, hours_ON1_CO2);             // час начала первого периода
  EEPROM.write(0x31, minutes_ON1_CO2);           // минута начала первого периода
  EEPROM.write(0x32, hours_OFF1_CO2);            // час конца первого периода
  EEPROM.write(0x33, minutes_OFF1_CO2);          // минута конца первого периода
  EEPROM.write(0x34, hours_ON2_CO2);             // час начала второго периода
  EEPROM.write(0x35, minutes_ON2_CO2);           // минута начала второго периода
  EEPROM.write(0x36, hours_OFF2_CO2);            // час конца второго периода
  EEPROM.write(0x37, minutes_OFF2_CO2);          // минута конца второго периода
   
 
  //=============================== Переменные системы обогрева аквариума
  EEPROM.write(0x50, temp_AVR);                      // заданная средняя температура воды аквариума
  EEPROM.write(0x51, (byte)hysteresis*10);           // гистерезис температур в большую или меньшую сторону
  //=============================== Переменные системы кормления
  EEPROM.write(0x60, hours_feed1);         // час начала первого кормления (утреннего)
  EEPROM.write(0x61, minutes_feed1);       // минута начала первого кормления (утреннего)
  EEPROM.write(0x62, duration_feed1);      // длительность первого кормления (утреннего) (в минутах)
  EEPROM.write(0x63, hours_feed2);           // час начала второго кормления (вечернего)
  EEPROM.write(0x64, minutes_feed2);         // минута начала второго кормления (вечернего)
  EEPROM.write(0x65, duration_feed2);        // длительность второго кормления (вечернего) (в минутах)
 
   
  pMenu = 0;              // После сохраниния попадаем в информационное меню
  ClearDisplay();
}
void CancelSettings() {
  //String mdisplay0;       // Первая строка дисплея
  String mdisplay1;       // Вторая строка дисплея
  String mdisplay2;       // Третья строка дисплея
  //String mdisplay3;       // Четвертая строка дисплея
  //mdisplay0 = mdisplay3 = "";
  lcd.clear();
  //mdisplay1 = "  -= Cancelling =-  ";
  mdisplay1 = "  "; mdisplay1 += "-"; mdisplay1 += "="; mdisplay1 += " Cancel"; mdisplay1 += " NOW "; 
  mdisplay1 += "="; mdisplay1 += "-"; mdisplay1 += "  ";
  lcd.setCursor(0,1);
  lcd.print(mdisplay1);
  for (int i=19; i>-1; i--) {            // Формируем Прогресс Бар и выводим его
    mdisplay2 = "";
    for (int j=0; j<i; j++) {
      mdisplay2.concat("\2");
    }
    lcd.setCursor(0,2);
    lcd.print(mdisplay2);
    //PrintDisplay(mdisplay0, mdisplay1, mdisplay2, mdisplay3); // Выводим строки на дисплей
    lcd.setCursor(i, 2);
    lcd.print(" ");
    delay(250);
  }
  pMenu = 0;              // После отмены изменений попадаем в информационное меню
  LoadSettings();         // ЗАГРУЖАЕМ ЗНАЧЕНИЯ ПЕРЕМЕННЫХ ИЗ ЭНЕРГОНЕЗАВИСИМОЙ ПАМЯТИ
  ClearDisplay();
}
//=================================== ГЛАВНЫЙ ЦИКЛ ПРОГРАММЫ ===============================
void loop() {
  timeRtc();
  timerLED1();
  timerLED2();
  timerLAMP();
  timerCO2();
 
  Menu();
  timerFeeding();
  timerAlarm(); 
  detectTemperature();
  HEATER();
}

