#include "LedControl.h"
#include "EEPROM.h"
#include "LiquidCrystal.h"

// Only use Serial for debugging purposes, it messes up the LCD display
#define BAUD_RATE 9600

// Change the following for joystick / potentiometer
#define X_PIN A2
#define BTN_PIN 13

// V0_PIN uses PWM to control display intensity
#define V0_PIN 9 

bool gUnderFire[9];
long gTimer = 0;
bool gMessageDisplay = false;
long gMillis2 = 0;
long gMillis = millis();
int  gSpeed = 200;



class Matrix {
// The Matrix class wraps around LedControl               
// With minimal effort, it could be changed with another type of square display

  private:
    
    LedControl led_matrix = LedControl(12, 11, 10, 1);

  public:

    Matrix(){
        led_matrix.shutdown(0, false); 
        led_matrix.setIntensity(0, 2); 
        led_matrix.clearDisplay(0);
    }

    void clearMatrix() {
        led_matrix.clearDisplay(0);
    }

    void ledOn(int row, int col) {
        led_matrix.setLed(0, row - 1, col - 1, true);
    }

    void ledOff(int row, int col) {
        led_matrix.setLed(0, row - 1, col - 1, false);
    }

} matrix; // to be treated like Singleton


class LCD {
  private:

    LiquidCrystal lcd = LiquidCrystal(2, 3, 4, 5, 6, 7);

  public:

    LCD() {
        lcd.begin(16, 2);
        lcd.clear();
    }

    void clearScreen() {
        lcd.clear();
    }

    void printLCD(String gMessageDisplay, int row, int col) {
        lcd.setCursor(col, row);  
        lcd.print(gMessageDisplay);
    }

} display; // to be treated like Singleton


class GameMenu {

  public:

    void welcomeMessage(LCD display) {
        display.clearScreen();
        display.printLCD("Spaceworms", 0, 3);
        display.printLCD("Press X to start", 1, 0);
    }

    void gameOverMessage(LCD display, long score) {
        display.clearScreen();

        display.printLCD("Game Over! ", 0, 1);
        display.printLCD(String(score/100), 0, 12);

        display.printLCD("Highscore: ", 1, 1);
        long highscore;
        EEPROM.get(0, highscore);
        display.printLCD(String(highscore/100), 1 ,12);
    }
    void highscoreScreen(LCD display, long score) {
        display.clearScreen();
        display.printLCD("Highscore", 0, 1);
        display.printLCD(String(score/100),0 ,12);
    }

} gameMenu; // to be treated like Singleton


class Control {
    //  Works regardless of analog input alternative 

  public:

    Control() {
        pinMode(BTN_PIN, INPUT);
        digitalWrite(BTN_PIN, HIGH);
    }

    bool shootListener() {
        //  1 = shooting | 0 = otherwise
        bool press = digitalRead(BTN_PIN);
        return press; 
    }

    int moveListener() {
        int input = analogRead(X_PIN);
        int x = map(input, 0, 1030, 1, 9);
        return x;
    }

} joystick; // to be treated like Singleton



class GameScreen {
    //  The game screen is 9x9 to avoid border collisions 

  private:

    bool screen[9][9];
  
  public:
  
    GameScreen() {
        clearScreen();
    }

    void clearScreen() {
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

    void bitOn(int row, int col) {
        screen[row][col] = true;
    }

    void bitOff(int row, int col) {
        screen[row][col] = false;
    }

} mainScreen; // to be treated like Singleton


class Spaceship {

  private:

    const int initialPosition = 4;
    const int yAxisLocked = 8;
    int position;

  public:

    Spaceship(){
        position = initialPosition;
        drawSpaceship();
    }

    void clearSpaceship() {
        mainScreen.bitOff(yAxisLocked - 1, position    );
        mainScreen.bitOff(yAxisLocked,     position - 1);
        mainScreen.bitOff(yAxisLocked,     position    );
        mainScreen.bitOff(yAxisLocked,     position + 1);
    }

    void drawSpaceship() {
        mainScreen.bitOn(yAxisLocked - 1, position    );
        mainScreen.bitOn(yAxisLocked,     position - 1);
        mainScreen.bitOn(yAxisLocked,     position    );
        mainScreen.bitOn(yAxisLocked,     position + 1);
    }

    void move(int position) {
        clearSpaceship();
        this->position = position;
        drawSpaceship();
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
} spaceship; // to be treated like Singleton


class GameMaster {
    //  In charge with the game flow. 
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
        gameScreen.clearScreen();
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

        gameScreen.clearScreen();
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
    int gSpeed;
    bool spawned;
    bool isMoving;

  public:

    Invader() {} // = default 
    
    Invader(int posRow, int posCol, int gSpeed) {
        this->posRow = posRow;
        this->posCol = posCol;
        this->gSpeed = gSpeed;
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

    void move(){
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
    // seconds.

    const int speedUpInterval = 10000; // ms
    if(millis() - gMillis2 > speedUpInterval) {
        gSpeed -= 10;
        gMillis2 = millis();
    }

    for (int col = 1; col <= 8; col++) {
        if (!invaders[col].isSpawned()) {
            invaders[col] = Invader(random(1,2), col, random(gSpeed, gSpeed+200));
        }
    }
    
}


void setup() {

    pinMode(V0_PIN, OUTPUT); 
    analogWrite(V0_PIN, 90); 

    randomSeed( analogRead(0) );

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

        int rand = random(1,9);
        if ( invaders[rand].isSpawned() ) {
            invaders[rand].move();
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
            mainScreen.clearScreen();
            gameMaster.startGame(mainScreen);
            display.clearScreen();
        }
    }
}