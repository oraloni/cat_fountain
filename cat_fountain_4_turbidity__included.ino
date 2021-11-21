/*
Cat automated drinking fountain
--------------------------------
When the cat comes near the fountain, an IR proximity sensor will detect it, and the pump will start working, pumping water
for the cat to drink and/or play. Along with the pump, and RGB LED will turn on showing a color on the scale from green to red,
depends on the turbidity level of the water - letting you know when the water is getting unclean.
When the cat leaves the fountain, and is out of the sensor reach, the pump and RGB LED will stay on for another 2 minute.

After this 2 minutes delay, the pump and will stop working, and the fountain enters his auto water circulation intervals mode.
As long as nothing comes near the fountain (the sensor), it will wait for 15 minutes, after which it will start the 
pump for a duration of 3 minute.
The fountain will keep doing this in 15 minutes intervals. Those are meant to make sure the water inside the fountain do not stay
still for a too long, and giving it the chance to pass through the filter, at least every 15 minutes - keeping the water fresh and cat worthy.

Hoomen should avoid unnecesery touching the water, and should be extra careful while handeling electricity and electric
appliance around water. 
stay safe and meow.
*/


#define MINUTE          60000UL // for millis()
#define MIN_DISTANCE    95

const byte irPin = A0;
const byte turbiditySensorPin = A4;

const byte fountainPin = 2;

int distance;

// for turbidity sampling
float turbidity_sensor_voltage;
float ntu;
const int WATER_SAMPLES = 600;

// states
bool close_by = false;

bool in_delay = false;

bool in_interval = false;

bool interval_on = false;

bool fountain_on = false;
/////

bool rgb_on = false;



// Timing

unsigned long away_time_count = 0; // pump will stay on for a duration of time after
                                   // an object is not detected anymore  
unsigned long to_auto_interval_count = 0; // auto water circulation interval delay
unsigned long periodic_time_on = 0; // of auto water circulation interval
unsigned long waiting_for_water_sampling = 0;

const unsigned long interval_duration = 15 * MINUTE; // intervals between auto circulation
const unsigned long periodic_on_duration = 3 * MINUTE; // time of interval's auto circulation
const unsigned long away_duration = 2 * MINUTE; // the time the fountain stays on after detected object left
const unsigned long water_sampling_interval = 1 * MINUTE; // sample water turbidity interval 

// RGB Led

const byte RPin = 9;
const byte GPin = 10;
const byte BPin = 11;

const byte RGB_PINS[] = {RPin, GPin, BPin};

byte r_val = 255;
byte g_val = 255;
byte b_val = 255;

byte RGB_VALS[] = {r_val, g_val, b_val};

void setup() {
    Serial.begin(9600);
    //switch
    pinMode(fountainPin, OUTPUT);
    pinMode(turbiditySensorPin, INPUT);
    pinMode(irPin, INPUT);
    // all the RGB led are in OUTPUT mode
    for (int pin = 9; pin < 12; pin++)
    {
        pinMode(pin, OUTPUT);
    }
}

void loop() {
    manage_distance();
    //delay(100);
    manageWaterTurbidityScanIntervals();
    manage_rgb_led();
    
    if (in_interval)
    {
        manage_interval();
    }
    if (interval_on)
    {
        start_periodic_pump();
    }
}

int scanForNearObjects()
{
    int num_samples = 30;
    int avg_reads[num_samples];
    for (int i = 0; i < num_samples; i++)
    {
        avg_reads[i] = analogRead(irPin);
        delayMicroseconds(200);
    }
    int avg_sum = 0;
    for (int i = 0; i < num_samples; i++)
    {
        avg_sum += avg_reads[i];
    }
    avg_sum = avg_sum / num_samples;
    
    return avg_sum;
}


void manage_distance()
{
    distance = scanForNearObjects(); 
    Serial.print("Ditance: "); Serial.println(distance);
    if(distance > MIN_DISTANCE)
    {
        Serial.println("close_by = true; in_delay = false");
        close_by = true;
        in_delay = false;
        away_time_count = 0;
        open_fountain();
    }
    // object left, we enter delay period
    else if (close_by == true && distance < MIN_DISTANCE)
    {
        close_by = false;
        in_delay = true;
        away_time_count = millis();
        manage_delay_count();
    }
    else if (close_by == false && distance < MIN_DISTANCE)
    // object has left and is not close by - meaning we need to check if
    // we are still in delay period, or the interval period
    {
        if (in_delay == true)
        {
            manage_delay_count();
        }
    }
}

void manage_delay_count()
{
    Serial.print("delay time: "); Serial.println(millis() - away_time_count);
    // if object comes back while in the delay duration
    // we reset delay counter
    if (in_delay == false && close_by == true)
    {
        away_time_count = 0;
    }
    // if object left we close the fountain only after the delay duration have passed
    else if (in_delay == true && close_by == false)
    {
        if ((millis() - away_time_count) > away_duration)
        // delay duration passed
        {
            in_delay = false;
            stop_fountain();
            away_time_count = 0;
            // when the delay counter finish
            // enter interval mode
            in_interval = true;
            to_auto_interval_count = millis();
            manage_interval();
        }
    }
    else if (in_delay == true && close_by == true)
    {
       in_delay = false;
       away_time_count = 0;
    }
    else if (in_delay == false && close_by == false && fountain_on == false)
    {
        if(in_interval = true)
        {
            manage_interval();
        }
        else
        // No object was detected while counting the 15 minutes auto water circulation interval delay
        // we start the pump to let water circulate for a duration of time, after making sure the fountain
        // is not already on, and that we are not in the post object delay period
        {
            start_periodic_pump();
        }
    }

}

void manage_interval()
{
    Serial.print("interval count: "); Serial.println(millis() - to_auto_interval_count);
   if (in_interval == true && close_by == false)
   {    
     if((millis() - to_auto_interval_count) > interval_duration) 
     {
        // no object is detected by the IR sensor while in the auto water circulation interval delay
        // the fountain is not in the post-object delay, either
        if (in_delay == true || close_by == true)
        {
            to_auto_interval_count = 0;
            in_interval = false;
        }
        else
        {
            open_fountain();
            periodic_time_on = millis();
            in_interval = false;
            interval_on = true;
        }
     }
   }
   else if (in_interval == true && close_by == true)
   // an object was detected while in the water circulation interval delay
   // stop counting and reset count
   {
        to_auto_interval_count = 0;
        in_interval = false;
   }
}

void start_periodic_pump()
{
    if (interval_on == true) {
        Serial.print("periodic on: "); Serial.println(millis() - periodic_time_on);
        if (millis() - periodic_time_on > periodic_on_duration)
        {
            interval_on = false;
            stop_fountain();
            periodic_time_on = 0;
            if (in_delay == false && close_by == false && fountain_on == false)
            {
                in_interval = true;
                to_auto_interval_count = millis();
            }
        }
        else if(close_by == true || in_delay == true)
        {
            interval_on = false;
            periodic_time_on = 0;
        }
    }
}

void stop_fountain()
{
    fountain_on = false;
    Serial.println("Fountain is closing...");
    digitalWrite(fountainPin, LOW);
}

void open_fountain()
{
    fountain_on = true;
    Serial.println("Fountain is starting...");
    digitalWrite(fountainPin, HIGH);
}

void  manage_rgb_led()
{
    if (fountain_on)
    {
        
        RGB_VALS[0] = 255;
        RGB_VALS[1] = 0;
        RGB_VALS[2] = 0;
    }
    else
    {
        RGB_VALS[0] = 0;
        RGB_VALS[1] = 255;
        RGB_VALS[2] = 0;
    }
    
    for (int pin = 0; pin < 3; pin++)
    {
         analogWrite(RGB_PINS[pin], RGB_VALS[pin]);
    }
//    if(ntu) {
//        static byte rgb_factor;
//
//        
//        
//        RGB_VALS[0] = ntu;
//        RGB_VALS[1] = -ntu;
//        RGB_VALS[2] = 0;
//    
//        for (int pin = 0; pin < 3; pin++)
//        {
//            analogWrite(RGB_PINS[pin], RGB_VALS[pin]);
//        }
//    }
}

void manageWaterTurbidityScanIntervals()
{
    if ((millis() - waiting_for_water_sampling) > water_sampling_interval)
    {
        scanWaterTurbidity();
        
        waiting_for_water_sampling = millis();
    }
}

void scanWaterTurbidity()
{
    turbidity_sensor_voltage = 0;
    for (int i = 0; i <WATER_SAMPLES; i++)
    {
        turbidity_sensor_voltage += ((float)analogRead(turbiditySensorPin)/1023) * 5;
    }
    turbidity_sensor_voltage /= WATER_SAMPLES;
    Serial.println("Turbidity Voltage: " + String(turbidity_sensor_voltage));

    turbidity_sensor_voltage = round_to_decimal_point(turbidity_sensor_voltage, 2);

    if (turbidity_sensor_voltage < 2.5) { ntu = 3000;}
    else
    {
        ntu = -1120.4*square(turbidity_sensor_voltage) +5742.3 * turbidity_sensor_voltage - 4352.9;
        Serial.println("NTU value: " + String(ntu));
    }
    ntu = map(ntu, 0, 3000, 0, 255);
    constrain(ntu, 0, 255);
}

float round_to_decimal_point(float in_value, int decimal_place)
{
    float multiplier = powf(10.0f, decimal_place);
    in_value = roundf(in_value * multiplier)/ multiplier;
    return in_value;
}
