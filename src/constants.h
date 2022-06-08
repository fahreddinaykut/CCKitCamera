/*
* Constants_structs_and_functions.h
*      
* --> This header file contains the structs and functions used in both ESP32 modules;
*            
*/


/********************************
** Simbolic constants          **
********************************/
// --> Pins from the OV2640 camera module (CAMERA_MODEL_AI_THINKER)
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1 // RESET pin is not available
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26 //SDA pin - 'GPIO 26'
#define SIOC_GPIO_NUM     27 //SCL pin - 'GPIO 26'
#define Y9_GPIO_NUM       35 //D7 pin - 'GPIO 35'
#define Y8_GPIO_NUM       34 //D6 pin - 'GPIO 34'
#define Y7_GPIO_NUM       39 //D5 pin - 'GPIO 39'
#define Y6_GPIO_NUM       36 //D4 pin - 'GPIO 36'
#define Y5_GPIO_NUM       21 //D3 pin - 'GPIO 21'
#define Y4_GPIO_NUM       19 //D2 pin - 'GPIO 19'
#define Y3_GPIO_NUM       18 //D1 pin - 'GPIO 18'
#define Y2_GPIO_NUM        5 //D0 pin - 'GPIO 5'
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22


// --> EEPROM constants: picture number saved in card
#define EEPROM_SIZE 1
#define EEPROM_ADDRESS 0



