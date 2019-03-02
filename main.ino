#include "LedControl.h"
#include "EEPROM.h"
#include "LiquidCrystal.h"

// Change the following for joystick / potentiometer (both work the same)
#define X_PIN A2    // X axis control
#define BTN_PIN 13  // Button control

// Digital Arduino pins in use:
const byte D2 = 2;
const byte D3 = 3;
const byte D4 = 4;
const byte D5 = 5;
const byte D6 = 6;
const byte D7 = 7;

// Only use Serial for debugging purposes, it could crash the LCD Display
const int BAUD_RATE =  9600;

// V0_PIN uses PWM to control display intensity
const byte V0_PIN =  9;
const int gLEDBacklight = 90; 


// Game dificulty is directly influenced by the following variable:
int gSpeed = 140;

long gTimer = 0;

// If i-th column is under fire, gUnderFire[i] == true
bool gUnderFire[9];

bool gMessageDisplay = false;

// Normal gamescores vary between 10 and 1000
const int gScoreModifier = 100;

// We use two of these because they may be updated concurrently
long gMillis2 = 0;
long gMillis = 0;

class Matrix {
// SINGLETON 
// The Matrix class wraps around LedControl               
// With minimal effort, it could be changed with another type of square display

  private:
    const int dataPin = 12;
    const int clockPin = 11;
    const int csPin = 10;
    const int numDevices = 1;
    LedControl ledMatrix = LedControl(dataPin, clockPin, csPin, numDevices);

    Matrix() {
        ledMatrix.shutdown(0, false); 
        ledMatrix.setIntensity(0, 2); 
        ledMatrix.clearDisplay(0);
    }
  public:
  
    static Matrix& instance() {
        static Matrix INSTANCE;
        return INSTANCE;
    }

    void clearMatrix() {
        ledMatrix.clearDisplay(0);
    }

    void ledOn(int row, int col) {
        ledMatrix.setLed(0, row - 1, col - 1, true);
    }

    void ledOff(int row, int col) {
        ledMatrix.setLed(0, row - 1, col - 1, false);
    }

};

Matrix matrix = Matrix::instance();


class LCD {
    // SINGLETON
    // Makes use of a 16x2 screen for message display

  private:

    LiquidCrystal lcd = LiquidCrystal(D2, D3, D4, D5, D6, D7);
    LCD() {
        lcd.begin(16, 2);
        lcd.clear();
    }

  public:

    static LCD& instance() {
        static LCD INSTANCE;
        return INSTANCE;
    }

    void clear() {
        lcd.clear();
    }

    void printLCD(String gMessageDisplay, int row, int col) {
        lcd.setCursor(col, row);  
        lcd.print(gMessageDisplay);
    }

};

LCD display = LCD::instance();


class GameMenu {
    // Uses the LCD screen to communicate
  public:

    void welcomeMessage(LCD display) {
        display.clear();
        display.printLCD("Spaceworms", 0, 3);
        display.printLCD("Press X to start", 1, 0);
    }

    void gameOverMessage(LCD display, long score) {
        // We display the highscore in the game over message
        //  using EEPROM memory available on the board
        display.clear();

        display.printLCD("Game Over! ", 0, 1);
        display.printLCD(String(score/gScoreModifier), 0, 12);

        display.printLCD("Highscore: ", 1, 1);
        long highscore;
        EEPROM.get(0, highscore);
        display.printLCD(String(highscore/gScoreModifier), 1 ,12);
    }
    void highscoreScreen(LCD display, long score) {
        display.clear();
        display.printLCD("Highscore", 0, 1);
        display.printLCD(String(score/gScoreModifier),0 ,12);
    }

} gameMenu;

class GameObject {
    
  public:
    virtual void draw() = 0;
    virtual void clear() = 0;
};


class Control {
    // Works regardless of analog input alternative 
  public:

    Control() {
        pinMode(BTN_PIN, INPUT);
        digitalWrite(BTN_PIN, HIGH);
    }

    bool shootListener() {
        bool press = digitalRead(BTN_PIN);
        return press; 
    }

    int moveListener() {
        int input = analogRead(X_PIN);
        int mappedInput = map(input, 0, 1030, 1, 9);
        return mappedInput;
    }

} joystick; 



class GameScreen : public GameObject {
    // SINGLETON
    // The game screen is 9x9 instead of 8x8 to avoid border collisions 

  private:

    bool screen[9][9];
    GameScreen() {
        clear();
    }
  public:
    static GameScreen& instance() {
            static GameScreen INSTANCE;
            return INSTANCE;
    }

    void clear() {
        for (int i = 1; i <= 8; i++) {
            for (int j = 1; j <= 8; j++) {
                screen[i][j] = false;
            }
        }
    }

    void clearMissiles() {
        for (int i = 1; i <= 8; i++) {
            gUnderFire[i] = false;
        }
    }
    void draw() {
        //  This function feeds frames to the screen 

        for (int i = 1; i <= 8; i++) {
            for (int j = 1; j <= 8; j++) {
                if (screen[i][j]) {
                    matrix.ledOn(i, j);
                } else {
                    matrix.ledOff(i, j);
                }
            }
        }
    }

    // bitOn / bitOff methods provide logical object creation mechanisms
    // For physical interaction with the LED matrix, check Matrix class
    void bitOn(int row, int col) {
        screen[row][col] = true;
    }

    void bitOff(int row, int col) {
        screen[row][col] = false;
    }

};
GameScreen mainScreen = GameScreen::instance(); 


class Spaceship : public GameObject{

  private:

    const int initialPosition = 4;
    const int yAxisLocked = 8;
    int position;

  public:

    Spaceship(){
        position = initialPosition;
        draw();
    }

    void clear() {
        mainScreen.bitOff(yAxisLocked - 1, position    );
        mainScreen.bitOff(yAxisLocked,     position - 1);
        mainScreen.bitOff(yAxisLocked,     position    );
        mainScreen.bitOff(yAxisLocked,     position + 1);
    }

    void draw() {
        mainScreen.bitOn(yAxisLocked - 1, position    );
        mainScreen.bitOn(yAxisLocked,     position - 1);
        mainScreen.bitOn(yAxisLocked,     position    );
        mainScreen.bitOn(yAxisLocked,     position + 1);
    }

    void move(int position) {
        clear();
        this->position = position;
        draw();
    }

    void signalShoot(int col) {
        long currentTime;
        gUnderFire[col] = true;
        if (currentTime - gMillis >= 200) {
            gUnderFire[col] = false;
            gMillis = currentTime;
        }
    }

    void gun(bool shoot) {
        int trajectory = position;
        int startPosition = yAxisLocked - 2;

        if (shoot) {
            for (int i = startPosition; i >= 0; i--) {
                mainScreen.bitOn(i, trajectory);
                mainScreen.draw();
            }
            for (int i = startPosition; i >= 0; i--) {
                mainScreen.bitOff(i, trajectory);
                mainScreen.draw();
            }
            signalShoot(position); 
        }

    }
} spaceship;


class GameMaster {
    // In charge with the game flow. 
  private:

    bool running;
    bool end;
  
  public:

    GameMaster() {
        end = false;
        running = false;
    }

    bool isRunning() {
        return running;
    }

    void startGame(GameScreen &gameScreen) {
        gMessageDisplay = false;
        gTimer = millis();
        if (joystick.shootListener()) {
            gameMenu.welcomeMessage(display);
            running = true;
        }
    }

    void endGame() {
        running = false;
    }

    void drawGameOverScreen(GameScreen &gameScreen) {
        gameScreen.clear();
        for (int i = 1; i <= 8; i++) {
            gameScreen.bitOn(i, i);
        }
        gameScreen.draw();
    }
    
    long getHighScore() {
        long highscore;
        EEPROM.get(0, highscore);
        return highscore;
    }

    void setHighScore(long currentScore) {
        if (getHighScore() < currentScore) {
            EEPROM.put(0, currentScore);
        }
    }

    void gameOver(GameScreen &gameScreen) {
        long score = millis() - gTimer;
        setHighScore(score);
        gameMenu.gameOverMessage(display, score);

        end = true;
        running = false;

        gameScreen.clear();
        drawGameOverScreen(gameScreen);

        gTimer = millis();
    }

    void isGameOver() {
        return end;
    }

} gameMaster;


class Invader {
  private:

    int posCol;
    int posRow;
    int singleSpeed;
    bool spawned;
    bool isMoving;

  public:

    Invader() = default; 
    
    Invader(int posRow, int posCol, int singleSpeed) {
        this->posRow = posRow;
        this->posCol = posCol;
        this->singleSpeed = gSpeed;
        this->spawned = true;
        this->isMoving = false;
        drawInvader();
    }

    void drawInvader() {
        mainScreen.bitOn(posRow - 1, posCol);
        mainScreen.bitOn(posRow    , posCol);
    }

    void clearInvader() {
        mainScreen.bitOff(posRow - 1, posCol);
        mainScreen.bitOff(posRow    , posCol);
    }   

    bool isSpawned() {
        return spawned;
    }

    void despawn() {
        if (isMoving) {
            spawned = false;
            clearInvader();
        }
    }
    
    void signalGameOver(GameMaster &gameMaster){
        gameMaster.gameOver(mainScreen);
    }

    void move(int){
        // Invaders move using this function
        // 
        // If a column is under fire (gUnderFire[col] == true), the invader residing
        // in that particular column instantly dies.
         

        isMoving = true;
        long currentTime = millis();
        int directionRow = 1;
        int directionCol = 1;
        if (posRow <= 6) {

            if (currentTime - gMillis >= gSpeed) {

                clearInvader();
                posRow = posRow + directionRow;
                posCol = posCol;
                if (posRow == 7 && !gUnderFire[posCol]) {
                    signalGameOver(gameMaster);
                }
                
                if (gUnderFire[posCol] && isMoving) {
                    this->despawn();
                    gUnderFire[posCol] = false;
                } else {
                    drawInvader();
                }
                
                gMillis = currentTime;
            }
        }
    }
} invaders[9];



void gDrawInvaders(int gSpeed) {
    // New invaders are spawned on empty cells. Their speed increases by 10ms every 10 
    //  seconds.

    const int speedUpInterval = 10000; // ms

    if (millis() - gMillis2 > speedUpInterval) {
        gSpeed -= 10; // ms
        gMillis2 = millis();
    }

    for (int col = 1; col <= 8; col++) {
        if (!invaders[col].isSpawned()) {
            invaders[col] = Invader(random(1,2), col, random(gSpeed, gSpeed + 100));
        }
    }
    
}


void setup() {

    pinMode(V0_PIN, OUTPUT); 
    analogWrite(V0_PIN, gLEDBacklight); 

    // When not connected, pins generate noise which is a decent random seed
    //  hence the usage of analog pin 0 
    randomSeed( analogRead(A0) );

    gameMenu.welcomeMessage(display);
}


void loop() {
    if ( gameMaster.isRunning() ) {
        if(!gMessageDisplay){

            display.printLCD("Shoot 'em all!", 0, 0);
            display.printLCD("Speed increases!", 1, 0);
            gMessageDisplay = true;

        }

        gDrawInvaders(gSpeed);
        mainScreen.draw(); 

        int rand = random(1,9);  // Columns start at 1 and they end at 8
        if ( invaders[rand].isSpawned() ) {
            invaders[rand].move(0);
        }
        spaceship.move(joystick.moveListener());
        spaceship.gun(joystick.shootListener());
        
    } else {
        if ( joystick.shootListener() ) {
            for (int col = 1; col <= 8; col++) {
                if(invaders[col].isSpawned()) 
                    invaders[col].despawn();
            }
            for (int col = 1; col <= 8; col++) {
                gUnderFire[col] = 0;
            }
            mainScreen.clear();
            gameMaster.startGame(mainScreen);
            display.clear();
        }
    }
}