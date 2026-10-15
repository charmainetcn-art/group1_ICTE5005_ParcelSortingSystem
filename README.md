# group1_ICTE5005_ParcelSortingSystem

# Members 
23036897 
23594900 
21455209 
23446470

# Steps to replicate
1. connect to 1 arduino and upload Lidar code and label this as Arduino1
2. connect to 1 arduino and upload Servo code and label this as Arduino2
3. connect to 1 arduino and upload Bin code and label this as Arduino3
4. connect to 1 arduino and upload Master code and label this as Arduino4

# required components
arduino 1 
- lidar

arduino 2 
- servo

arduino 3
- 2 lidar

arduino 4 
- 2 buttons
- 1 green light
- 1 red light
- 2 resistor

# intended wiring 
arduino 1 
TRIG = Pin 9, 
ECHO = Pin 10, 
A4 = board SDA column,
A5  = board SCL column,
GND = board Shared 

arduino 2
servo Signal Wire (Orange/Yellow) = Pin 9,
servo Red Wire = External 5V (+),
servo Brown/Black Wire = External 5V (–) = board GND,
Pin A4 = board SDA column, 
Pin A5 = board SCL column, 
GND = board GND

arduino 3
TRIG = Pin 2,
ECHO = Pin 3,
TRIG = Pin 4, 
ECHO = Pin 5,
Pin A4 = board SDA column, 
Pin A5 = board SCL column, 
GND = board GND

arduino 4
START Button: Pin 2  GND
STOP Button: Pin 3  GND
Green LED (Running): Pin 6  220Ω  LED (+) | LED (–)  GND
Red LED (Warning/Full): Pin 7  220Ω  LED (+) | LED (–)  GND
Yellow LED (Activity): Pin 8  220Ω  LED (+) | LED (–)  GND
Pin A4 = board SDA column, 
Pin A5 = board SCL column, 
GND = board GND

