/*
    Máquina Expendedora en Arduino
    Sistemas Empotrados y de Tiempo Real
    autor: Ioana Carmen Pasca
 */

#include <LiquidCrystal.h>
#include <DHT.h>
#include <avr/wdt.h>

//----------------------------------------------------------------- Constants
//---------------------------------------------------------- Pins
#define VRX_PIN A0     // Arduino pin connected to VRX pin
#define VRY_PIN A1     // Arduino pin connected to VRY pin
#define SW_PIN 3       // Arduino pin connected to SW  pin
#define BOTTON_PIN 2   // Arduino pin connected to botton
#define LED01_PIN 23   // LED AZUL
#define LED02_PIN 9    // LED VERDE (PWM)
#define TEMP_H_PIN 24  // SENSOR DE HUMEDAD Y TEMPERATURA
#define HCTRGG 10      // SENSOR DE ULTRA SONIDOS TRIGGER
#define HCECHO 13      // SENSOR DE ULTRA SONIDOS ECHO
#define LCD_RS 12      // LCD PINS
#define LCD_ENABLE 11  // LCD PINS
#define LCD_D4 6       // LCD PINS
#define LCD_D5 5       // LCD PINS
#define LCD_D6 4       // LCD PINS
#define LCD_D7 7       // LCD PINS
//---------------------------------------------------------- States
#define ADMIN 1
#define START 2
#define WAITING 3
#define SERVING 4
#define CHANGE 5
//---------------------------------------------------------- Time (ms)
#define COFFEE_FINISHED_TIME 3000 // message coffee
#define TIMEDHT 5000  // seconds in screen
#define MIN_TIME_DRINKS 4000 // from 4
#define MAX_TIME_DRINKS 9000 // to 10 seconds for drinks
#define TIME_STARTUP 6000
#define MIN_TIME_RESET_BOTTON 2000
#define MAX_TIME_RESET_BOTTON 3000
#define TIME_ADMIN_BOTTON 5000
//---------------------------------------------------------- LCD
LiquidCrystal lcd_1(LCD_RS, LCD_ENABLE, LCD_D4, LCD_D5, LCD_D6, LCD_D7);
//---------------------------------------------------------- LED
#define MIN_PWD_LIGHT 50
#define MAX_LIGHT_PWD 255
#define INCREASED_PWD_LIGHT 3
//-------------------------------- Humidity and Temperature sensor
#define DHTTYPE DHT11
DHT dht(TEMP_H_PIN, DHTTYPE);
//---------------------------------------------------------- Joystick
// Range
#define LEFT_THRESHOLD 400
#define RIGHT_THRESHOLD 800
#define UP_THRESHOLD 400
#define DOWN_THRESHOLD 800
// Commands
#define COMMAND_NO 0x00
#define COMMAND_LEFT 0x01
#define COMMAND_RIGHT 0x02
#define COMMAND_UP 0x04
#define COMMAND_DOWN 0x08
//---------------------------------------------------------- Ultrasound
#define DISTANCE_CLIENT 100  // cm = 1m
//---------------------------------------------------------- Drinks
#define MIN_DRINKS 0  // min number of drinks
#define MAX_DRINKS 4  // max number of dinks
//---------------------------------------------------------- Admin
#define MIN_ADMIN 0  // min number of options in admin
#define MAX_ADMIN 3  // max number of options in admin
#define DHT_I 0      // option temperature and humidity of menu admin
#define HC_II 1      // option distance of menu admin
#define MILIS_III 2  // option timer of menu admin
#define CHANGE_IV 3  // option change prices of menu admin
#define INCREASE_PRICE 0.05 // how much the prices changes in admin menu
String drinks[] = {
  "Cafe Solo",
  "Cafe Cortado",
  "Cafe Doble",
  "Cafe Premium",
  "Chocolate"};
float prices[] = {
  1.00,
  1.10,
  1.25,
  1.50,
  2.00
};
String admin[] = {
  "Ver temperatura",
  "Ver distancia \nsensor",
  "Ver contador",
  "Modificar precios"
};

//----------------------------------------------------------------- Variables
//--------------------------------------------- State
int state = 0;
//--------------------------------------------- Time
unsigned long start_time_starup = 0;
unsigned long start_time_ht = 0;
unsigned long start_time_preparing = 0;
unsigned long start_time_f_coffe = 0;
int random_time = 0;
//--------------------------------------------- Menu
//-- Admin
volatile bool admin_menu_flag = false;
int pos_admin_menu = 0;
int selection = 0;
bool button_change_pressed = false;
bool button_admin_pressed = false;
//-- Change
bool flag_change_menu = false;
//-- Start up
bool flag_start_menu = false;
bool first_start_menu = true;
//-- Main manu
bool flag_person = false;
bool first_ht = true;
bool flag_menu_dht = false;
bool first_joy_button = true;
bool finished_coffe = true;
bool drink_selected = false;
//--------------------------------------------- Admin
unsigned long start_time_button_admin = 0;
//--------------------------------------------- Joystick
int x_value = 0;  // To store value of the X axis
int y_value = 0;  // To store value of the Y axis
int cm = 0;
int command = COMMAND_NO;
//--------------------------------------------- Drinks
int pos_drink = 0;
//--------------------------------------------- Button
int joy_button_state = 0;
//--------------------------------------------- Ranodm
long randNumber;
//--------------------------------------------- Leds
int pwd_light = MIN_PWD_LIGHT;


//----------------------------------------------------------------- Functions
long read_ultrasonic_distance(int trigger_pin, int echo_pin) {
    // Clear trigger pin
    pinMode(trigger_pin, OUTPUT);
    digitalWrite(trigger_pin, LOW);
    delayMicroseconds(2);

    // Sets the trigger HIGH for 10 microseconds
    digitalWrite(trigger_pin, HIGH);
    delayMicroseconds(10);
    digitalWrite(trigger_pin, LOW);
    pinMode(echo_pin, INPUT);

    // Reads the echo pin
    return pulseIn(echo_pin, HIGH);
}

int read_joystick() {
    // Read analog values
    x_value = analogRead(VRX_PIN);
    y_value = analogRead(VRY_PIN);

    // Converts the analog value to commands
    command = COMMAND_NO;

    // Check left/right commands
    if (x_value < LEFT_THRESHOLD)
        command = command | COMMAND_LEFT;
    else if (x_value > RIGHT_THRESHOLD)
        command = command | COMMAND_RIGHT;

    // Check up/down commands
    if (y_value < UP_THRESHOLD)
        command = command | COMMAND_UP;
    else if (y_value > DOWN_THRESHOLD)
        command = command | COMMAND_DOWN;

    return command; // return the pertinent command
}

int up_down_selection(int position, int MAX, int MIN) { // Selecting options with joystick
    // Constant
    int INCREMENT_JOYSTICK = 1;
    // Read the joystick
    int command = read_joystick();

    // Iterates through the list upwards
    if (command & COMMAND_UP) {
        position += INCREMENT_JOYSTICK;
        
        // If it has reched the limit resets the list else continues
        position = (position > MAX) ? MIN : position;
        delay(100); // bouncing
    }

    // Iterates through the list downwards
    if (command & COMMAND_DOWN) {
        position -= INCREMENT_JOYSTICK;
        
        // If it has reched the limit resets the list else continues
        position = (position < MIN) ? MAX : position;
        delay(100); // bouncing
    }

    return position;
}

//----------------------------------------------------- Interruptions
void ISR_admin() { // Enters the admin menu
    start_time_button_admin = millis();
    admin_menu_flag = true;
}

//----------------------------------------------------- Interface
void temp_h_menu() { // Menu for Teperature & Humidity
    // Constant
    int MAX_H_DIGITS = 2;
    // Read sensor DHT
    int chk = dht.read(TEMP_H_PIN);
    float t = dht.readTemperature();
    float h = dht.readHumidity();

    lcd_1.clear();
    lcd_1.print("Temp: ");
    lcd_1.print(t, 2);
    lcd_1.write(0xDF); // "º"
    lcd_1.print("C");

    lcd_1.setCursor(0, 1);
    lcd_1.print("Hum: ");
    lcd_1.print(h, MAX_H_DIGITS);
    lcd_1.print("%");
}

void drinks_menu(int p_drink) { // Menu of Drinks
    lcd_1.clear();
    lcd_1.print(drinks[p_drink]);
    lcd_1.setCursor(0, 1);
    lcd_1.print(prices[p_drink]);
}

void admin_menu(int p_admin) {
    lcd_1.clear();
    
    // Split string into lines using "\n" as delimiter
    String currentLine = admin[p_admin];
    int newlineIndex = currentLine.indexOf('\n');
    
    // Print the first line
    lcd_1.print(currentLine.substring(0, newlineIndex));
    
    // If there is a second line it also prints it
    if (newlineIndex != -1) {
        lcd_1.setCursor(0, 1);
        lcd_1.print(currentLine.substring(newlineIndex + 1));
    }
}


void select_drink() { // Select an option in the drink menu
    unsigned long elapsed_time_preparing = millis() - start_time_preparing;

    if (elapsed_time_preparing < random_time) {
        lcd_1.clear();
        lcd_1.print("Preparando");
        lcd_1.setCursor(0, 1);
        lcd_1.print("Cafe...");
        // Increases the brightness of the led
        analogWrite(LED02_PIN, pwd_light);
        pwd_light += INCREASED_PWD_LIGHT;
    } else {
        if (finished_coffe) {
            start_time_f_coffe = millis();
            finished_coffe = false;

            // Reset the amount of light
            pwd_light = MIN_PWD_LIGHT;
            analogWrite(LED02_PIN, MAX_LIGHT_PWD); // When finished uses full light
      }
      unsigned long elapsed_time_preparing = millis() - start_time_f_coffe;

      // Coffee ready text
      lcd_1.clear();
      lcd_1.print("RETIRE");
      lcd_1.setCursor(0, 1);
      lcd_1.print("BEBIDA");

      if (elapsed_time_preparing > COFFEE_FINISHED_TIME) {
          // Clean upon exit
          lcd_1.clear();
          analogWrite(LED02_PIN, 0);
          pos_drink = 0;
          flag_person = false;
          flag_menu_dht = false;
          first_joy_button = true;
          finished_coffe = true;
          first_ht = true;
          drink_selected = false;
      }
    }
}

void distance_menu() { // Measure distance
    cm = 0.01723 * read_ultrasonic_distance(HCTRGG, HCECHO); // HC SR04

    lcd_1.clear();
    lcd_1.print("Distancia: ");
    lcd_1.setCursor(0, 1);
    lcd_1.print(cm);
    lcd_1.print("cm");
}

void timer_menu() { // Shows the running time
    lcd_1.clear();
    lcd_1.print("Tiempo ejecucion: ");
    lcd_1.setCursor(0, 1);
    lcd_1.print(millis()/1000); // In seconds
    lcd_1.print("s");
}

void change_menu(int pos_price) { // Change the prices of the drinks
    // Read the joystick
    int command_change = read_joystick();

    // Prints the menu for drinks in the position desired
    drinks_menu(pos_price);

    // Iterates through the list upwards
    if (command_change & COMMAND_DOWN) {
        prices[pos_price] += INCREASE_PRICE;
        
        delay(100); // bouncing
    }

    // Iterates through the list downwards
    if (command_change & COMMAND_UP) {
        prices[pos_price] -= INCREASE_PRICE;
        
        // If it has reched 0 no negative price else continues
        prices[pos_price] = (prices[pos_price] < 0) ? 0 : prices[pos_price];
        delay(100); // bouncing
    }

    // Returns to drink menu
    if (command_change & COMMAND_LEFT) {
        Serial.print("--------- button_change_pressed: ");
        button_change_pressed = false;
    }
}

//---------------------------------------------------------- Set-Up
void setup() {
    Serial.begin(9600);

    // Asigning leds
    pinMode(LED01_PIN, OUTPUT);
    pinMode(LED02_PIN, OUTPUT);

    // Set up the number of columns and rows on the LCD
    lcd_1.begin(16, 2);

    // Initialize buttons
    pinMode(BOTTON_PIN, INPUT); // normal
    pinMode(SW_PIN, INPUT_PULLUP); // joystick

    // Watchdog set up
    wdt_enable(WDTO_2S);

    // Pin of interruption
    attachInterrupt(digitalPinToInterrupt(BOTTON_PIN), ISR_admin, RISING);

    // Seed the random number generator
    randomSeed(analogRead(0));
}

//---------------------------------------------------------- Main Program
void loop() {
    // Sensors
    cm = 0.01723 * read_ultrasonic_distance(HCTRGG, HCECHO); // HC SR04
    int read_botton = digitalRead(BOTTON_PIN); // button
    int joy_button_state = digitalRead(SW_PIN); // Joystick button
    Serial.print("--------- State: "); // Control prints
    Serial.print(state);
    Serial.print("\n");

    //---------------------------------------------------------- Select state
    if (admin_menu_flag) { // Prints the admin menu
        unsigned long elapsed_time_admmin = millis() - start_time_button_admin;
        Serial.print("--------- elapsed_time_admmin: "); // Control prints
        Serial.print(elapsed_time_admmin);
        Serial.print("\n");

        if (elapsed_time_admmin > TIME_ADMIN_BOTTON) {
            state = ADMIN;
        } else if ((elapsed_time_admmin > MIN_TIME_RESET_BOTTON && elapsed_time_admmin < MAX_TIME_RESET_BOTTON) && read_botton == LOW) {
            state = START;
            digitalWrite(LED01_PIN, LOW);
            analogWrite(LED02_PIN, 0);
            first_start_menu = true;
            admin_menu_flag = false;
            flag_menu_dht = false;
            first_ht = true;
        }
    } else if (flag_change_menu) { // When changing the price of a drink
        state == CHANGE;
    } else if (!flag_start_menu) { // State a)
        state = START;

        if (first_start_menu) { // Starts the timer
            start_time_starup = millis();
            first_start_menu = false;
        }
    } else if (flag_person || cm < DISTANCE_CLIENT) { // Serving
        state = SERVING;

        if (first_ht) {
            start_time_ht = millis();
            random_time = random(MIN_TIME_DRINKS, MAX_TIME_DRINKS); // Generate random in 4 or 9
            first_ht = false;
        }
    } else if (!flag_person) { // Wait for a person
        state = WAITING;
    }

    //---------------------------------------------------------- States
    if (state == ADMIN) { // Enters admin menu
        // Light the two leds
        digitalWrite(LED01_PIN, HIGH);
        analogWrite(LED02_PIN, MAX_LIGHT_PWD);

        if (button_admin_pressed || joy_button_state == LOW) {
            button_admin_pressed = true; // Button has been preesed
            int command_admin = read_joystick();

            // Returns to admin menu
            if (command_admin & COMMAND_LEFT) {
                button_admin_pressed = false;
            }

            // Option selection
            if (pos_admin_menu == DHT_I) temp_h_menu(); // menu DHT
            if (pos_admin_menu == HC_II) distance_menu(); // menu HC-SR04
            if (pos_admin_menu == MILIS_III) timer_menu(); // menu Time
            if (pos_admin_menu == CHANGE_IV) { // Change price of drink
                flag_change_menu = true;
                admin_menu_flag = false;
                state = CHANGE;
            }
        } else {
            // Prints admin menu
            admin_menu(pos_admin_menu);
            pos_admin_menu = up_down_selection(pos_admin_menu, MAX_ADMIN, MIN_ADMIN);
        }
    } else if (state == CHANGE) { // CHANGE
        int command_change_menu = read_joystick(); // reads the joystick

        if (command_change_menu & COMMAND_LEFT) { // Exists reseting all variables
            flag_change_menu = false;
            admin_menu_flag = true;
            state = ADMIN;
        }

        if (button_change_pressed || joy_button_state == LOW) {
            // Return in the drink selection
            button_change_pressed = true;
            change_menu(selection);
        } else {
            // If button pressed saves the option of drink
            drinks_menu(selection);
            selection = up_down_selection(selection, MAX_DRINKS, MIN_DRINKS);
        }
    } else if (state == SERVING) { // Select the drink (State b))
        flag_person = true;

        if (!flag_menu_dht) { // Shows temperature and humidty
            unsigned long elapsed_time_ht = millis() - start_time_ht;

            if (elapsed_time_ht <= TIMEDHT) {
                temp_h_menu();
            } else {
                flag_menu_dht = true;
            }
        } else {  // Normal menu
          if (drink_selected || joy_button_state == LOW) { // Drink selected
              if (first_joy_button) {
                  start_time_preparing = millis();
                  first_joy_button = false;
              }
              drink_selected = true;
              select_drink();
          } else { // Normal drink menu
              drinks_menu(pos_drink);
              pos_drink = up_down_selection(pos_drink, MAX_DRINKS, MIN_DRINKS);
          }
        }
    } else if (state == START) { // State a)
        unsigned long elapsed_time_start = millis() - start_time_starup;

        lcd_1.clear();
        lcd_1.print("CARGANDO...");

        // Lights up 1 s and then waits 1 s
        if (elapsed_time_start < TIME_STARTUP) {
            if ((elapsed_time_start / 1000) % 2 == 0) {
                digitalWrite(LED01_PIN, HIGH);
            } else {
                digitalWrite(LED01_PIN, LOW);
            }
        } else { // Done with the LED
            lcd_1.clear();
            lcd_1.print("Servicio");
            digitalWrite(LED01_PIN, LOW);
            delay(100);
            flag_start_menu = true;
        }
    } else if (state == WAITING) { // Waiting fot a person to order
        lcd_1.clear();
        lcd_1.print("ESPERANDO");
        lcd_1.setCursor(0, 1);
        lcd_1.print("CLIENTE");
    }
    
    wdt_reset(); // Resets watchdog
    delay(100);
}