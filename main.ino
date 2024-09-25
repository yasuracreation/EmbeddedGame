#include <LiquidCrystal.h>
#include <stdio.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <stdlib.h>
#include <unistd.h>
#include <util/delay.h>

#define PIN_AUTOPLAY 1
#define PORT_AUTOPLAY PORTD // Replace this with the appropriate port
#define SPRITE_RUN1 1
#define SPRITE_RUN2 2
#define SPRITE_JUMP 3
#define SPRITE_JUMP_UPPER '.' // Use the '.' character for the head
#define SPRITE_JUMP_LOWER 4
#define SPRITE_TERRAIN_EMPTY ' ' // User the ' ' character
#define SPRITE_TERRAIN_SOLID 5
#define SPRITE_TERRAIN_SOLID_RIGHT 6
#define SPRITE_TERRAIN_SOLID_LEFT 7

#define HERO_HORIZONTAL_POSITION 1 // Horizontal position of hero on screen

#define TERRAIN_WIDTH 16
#define TERRAIN_EMPTY 0
#define TERRAIN_LOWER_BLOCK 1
#define TERRAIN_UPPER_BLOCK 2

#define HERO_POSITION_OFF 0         // Hero is invisible
#define HERO_POSITION_RUN_LOWER_1 1 // Hero is running on lower row (pose 1)
#define HERO_POSITION_RUN_LOWER_2 2 //
#define HERO_POSITION_JUMP_2 4      // Half-way up
#define HERO_POSITION_JUMP_1 3      // Starting a jump
#define HERO_POSITION_JUMP_2 4      // Half-way up
#define HERO_POSITION_JUMP_3 5      // Jump is on upper row
#define HERO_POSITION_JUMP_4 6      // Jump is on upper row
#define HERO_POSITION_JUMP_5 7      // Jump is on upper row
#define HERO_POSITION_JUMP_6 8      // Jump is on upper row
#define HERO_POSITION_JUMP_7 9      // Half-way down
#define HERO_POSITION_JUMP_8 10     // About to land

#define HERO_POSITION_RUN_UPPER_1 11 // Hero is running on upper row (pose 1)
#define HERO_POSITION_RUN_UPPER_2 12 //
#define MAX_TRIES 3
#define MAX_SCORE = 15
#define F_CPU 16000000UL

#define SPEAKER_PIN PD3 // Pin where the speaker/buzzer is connected
#define buzzerPin 3
#define BAUD 9600
#define UBRR_VALUE ((F_CPU / (16UL * BAUD)) - 1)
int sys = 0; // maintain the status of changing display to home to menu page
int menu = 1;
int active = 0;
int tries = 0;
int totalMarks = 0;
static bool playing = false;
int gameBeginNew = 0;
bool isWinner = false;

static char terrainUpper[TERRAIN_WIDTH + 1];
static char terrainLower[TERRAIN_WIDTH + 1];
static bool buttonPushed = false;
char username[6]; // To store the username (5 characters + null terminator)

const int rs = 13, en = 12, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
const int START = 0, AUDIO = 1, EDIT_USER = 2, EXIT = 3;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
volatile uint16_t timerOverflowCount = 0;
ISR(INT0_vect)
        {
                buttonPushed = true;
        }
ISR(TIMER1_COMPA_vect) {
    timerOverflowCount++; // Increment the overflow count
}

const uint16_t melody2[] = {1500, 0, 1500, 0, 1500, 0, 1500, 0};
const uint16_t durations2[] = {300, 100, 300, 100, 300, 100, 300, 100};

int main()
{
    /*
     Set port D bits to 11110000 (key pad).
     Rows are denoted by first 4 digits and
     columns are denoted by last 4 digits.
     */
    // PORTD |= 0XF0;
    PORTD = (PORTD & 0x0F);
    PORTB |= 0x0F;

    // Variable to store column index.
    int c;

    // Variable to store row index.
    int r;

    // input cursor column position.
    int input_cursor_col = 0;
    int input_cursor_row = 0;
    DDRD &= ~0x04;
    PORTD |= 0x04;
    EICRA |= (1 << ISC01);
    EICRA &= ~(1 << ISC00);

    sei();
    EIMSK |= (1 << INT0);


    /*
    This is to set the current screen.
    0 - Splash screen.
    1 - Enter username.
    2 - Menu page
    */
    int current_screen = 0;

    // Key pad matrix.
    int keys[4][4] = {
            {'1', '2', '3', 'A'},
            {'4', '5', '6', 'B'},
            {'7', '8', '9', 'C'},
            {'*', '0', '#', 'D'}};

    char menu[4][20] = {"Start", "Audio volume", "Edit username", "Exit"};
    // Update num_of_menu_items value if more menu items added.
    int num_of_menu_items = 4;
    int menu_cursor_position = 0;



    // set up the LCD's number of columns and rows.
    lcd.begin(16, 2);

    //--Splash screen
    lcd.setCursor(0, 0);
    lcd.print("JUMPING GAME");
    custom_delay_ms(500);
    lcd.clear();

    lcd.setCursor(4, 1);
    lcd.print("JUMPING GAME");
    custom_delay_ms(500);
    lcd.clear();

    lcd.setCursor(0, 1);
    lcd.print("JUMPING GAME");
    custom_delay_ms(500);
    lcd.clear();

    lcd.setCursor(4, 0);
    lcd.print("JUMPING GAME");
    custom_delay_ms(500);
    lcd.clear();
    //--Splash screen end

    current_screen = 1;
    lcd.print("Username?");
    char username[6]; // To store the username (5 characters + null terminator)

    setupPWM();
    setupTimer();

    while (true)
    {
        if (current_screen != 3)
        {
            for (c = 0; c < 4; c++)
            {
                /*
                Set the direction of DDRD register (Key pad).

                0X08 is 00001000.
                When we get the value of c, we shift the 1 by c number of positions.
                Ex: if c is 2 we shift 1 by 2 positions.
                */
                DDRD = (0X80 >> c);

                for (r = 0; r < 4; r++)
                {

                    /*
                    Check if specific bit in PIND is clear or not

                    0X20 is 0010 0000
                    (0X80 )>> r => Here we shift the value 1 by r number of positions
                    and then do bitwise AND operation and get the negate.

                    Ex:
                    PIND       1101 0000
                    0X80 >> 2  0010 0000
                    AND        0000 0000
                    */
                    if (!(PINB & (0X08) >> r))
                    {
                        // Obtain the pressed key value from the matrix.
                        char pressedKey = keys[r][c];

                        if (current_screen == 1)
                        {
                            if (pressedKey != '#' && input_cursor_col < 5)
                            {
                                lcd.setCursor(input_cursor_col, 1);
                                lcd.print(pressedKey);
                                username[input_cursor_col] = pressedKey;
                                input_cursor_col += 1;
                            }
                            else if (pressedKey == '#' && input_cursor_col > 0)
                            {
                                // set current screen to menu

                                username[input_cursor_col] = '\0'; // Null-terminate the username
                                pressedKey = '0';
                                current_screen = 2;

                            }

                        }

                        if (current_screen == 2)
                        {
                            lcd.clear();
                            lcd.print(">");
                            lcd.setCursor(2, 0);
                            lcd.print(menu[menu_cursor_position]);

                            lcd.setCursor(0, 1);
                            lcd.print(menu[menu_cursor_position + 1]);

                            if (pressedKey == '2')
                            {
                                if (menu_cursor_position > 0)
                                {
                                    menu_cursor_position -= 1;
                                }

                                lcd.clear();
                                lcd.print(">");
                                lcd.setCursor(2, 0);
                                lcd.print(menu[menu_cursor_position]);

                                lcd.setCursor(0, 1);
                                lcd.print(menu[menu_cursor_position + 1]);
                            }

                            if (pressedKey == '8')
                            {

                                if (menu_cursor_position < num_of_menu_items - 1)
                                {
                                    menu_cursor_position += 1;
                                }

                                if (menu_cursor_position < num_of_menu_items - 1)
                                {
                                    lcd.clear();
                                    lcd.print(">");
                                    lcd.setCursor(2, 0);
                                    lcd.print(menu[menu_cursor_position]);

                                    lcd.setCursor(0, 1);
                                    lcd.print(menu[menu_cursor_position + 1]);
                                }
                                else
                                {
                                    lcd.clear();
                                    lcd.print(menu[menu_cursor_position - 1]);

                                    lcd.setCursor(0, 1);
                                    lcd.print(">");
                                    lcd.setCursor(2, 1);
                                    lcd.print(menu[menu_cursor_position]);
                                }
                            }
                        }

                        if (current_screen == 2)
                        {
                            if (pressedKey == '5')
                            {
                                lcd.clear();
                                lcd.print("- ");
                                if (menu_cursor_position == 0)
                                {
                                    initializeGraphics();
                                    gameBeginNew = 1;
                                    gameStart();
                                    current_screen = 3;
                                }
                            }
                        }

                        if (current_screen == 99)
                        {
                            if (pressedKey == '*')
                            {
                                current_screen = 2;
                            }
                        }
                        if (current_screen == 3)
                        {
                            gameStart();
                        }
                        else
                        {
                            custom_delay_ms(50);
                        }
                    }
                }
            }
        }
        if (current_screen == 3)
        {
            gameStart();

        }

    }

}
void setupTimer() {
    // Set Timer 1 to CTC mode (Clear Timer on Compare Match)
    TCCR1B |= (1 << WGM12);

    // Set the compare match value for 1 ms delay (16 MHz / 64 / 1000 Hz = 250)
    OCR1A = 250;

    // Enable Timer 1 Compare A interrupt
    TIMSK1 |= (1 << OCIE1A);

    // Set prescaler to 64
    TCCR1B |= (1 << CS11) | (1 << CS10);

    // Enable global interrupts
    sei();
}
void custom_delay_ms(uint16_t milliseconds) {
    timerOverflowCount = 0; // Reset overflow count

    // Calculate the number of overflows required for the delay
    uint16_t totalOverflows = (milliseconds * F_CPU) / (64UL * 1000UL);

    // Wait until the required number of overflows occur
    while (timerOverflowCount < totalOverflows) {
        // Do nothing, wait for the interrupt to increment timerOverflowCount
    }
}




void setupPWM() {
    DDRD |= (1 << SPEAKER_PIN);

    // Configure Timer1 for Fast PWM mode, non-inverted output on OC1A
    TCCR2A = (1 << COM2A1) | (1 << WGM21);
    TCCR2B = (1 << WGM22) | (1 << CS20);

    // Set initial frequency (example: 1000 Hz)
    OCR2A = F_CPU / (2 * 1 * 500) - 1;
    // Enable global interrupts
    sei();

//    playMelody(melody1, durations1, sizeof(melody1) / sizeof(melody1[0]));
//    custom_delay_ms(500); // Wait between melodies

    playMelody(melody2, durations2, sizeof(melody2) / sizeof(melody2[0]));
    custom_delay_ms(500); // Wait between melodies

//    playSound(2000, 100);
}


void playSound(uint16_t frequency, uint16_t duration_ms) {
    OCR2A = F_CPU / (2 * 1 * 1000) - 1; // Set frequency
    custom_delay_ms(800);
    OCR2A = 0; // Turn off sound


    uint16_t cycles = (F_CPU / (2 * frequency));

    // Calculate the number of toggles needed for the desired duration
    uint16_t toggles = (duration_ms * frequency) / 1500;

    for (uint16_t i = 0; i < toggles; ++i) {
        PORTD ^= (1 << SPEAKER_PIN); // Toggle the buzzer pin
        custom_delay_ms(15); // Delay for half the period
    }

    PORTD &= ~(1 << SPEAKER_PIN); // Turn off the buzzer at the end
}


void playTone(uint16_t frequency, uint16_t duration_ms) {
    // Calculate the number of cycles required for the desired frequency
    uint16_t cycles = (F_CPU / (2 * frequency));

    // Calculate the number of toggles needed for the desired duration
    uint16_t toggles = (duration_ms * frequency) / 1000;

    for (uint16_t i = 0; i < toggles; ++i) {
        PORTD ^= (1 << SPEAKER_PIN); // Toggle the buzzer pin
        _delay_us(cycles); // Delay for half the period
    }

    PORTD &= ~(1 << SPEAKER_PIN); // Turn off the buzzer at the end
}

void playMelody(const uint16_t *melody, const uint16_t *durations, uint8_t length) {
    for (uint8_t i = 0; i < length; ++i) {
        playTone(melody[i], durations[i]);
        custom_delay_ms(150); // Add a short delay between notes
    }
}

void initializeGraphics()
{
    static byte graphics[] = {
            // Run position 1
            B01100,
            B01100,
            B00000,
            B01110,
            B11100,
            B01100,
            B11010,
            B10011,
            // Run position 2
            B01100,
            B01100,
            B00000,
            B01100,
            B01100,
            B01100,
            B01100,
            B01110,
            // Jump
            B01100,
            B01100,
            B00000,
            B11110,
            B01101,
            B11111,
            B10000,
            B00000,
            // Jump lower
            B11110,
            B01101,
            B11111,
            B10000,
            B00000,
            B00000,
            B00000,
            B00000,
            // Ground
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            B11111,
            // Ground right
            B00011,
            B00011,
            B00011,
            B00011,
            B00011,
            B00011,
            B00011,
            B00011,
            // Ground left
            B11000,
            B11000,
            B11000,
            B11000,
            B11000,
            B11000,
            B11000,
            B11000,
    };
    int i;
    // Skip using character 0, this allows lcd.print() to be used to
    // quickly draw multiple characters
    for (i = 0; i < 7; ++i)
    {
        lcd.createChar(i + 1, &graphics[i * 8]);
    }
    for (i = 0; i < TERRAIN_WIDTH; ++i)
    {
        terrainUpper[i] = SPRITE_TERRAIN_EMPTY;
        terrainLower[i] = SPRITE_TERRAIN_EMPTY;
    }
}

/**
 * game run function
 */
void gameStart()
{

    static byte heroPos = HERO_POSITION_RUN_LOWER_1;
    static byte newTerrainType = TERRAIN_EMPTY;
    static byte newTerrainDuration = 1;

    static bool blink = false;
    static unsigned int distance = 0;

    if (!playing || isWinner)
    {
        lcd.clear();
        lcd.setCursor(0, 0);
        drawHero((blink) ? HERO_POSITION_OFF : heroPos, terrainUpper, terrainLower, distance >> 3);
        if (blink)
        {
            if(isWinner){
                showWinnerScreen();
            } else{
                showGameBlinkScreen();

            }
        }

        custom_delay_ms(1000);
        blink = !blink;
        Serial.print(buttonPushed);
        /**
         * update playing state and initialized the charactor graphic function
         */
        if (buttonPushed)
        {
            Serial.print("User: 5");
            initializeGraphics();
            playing = true;       // start pay functions one paying is true
            buttonPushed = false; // reset button action to false
            distance = 0;         // initialized the score
            if(tries == MAX_TRIES){
                tries = 0;
                totalMarks = 0;
            }
            if(isWinner){
                tries = 0;
                totalMarks = 0;
                isWinner = false;
            }

        }
    }
    else
    {
        /**
         * continue the game actions now game is in playing state
         *
         */
        advanceTerrain(terrainLower, newTerrainType == TERRAIN_LOWER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);
        advanceTerrain(terrainUpper, newTerrainType == TERRAIN_UPPER_BLOCK ? SPRITE_TERRAIN_SOLID : SPRITE_TERRAIN_EMPTY);

        if (--newTerrainDuration == 0)
        {
            if (newTerrainType == TERRAIN_EMPTY)
            {
                newTerrainType = (random(3) == 0) ? TERRAIN_UPPER_BLOCK : TERRAIN_LOWER_BLOCK;
                newTerrainDuration = 2 + random(10);
            }
            else
            {
                newTerrainType = TERRAIN_EMPTY;
                newTerrainDuration = 10 + random(10);
            }
        }

        if (buttonPushed)
        {
            if (heroPos <= HERO_POSITION_RUN_LOWER_2)
                heroPos = HERO_POSITION_JUMP_1;
            buttonPushed = false;
        }

        if (drawHero(heroPos, terrainUpper, terrainLower, distance >> 3))
        {
            playing = false; // The hero collided with something. Too bad.
            // Play a short beep sound
            playSound(1000, 200); // Play a 1000 Hz sound for 200 ms

            // Wait for a moment
            custom_delay_ms(500);

            // Play a longer low-pitched sound
            playSound(200, 1000); // Play a 200 Hz sound for 1000 ms
        }

        else
        {
            if (heroPos == HERO_POSITION_RUN_LOWER_2 || heroPos == HERO_POSITION_JUMP_8)
            {
                heroPos = HERO_POSITION_RUN_LOWER_1;
            }
            else if ((heroPos >= HERO_POSITION_JUMP_3 && heroPos <= HERO_POSITION_JUMP_5) && terrainLower[HERO_HORIZONTAL_POSITION] != SPRITE_TERRAIN_EMPTY)
            {
                heroPos = HERO_POSITION_RUN_UPPER_1;
            }
            else if (heroPos >= HERO_POSITION_RUN_UPPER_1 && terrainLower[HERO_HORIZONTAL_POSITION] == SPRITE_TERRAIN_EMPTY)
            {
                heroPos = HERO_POSITION_JUMP_5;
            }
            else if (heroPos == HERO_POSITION_RUN_UPPER_2)
            {
                heroPos = HERO_POSITION_RUN_UPPER_1;
            }
            else
            {
                ++heroPos;
            }
            ++distance; // increment the score distance
            bool pinValue = (terrainLower[HERO_HORIZONTAL_POSITION + 2] == SPRITE_TERRAIN_EMPTY) ? true : false;
            if (pinValue)
            {
                PORT_AUTOPLAY |= (1 << PIN_AUTOPLAY); // Set the entire port to HIGH
            }
            else
            {
                PORT_AUTOPLAY &= ~(1 << PIN_AUTOPLAY); // Set the entire port to LOW
            }
        }
        custom_delay_ms(1);
    }
}
bool drawHero(byte position, char *terrainUpper, char *terrainLower, unsigned int score)
{
    bool collide = false;
    char upperSave = terrainUpper[HERO_HORIZONTAL_POSITION];
    char lowerSave = terrainLower[HERO_HORIZONTAL_POSITION];
    byte upper, lower;
    switch (position)
    {
        case HERO_POSITION_OFF:
            upper = lower = SPRITE_TERRAIN_EMPTY;
            break;
        case HERO_POSITION_RUN_LOWER_1:
            upper = SPRITE_TERRAIN_EMPTY;
            lower = SPRITE_RUN1;
            break;
        case HERO_POSITION_RUN_LOWER_2:
            upper = SPRITE_TERRAIN_EMPTY;
            lower = SPRITE_RUN2;
            break;
        case HERO_POSITION_JUMP_1:
        case HERO_POSITION_JUMP_8:
            upper = SPRITE_TERRAIN_EMPTY;
            lower = SPRITE_JUMP;
            break;
        case HERO_POSITION_JUMP_2:
        case HERO_POSITION_JUMP_7:
            upper = SPRITE_JUMP_UPPER;
            lower = SPRITE_JUMP_LOWER;
            break;
        case HERO_POSITION_JUMP_3:
        case HERO_POSITION_JUMP_4:
        case HERO_POSITION_JUMP_5:
        case HERO_POSITION_JUMP_6:
            upper = SPRITE_JUMP;
            lower = SPRITE_TERRAIN_EMPTY;
            break;
        case HERO_POSITION_RUN_UPPER_1:
            upper = SPRITE_RUN1;
            lower = SPRITE_TERRAIN_EMPTY;
            break;
        case HERO_POSITION_RUN_UPPER_2:
            upper = SPRITE_RUN2;
            lower = SPRITE_TERRAIN_EMPTY;
            break;
    }
    if (upper != ' ')
    {
        terrainUpper[HERO_HORIZONTAL_POSITION] = upper;
        collide = (upperSave == SPRITE_TERRAIN_EMPTY) ? false : true;
    }
    if (lower != ' ')
    {
        terrainLower[HERO_HORIZONTAL_POSITION] = lower;
        collide |= (lowerSave == SPRITE_TERRAIN_EMPTY) ? false : true;
    }

    byte digits = (score > 20) ? 5 : (score > 15) ? 4
                                                  : (score > 12)    ? 3
                                                                    : (score > 9)     ? 2
                                                                                      : 1;
    if (collide && playing)
    {
        tries += 1;
        totalMarks = score;
    }
    if(score > 15){
        isWinner = true;
         playMelody(melody2, durations2, sizeof(melody2) / sizeof(melody2[0]));
    }
    // Draw the scene
    terrainUpper[TERRAIN_WIDTH] = '\0';
    terrainLower[TERRAIN_WIDTH] = '\0';
    char temp = terrainUpper[16 - digits];
    terrainUpper[16 - digits] = '\0';
    lcd.setCursor(0, 0);
    lcd.print(terrainUpper);
    terrainUpper[16 - digits] = temp;
    lcd.setCursor(0, 1);
    lcd.print(terrainLower);

    lcd.setCursor(16 - digits, 0);
    lcd.print(score);

    terrainUpper[HERO_HORIZONTAL_POSITION] = upperSave;
    terrainLower[HERO_HORIZONTAL_POSITION] = lowerSave;
    return collide;
}
/**
 * getting amount of mini seconds as an argument and calculate the number of cycle that need to be run
 * then preform the loop accordingly
 */
// void custom_delay_ms(uint16_t milliseconds)
// {
//     // Calculate the number of cycles required for the delay
//     uint32_t cycles = ((uint32_t)milliseconds * F_CPU) / 1000;

//     // Perform the delay by looping
//     for (uint32_t i = 0; i < cycles; ++i)
//     {
//         asm volatile("nop");
//     }
// }

void advanceTerrain(char *terrain, byte newTerrain)
{
    for (int i = 0; i < TERRAIN_WIDTH; ++i)
    {
        char current = terrain[i];
        char next = (i == TERRAIN_WIDTH - 1) ? newTerrain : terrain[i + 1];
        switch (current)
        {
            case SPRITE_TERRAIN_EMPTY:
                terrain[i] = (next == SPRITE_TERRAIN_SOLID) ? SPRITE_TERRAIN_SOLID_RIGHT : SPRITE_TERRAIN_EMPTY;
                break;
            case SPRITE_TERRAIN_SOLID:
                terrain[i] = (next == SPRITE_TERRAIN_EMPTY) ? SPRITE_TERRAIN_SOLID_LEFT : SPRITE_TERRAIN_SOLID;
                break;
            case SPRITE_TERRAIN_SOLID_RIGHT:
                terrain[i] = SPRITE_TERRAIN_SOLID;
                break;
            case SPRITE_TERRAIN_SOLID_LEFT:
                terrain[i] = SPRITE_TERRAIN_EMPTY;
                break;
        }
    }
}
void showGameBlinkScreen() {
    lcd.clear();
    lcd.setCursor(0, 0);

    if (tries == 0) {
        lcd.print("Press Start");
    }
    else if (tries > 0 && tries < 3) {
        lcd.print("You have ");
        lcd.print(MAX_TRIES - tries);
        lcd.print(" tries left");

    }
    else if (tries == MAX_TRIES) {
        showGameEndScreen();
    }

}
void showGameEndScreen() {
    lcd.clear();

    lcd.setCursor(2, 0);
    lcd.print("................");
    lcd.setCursor(2, 1);
    lcd.print("....Game Over....");
    custom_delay_ms(1000);

    lcd.setCursor(0, 0);
    lcd.print("................");
    lcd.setCursor(0, 1);
    lcd.print(".... Total: ");
    lcd.print(totalMarks);
    lcd.print("....");
    custom_delay_ms(1000);

    lcd.setCursor(2, 0);
    lcd.print("................");
    lcd.setCursor(2, 1);
    lcd.print("................");
    custom_delay_ms(1000);

    lcd.setCursor(2, 0);
    lcd.print("....Game Over....");
    lcd.setCursor(2, 1);
    lcd.print("................");
    custom_delay_ms(1000);

    lcd.setCursor(2, 0);
    lcd.print("................");
    lcd.setCursor(2, 1);
    lcd.print(".... Total: ");
    lcd.print(totalMarks);
    lcd.print("....");
    custom_delay_ms(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("................");
    lcd.setCursor(2, 0);
    lcd.print("Press Start");
    lcd.setCursor(2, 1);
    lcd.print("New Game");
    custom_delay_ms(1000);
}
void showWinnerScreen() {
    lcd.clear();

    lcd.setCursor(2, 0);
    lcd.print("....Congraz....");
    lcd.setCursor(2, 1);
    lcd.print("....Congraz....");
    custom_delay_ms(1000);
    lcd.clear();
    lcd.setCursor(0, 0);
    //lcd.print(".... Total: ");
    lcd.print(username);
    lcd.print("....");
    custom_delay_ms(1000);
    lcd.clear();
    lcd.setCursor(2, 0);
    lcd.print("................");
    lcd.setCursor(2, 1);
    lcd.print(" You Are The");
    custom_delay_ms(1000);

    lcd.setCursor(2, 0);
    lcd.print("....WINNER....");
    lcd.setCursor(2, 1);
    lcd.print("................");
    custom_delay_ms(1000);

    lcd.setCursor(2, 0);
    lcd.print("................");
    lcd.setCursor(2, 1);
    lcd.print(".... Total: ");
    lcd.print(totalMarks);
    lcd.print("....");
    custom_delay_ms(1000);

    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("................");
    lcd.setCursor(2, 0);
    lcd.print("Press Start");
    lcd.setCursor(2, 1);
    lcd.print("New Game");
    custom_delay_ms(1000);
}
