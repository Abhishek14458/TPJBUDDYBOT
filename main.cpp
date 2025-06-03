#include <SPI.h>
#include <XPT2046_Touchscreen.h> 
// Reference: XPT2046_Touchscreen Library by Paul Stoffregen
// GitHub Repository: https://github.com/PaulStoffregen/XPT2046_Touchscreen.git
// Version used: v1.4
// This library is used to interface with XPT2046-based resistive touchscreens

#include <TFT_eSPI.h>
#include <WiFi.h>
#include <WebServer.h>
// ======  (generated with help of ChatGPT) ======
// ========== Wi-Fi Configuration ==========
const char* ssid = "BuddyBot";         
const char* password = "12345678"; 

// ========== Touchscreen Pins ==========
#define XPT2046_IRQ 36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK 25
#define XPT2046_CS 33

SPIClass mySpi = SPIClass(VSPI);
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

WebServer server(80); // Web server on port 80

// ========== Game State ==========
enum GameState { WELCOME, MENU, QUIZ, GAME_OVER };
GameState currentState = WELCOME;

enum QuizCategory { MATH, SCIENCE };
QuizCategory selectedCategory;

struct Question {
    String question;
    String option1;
    String option2;
    int correctAnswer;  // 1 = Option1, 2 = Option2
};

// ========== Question Banks ==========
Question mathQuestions[10] = {
    {"What is 1 + 1?", "2", "3", 1},
    {"What is 2 + 3?", "5", "4", 1},
    {"What is 4 x 2?", "8", "6", 1},
    {"What is 10 - 5?", "5", "4", 1},
    {"What comes after 7?", "8", "9", 1},
    {"What is 6 + 4?", "10", "9", 1},
    {"How many sides does a square have?", "4", "3", 1},
    {"What is 3 x 3?", "9", "6", 1},
    {"How many fingers are in one hand?", "5", "4", 1},
    {"What is 20 / 4?", "5", "6", 1}
};

Question scienceQuestions[10] = {
    {"What do plants need to grow?", "Water", "Oil", 1},
    {"Which planet is closest to the sun?", "Mercury", "Venus", 1},
    {"What gas do humans breathe?", "Oxygen", "Carbon", 1},
    {"What is the color of grass?", "Green", "Blue", 1},
    {"Which animal can fly?", "Bird", "Dog", 1},
    {"What do we drink to stay hydrated?", "Water", "Juice", 1},
    {"What do bees make?", "Honey", "Milk", 1},
    {"What is the tallest animal?", "Giraffe", "Elephant", 1},
    {"What do fish use to breathe?", "Gills", "Lungs", 1},
    {"What is Earth's natural satellite?", "Moon", "Sun", 1}
};

Question currentQuestions[5];
int currentQuestionIndex = 0;
int score = 0;

// ========== Touch Mapping ==========
int mapX(int rawX) { return map(rawX, 200, 3700, 0, tft.width()); }
int mapY(int rawY) { return map(rawY, 200, 3700, 0, tft.height()); }

// ========== Quiz Logic ==========
void selectRandomQuestions() {
    Question* category = (selectedCategory == MATH) ? mathQuestions : scienceQuestions;
    for (int i = 0; i < 5; i++) {
        currentQuestions[i] = category[random(0, 10)];
    }
}

// ========== TFT Screens ==========
void drawWelcomeScreen() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(3);
    tft.setCursor(80, 80);
    tft.println("BuddyBot");
    tft.setTextSize(2);
    tft.setCursor(60, 160);
    tft.println("Touch to Start");
}

void drawMenu() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(60, 30);
    tft.println("Choose a Category");

    tft.fillRoundRect(50, 100, 220, 50, 10, TFT_BLUE);
    tft.setCursor(90, 120);
    tft.println("Math Quiz");

    tft.fillRoundRect(50, 180, 220, 50, 10, TFT_GREEN);
    tft.setCursor(80, 200);
    tft.println("Science Quiz");
}

void drawQuestion() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(40, 30);
    tft.println(currentQuestions[currentQuestionIndex].question);

    tft.fillRoundRect(50, 100, 220, 50, 10, TFT_BLUE);
    tft.setCursor(90, 120);
    tft.println(currentQuestions[currentQuestionIndex].option1);

    tft.fillRoundRect(50, 180, 220, 50, 10, TFT_GREEN);
    tft.setCursor(90, 200);
    tft.println(currentQuestions[currentQuestionIndex].option2);
}

void drawGameOver() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.setTextSize(2);
    tft.setCursor(60, 60);
    tft.println("Game Over!");
    tft.setCursor(60, 100);
    tft.printf("Score: %d/5", score);

    tft.fillRoundRect(50, 180, 220, 50, 10, TFT_YELLOW);
    tft.setCursor(90, 200);
    tft.println("Restart");
}

// ========== Touch Handlers ==========
void handleWelcomeTouch(int x, int y) {
    currentState = MENU;
    drawMenu();
}

void handleMenuTouch(int x, int y) {
    if (x >= 50 && x <= 270) {
        if (y >= 100 && y <= 150) {
            selectedCategory = MATH;
        }
        else if (y >= 180 && y <= 230) {
            selectedCategory = SCIENCE;
        }
        currentState = QUIZ;
        selectRandomQuestions();
        drawQuestion();
    }
}

void handleQuizTouch(int x, int y) {
    if (y >= 100 && y <= 150) {
        if (currentQuestions[currentQuestionIndex].correctAnswer == 1) score++;
    }
    else if (y >= 180 && y <= 230) {
        if (currentQuestions[currentQuestionIndex].correctAnswer == 2) score++;
    }

    currentQuestionIndex++;
    if (currentQuestionIndex < 5) drawQuestion();
    else { currentState = GAME_OVER; drawGameOver(); }
}

void handleGameOverTouch(int x, int y) {
    if (y >= 180 && y <= 230) {
        currentQuestionIndex = 0;
        score = 0;
        currentState = WELCOME;
        drawWelcomeScreen();
    }
}

// ====== Web Interface (HTML generated with help of ChatGPT) ======
// ========== Web Server Handler ==========
String getCategoryName() {
    return (selectedCategory == MATH) ? "Math Quiz" : "Science Quiz";
}

void handleRoot() {
    String html = "<!DOCTYPE html><html><head><title>BuddyBot Score</title></head><body>";
    html += "<h2>BuddyBot - Quiz Results</h2>";
    html += "<p><strong>Category:</strong> " + getCategoryName() + "</p>";
    html += "<p><strong>Score:</strong> " + String(score) + "/5</p>";
    html += "</body></html>";
    server.send(200, "text/html", html);
}

// ========== Setup ==========
void setup() {
    Serial.begin(115200);
    mySpi.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
    ts.begin(mySpi);
    ts.setRotation(1);
    tft.init();
    tft.setRotation(1);
    drawWelcomeScreen();

    // WiFi Setup
    WiFi.begin(ssid, password);
    Serial.print("Connecting to WiFi");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected");
    Serial.print("IP Address: ");
    Serial.println(WiFi.localIP());

    // Web Server Setup
    server.on("/", handleRoot);
    server.begin();
    Serial.println("Web server started");
}

// ========== Main Loop ==========
void loop() {
    server.handleClient();

    if (ts.tirqTouched() && ts.touched()) {
        TS_Point p = ts.getPoint();
        int x = mapX(p.x);
        int y = mapY(p.y);

        if (currentState == WELCOME) handleWelcomeTouch(x, y);
        else if (currentState == MENU) handleMenuTouch(x, y);
        else if (currentState == QUIZ) handleQuizTouch(x, y);
        else if (currentState == GAME_OVER) handleGameOverTouch(x, y);

        delay(300);
    }
}
