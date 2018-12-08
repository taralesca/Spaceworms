#include "LedControl.h"
#define X_pin A2
#define SW_pin 2

long long globalMillis = millis();
bool underFire[9]; // !TODO


class Matrix {
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


class Control {
  public:
    Control() {
        pinMode(SW_pin, INPUT);
        digitalWrite(SW_pin, HIGH);
    }

    bool shootListener() {
        /* 1 = shooting | 0 = otherwise */
        bool press = digitalRead(SW_pin);
        return press; 
        /// !TODO
    }

    int moveListener() {
        /* -1 = left | 0 = stay | 1 = right */
        int input = analogRead(X_pin);
        int x = map(input, 0, 1030, 1, 9);
        return x;
    }

} joystick; /* to be treated like Singleton */


class GameScreen {
  private:
    /* The game screen is 9x9 to avoid border collisions */
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
        // !TODO
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


class Invader {
    int posCol;
    int posRow;
    int speed;
    bool spawned;
  public:
    Invader(int posRow, int posCol, int speed){
        this->posRow = posRow;
        this->posCol = posCol;
        this->speed = speed;
        this->spawned = true;
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
        spawned = false;
        clearInvader();
    }

    void move(){
        long long currentTime = millis();
        int directionRow = 1;
        int directionCol = 1;
        if (posRow <= 5) {
            if (currentTime - globalMillis >= speed) {
                clearInvader();
                posRow = posRow + directionRow;
                posCol = posCol;
                drawInvader();

                if (underFire[posCol]) {
                    this->despawn();
                }

                globalMillis = currentTime;
            }
        }
    }
} ;


void setup() {

}

void loop() {
    mainScreen.draw();
    spaceship.move(joystick.moveListener());
    spaceship.gun(joystick.shootListener());
}