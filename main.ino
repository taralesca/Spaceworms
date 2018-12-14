#include "LedControl.h"
#include "LiquidCrystal.h"
#define BAUD_RATE 9600
#define INF 99999999999999999
long long globalMillis = millis();

/* Change the following for joystick / potentiometer */
#define X_PIN A2
#define BTN_PIN 13

/* V0_PIN uses PWM to control display intensity */
#define V0_PIN 9 

bool underFire[9]; // !TODO


class Matrix {
    /*                  The Matrix class wraps around LedControl                    */
    /* With minimal effort, it could be changed with another type of square display */
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

} matrix; /* to be treated like Singleton */


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

    void printLCD(String message, int row, int col) {
        lcd.setCursor(col, row);  
        lcd.print(message);
    }

} display; /* to be treated like Singleton */


class GameMenu {
    /// TODO! - may completely remove this
  public:

    void welcomeMessage(LCD display) {
        display.printLCD("Spaceworms", 0, 3);
        display.printLCD("Press X to start", 1, 0);
    }

    void gameOverMessage(LCD display) {
        display.clearScreen();
        display.printLCD("Game Over", 0, 3);
        display.printLCD("X to restart", 1, 2);
    }

} gameMenu; /* to be treated like Singleton */


class Control {
    /* Works regardless of analog input alternative */

  public:
    Control() {
        pinMode(BTN_PIN, INPUT);
        digitalWrite(BTN_PIN, HIGH);
    }

    bool shootListener() {
        /* 1 = shooting | 0 = otherwise */
        bool press = digitalRead(BTN_PIN);
        return press; 
        /// !TODO
    }

    int moveListener() {
        int input = analogRead(X_PIN);
        int x = map(input, 0, 1030, 1, 9);
        return x;
    }

} joystick; /* to be treated like Singleton */



class GameScreen {
    /* The game screen is 9x9 to avoid border collisions */

  private:
    bool screen[9][9];
  public:
    gameScreen() {
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
        // !TODO 
        for (int i = 1; i <= 8; i++) {
            underFire[i] = false;
        }
    }
    void draw() {
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

} mainScreen; /* to be treated like Singleton */


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
        /// !TODO
        long long currentTime;
        underFire[col] = true;
        if (currentTime - globalMillis >= 200) {
            underFire[col] = false;
            globalMillis = currentTime;
        }
    }

    void gun(bool shoot) {
        int trajectory = position;
        int startPosition = yAxisLocked - 2;
        if (shoot) {
            for(int i = startPosition; i >= 0; i--){
                mainScreen.bitOn(i, trajectory);
                mainScreen.draw();
            }
            for(int i = startPosition; i >= 0; i--){
                mainScreen.bitOff(i, trajectory);
                mainScreen.draw();
            }
            signalShoot(position); 

        }
    }
} spaceship; /* to be treated like Singleton */


class GameMaster {
    /* In charge with the game flow. */
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
        if(joystick.shootListener()){
            gameMenu.welcomeMessage(display);
            running = true;
        }
    }

    void endGame() {
        running = false;
    }

    void drawGameOverScreen(GameScreen &gameScreen) {
        gameScreen.clearScreen();
        for (int i = 1; i <= 8; i++){
            gameScreen.bitOn(i, i);
        }
        gameScreen.draw();
    }

    void gameOver(GameScreen &gameScreen) {
        gameMenu.gameOverMessage(display);
        end = true;
        running = false;
        gameScreen.clearScreen();
        drawGameOverScreen(gameScreen);
    }

    void isGameOver() {
        return end;
    }

} gameMaster;


class Invader {
    int posCol;
    int posRow;
    int speed;
    bool spawned;
    bool isMoving;
  public:
    Invader() {} /* = default */
    
    Invader(int posRow, int posCol, int speed){
        this->posRow = posRow;
        this->posCol = posCol;
        this->speed = speed;
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
        isMoving = true;
        long long currentTime = millis();
        int directionRow = 1;
        int directionCol = 1;
        if (posRow <= 6) {

            if (currentTime - globalMillis >= speed) {

                clearInvader();
                posRow = posRow + directionRow;
                posCol = posCol;
                if (posRow == 7 && !underFire[posCol]) {
                    signalGameOver(gameMaster);
                }
                
                if (underFire[posCol] && isMoving) {
                    this->despawn();
                    underFire[posCol] = false;
                } else {
                    drawInvader();
                }
                
                globalMillis = currentTime;
            }
        }
    }
} invaders[9];




void drawInvaders() {
    /// !TODO
    for (int col = 1; col <= 8; col++) {
        invaders[col] = Invader(random(1,4), col, random(300, 500));
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
        mainScreen.draw();
        for (int i = 1; i <= 8; i++) {
            if ( invaders[i].isSpawned() ) {
                invaders[i].move();
            }
        }
        spaceship.move(joystick.moveListener());
        spaceship.gun(joystick.shootListener());
        
    } else {
        if ( joystick.shootListener() ) {
            for (int col = 1; col <= 8; col++) {
                underFire[col] = 0;
            }
            mainScreen.clearScreen();
            drawInvaders();
            gameMaster.startGame(mainScreen);
        }
    }
}