#include<Wire.h>
#include<LiquidCrystal_I2C.h>
#include<arduinoFFT.h>


#define SAMPLES 64             //採樣樣本數, 必須是 2 的冪次方倍
#define SAMPLING_FREQUENCY 1700 //(能夠顯示的最高頻率 1700/2, 因為複數對稱的緣故)Ts = Based on Nyquist, must be 2 times the highest expected frequency.
#define  xres 16      // lcd設定, 單列數
#define  yres 8       // 供lcd單格八列映射用

LiquidCrystal_I2C lcd(0x3F,16,2); //設定lcd
arduinoFFT FFT = arduinoFFT();   //創建 FFT 物件

//自定義線條圖
byte v1[] = {
  B00000, B00000, B00000, B00000, B00000, B00000, B00000, B11111
};
byte v2[] = {
  B00000, B00000, B00000, B00000, B00000, B00000, B11111, B11111
};
byte v3[] = {
  B00000, B00000, B00000, B00000, B00000, B11111, B11111, B11111
};
byte v4[] = {
  B00000, B00000, B00000, B00000, B11111, B11111, B11111, B11111
};
byte v5[] = {
  B00000, B00000, B00000, B11111, B11111, B11111, B11111, B11111
};
byte v6[] = {
  B00000, B00000, B11111, B11111, B11111, B11111, B11111, B11111
};
byte v7[] = {
  B00000, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};
byte v8[] = {
  B11111, B11111, B11111, B11111, B11111, B11111, B11111, B11111
};
byte v9[] = { 
  B00000, B00000, B00000, B00000, B00000, B00000, B00000, B00000
};
int MY_ARRAY[]={0, 1, 2, 3, 4, 5, 6, 7, 8}; //供自定義線條對應
unsigned int samplingPeriod;
unsigned long microSeconds;


double vReal[SAMPLES]; //實數
double vImag[SAMPLES]; //虛數
char data_avgs[xres];  //區間樣本平均集 - 供繪圖使用

int yvalue;
int displaycolumn , displayvalue;
int peaks[xres];   //peak = 峰值
int steps = (SAMPLES /2) / xres;   //每區間幾個樣本單位
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers

void setup() 
{
    Serial.begin(115200); //設定鮑率
    samplingPeriod = round(900000*(1.0/SAMPLING_FREQUENCY)); //Period 

    
    lcd.init();           //LCD初始化
    lcd.backlight();      //LCD背光燈開啟
    lcd.createChar(1, v1);
    lcd.createChar(2, v2);
    lcd.createChar(3, v3);
    lcd.createChar(4, v4);
    lcd.createChar(5, v5);
    lcd.createChar(6, v6);
    lcd.createChar(7, v7);
    lcd.createChar(8, v8);
    lcd.createChar(9, v9);


    //畫圖 沒意義的圖  以下：
  String loading = "LOADING..    [0%]";
  int percentage = 0;
   
  for (int i = 0; i < xres; i++){
    lcd.setCursor(0, 0);

    percentage = (int) ((i / (float)xres) * 100);
    
    if (i < (xres / 3) && percentage % 2 == 0)
    {
      loading = "LOADING.  [" + String(percentage) + "%]";
      lcd.print(loading);
    }
    else if (i < ((xres / 3) * 2) && percentage % 2 == 0)
    {
      loading = "LOADING..[" + String(percentage) + "%]";
      lcd.print(loading);
    }
    else if (percentage % 2 == 0)
    {
      loading = "LOADING...[" + String(percentage) + "%]";
      lcd.print(loading);
    }

    for (int load = 0; load <= i; load++)
    {
      lcd.setCursor(load, 1);
      lcd.write(8);  
    }   
    
    delay(50);
  }

  lcd.setCursor(0, 0);
  loading = "LOADING...[100%]";
  lcd.print(loading);
  
  delay(200);                         // wait to get reference voltage stabilized and show the progress a bit longer time :)
  lcd.clear();
}
 
void loop() 
{  
   microSeconds = micros();

    //讀取/存取64筆資料
    for(int i=0; i<SAMPLES; i++)
    {
        
     
        vReal[i] = analogRead(A0); //傅立葉實數等於測量值
        vImag[i] = 0; //虛數為零
        Serial.println(vReal[i]);
        while(micros() - microseconds < sampling_period_us){
        //empty loop
      }
      microseconds += sampling_period_us;
    }

    /* FFT on samples*/
    FFT.Windowing(vReal, SAMPLES, FFT_WIN_TYP_HAMMING, FFT_FORWARD);  //窗函數
    FFT.Compute(vReal, vImag, SAMPLES, FFT_FORWARD);   //正傅立葉轉換
    //FFT.ComplexToMagnitude(vReal, vImag, SAMPLES);
    
    /*Find peak frequency and print peak*/
    double peak = FFT.MajorPeak(vReal, SAMPLES, 2000);   //基音頻


    Display(0, ReArrange(steps, data_avgs, vReal)); //顯示LCD

    lcd.setCursor(0,1);          //第二列(左)顯示時域值
    lcd.print(analogRead(A0));

    lcd.setCursor(9,1);
    lcd.print(peak);             //第二列(右)顯示基音頻
    lcd.setCursor(14,1);
    lcd.print("Hz");
    delay(100);


    for(int i=0;i<SAMPLES;i++){
      vReal[i]+=10;
    }

}
char * ReArrange(int steps, char * dataAvgs, double * realValues)   
{
  int c = 0;
    
  for(int i = 0; i < (SAMPLES /2); i += steps)      //i=0,2,4,6...64
  {
    dataAvgs[c] = 0;
      
    for (int k = 0 ; k < steps ; k++)          //每兩組唯一區間取平均
    {
      dataAvgs[c] = dataAvgs[c] + realValues[i + k];
    }
      
    dataAvgs[c] = dataAvgs[c] / steps; 
    c++;
  }

  return dataAvgs;
}

void Display(int line, char * data_avgs)
{
  displaycolumn = 0;
//  displayvalue = 0;
  yvalue = 0;
    
  // ++ send to display according measured value 
  for(int i = 0; i < xres; i++)
  { 
    
    data_avgs[i] = constrain(data_avgs[i], 0, 80);            // 限制範圍

    data_avgs[i] = map(data_avgs[i], 0,80, 0, yres);        // 映射至0~8, 供LCD用

    yvalue = data_avgs[i];


    /*data_peaks[i] = data_peaks[i] - 1;    // decay by one light
      
    if (yvalue > data_peaks[i]) {
      data_peaks[i] = yvalue ;
      yvalue = data_peaks[i]; 

    }   */
   /*if (yvalue >=3)
   {
    Serial.println(yvalue);
   }*/
    lcd.setCursor(displaycolumn, line);  //以下開始一格一格繪製(FOR迴圈)
    if (MY_ARRAY[yvalue] == 0)
    {
      lcd.write(9);
    }
    else
    {
      lcd.write(MY_ARRAY[yvalue]+1);
    }
    displaycolumn++;   //每次繪製一格
  }
  delay(100);
} 
