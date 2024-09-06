#define TOP_CORNER_SHARP 1
#define BOTTOM_CORNER_SHARP 2
#define TOP_LEFT_SMOOTH 3
#define TOP_RIGHT_SMOOTH 4
#define BOTTOM_LEFT_SMOOTH 5
#define BOTTOM_RIGHT_SMOOTH 6
#define MID_BLOCK 7
#define FULL_BLOCK 0xff
#define DOT 8

uint8_t topCornerSharp[8] = {0, 0, 31, 31, 31, 31, 31, 31};
uint8_t bottomCornerSharp[8] = {31, 31, 31, 31, 31, 31, 0, 0};
uint8_t topLeftSmooth[8] = {0, 0, 3, 7, 15, 15, 31, 31};
uint8_t topRightSmooth[8] = {0, 0, 24, 28, 30, 30, 31, 31};
uint8_t bottomLeftSmooth[8] = {31, 31, 15, 15, 7, 3, 0, 0};
uint8_t bottomRightSmooth[8] = {31, 31, 30, 30, 28, 24, 0, 0};
uint8_t midBlock[8] = {0, 0, 31, 31, 31, 31, 0, 0};
uint8_t dot[8] = {0, 0, 0, 7, 7, 7, 0, 0};

void initLargeChars(LiquidCrystal_I2C lcd) {
    lcd.createChar(TOP_CORNER_SHARP, topCornerSharp);
    lcd.createChar(BOTTOM_CORNER_SHARP, bottomCornerSharp);
    lcd.createChar(TOP_LEFT_SMOOTH, topLeftSmooth);
    lcd.createChar(TOP_RIGHT_SMOOTH, topRightSmooth);
    lcd.createChar(BOTTOM_LEFT_SMOOTH, bottomLeftSmooth);
    lcd.createChar(BOTTOM_RIGHT_SMOOTH, bottomRightSmooth);
    lcd.createChar(MID_BLOCK, midBlock);
    lcd.createChar(DOT, dot);
}

void printLargeTime(LiquidCrystal_I2C lcd, uint8_t hours, uint8_t minutes, uint8_t ticker) {
    uint8_t hour1 = hours / 10;
    uint8_t hour2 = hours % 10;

    uint8_t min1 = minutes / 10;
    uint8_t min2 = minutes % 10;

    drawDigit(lcd, hour1, 0);
    drawDigit(lcd, hour2, 4);
    drawDigit(lcd, min1, 9);
    drawDigit(lcd, min2, 13);
    cleanColumn(lcd, 3);
    cleanColumn(lcd, 8);
    cleanColumn(lcd, 12);
    lcd.setCursor(7, 0);
    lcd.print(' ');
    lcd.setCursor(7, 1);
    if (ticker) {
        lcd.write(DOT);
    } else {
        lcd.print(' ');
    }
    lcd.setCursor(7, 2);
    if (ticker) {
        lcd.write(DOT);
    } else {
        lcd.print(' ');
    }
}

void printLargeFloat(LiquidCrystal_I2C lcd, float num) {
    uint8_t numInt = (uint8_t) num;
    uint8_t num1 = numInt / 10;
    uint8_t num2 = numInt % 10;
    uint8_t point = ((int) (num * 10)) % 10;

    drawDigit(lcd, num1, 0);
    drawDigit(lcd, num2, 4);
    drawDigit(lcd, point, 9);
    cleanColumn(lcd, 3);
    drawDot(lcd, 7);
    cleanColumn(lcd, 8);
    cleanRange(lcd, 12, 4);
}

void printLargeInt(LiquidCrystal_I2C lcd, int num) {
    uint8_t num1 = num / 1000;
    uint8_t num2 = (num / 100) % 10;
    uint8_t num3 = (num / 10) % 10;
    uint8_t num4 = num % 10;

    if (num1 != 0) {
        drawDigit(lcd, num1, 0);
        drawDigit(lcd, num2, 4);
        drawDigit(lcd, num3, 8);
        drawDigit(lcd, num4, 12);
        cleanColumn(lcd, 7);
        cleanColumn(lcd, 11);
        cleanColumn(lcd, 15);
    } else if (num2 != 0) {
        drawDigit(lcd, num2, 0);
        drawDigit(lcd, num3, 4);
        drawDigit(lcd, num4, 8);
        cleanColumn(lcd, 7);
        cleanRange(lcd, 11, 5);
    } else {
        drawDigit(lcd, num3, 0);
        drawDigit(lcd, num4, 4);
        cleanRange(lcd, 7, 9);
    }
    cleanColumn(lcd, 3);
    
}

void drawDigit(LiquidCrystal_I2C lcd, uint8_t digit, uint8_t offset) {
    bool draw;
    lcd.setCursor(offset, 0);
    if ((digit % 2 == 0 || digit % 3 == 0) && digit != 4) {
        drawOrClean(true, TOP_LEFT_SMOOTH);
    } else {
        drawOrClean(digit != 1, TOP_CORNER_SHARP);
    }

    lcd.setCursor(offset + 1, 0);
    draw = digit != 1 && digit != 4;
    drawOrClean(draw, MID_BLOCK);

    lcd.setCursor(offset + 2, 0);
    if (digit % 3 == 1) {
        drawOrClean(true, TOP_CORNER_SHARP);
    } else if (digit == 5) {
        drawOrClean(true, MID_BLOCK);
    } else {
        drawOrClean(true, TOP_RIGHT_SMOOTH);
    }

    lcd.setCursor(offset, 1);
    if (digit == 2) {
        drawOrClean(true, TOP_LEFT_SMOOTH);
    } else if (digit == 5) {
        drawOrClean(true, BOTTOM_CORNER_SHARP);
    } else if (digit % 2 == 0 && digit != 4) {
        drawOrClean(true, FULL_BLOCK);
    } else {
        drawOrClean(digit > 3 && digit != 7, BOTTOM_LEFT_SMOOTH);
    }
    
    lcd.setCursor(offset + 1, 1);
    drawOrClean(digit > 1 && digit != 7, MID_BLOCK);

    lcd.setCursor(offset + 2, 1);
    if (digit == 2) {
        drawOrClean(true, BOTTOM_RIGHT_SMOOTH);
    } else if (digit == 5 || digit == 6) {
        drawOrClean(true, TOP_RIGHT_SMOOTH);
    } else {
        drawOrClean(true, FULL_BLOCK);
    }

    lcd.setCursor(offset, 2);
    if (digit == 2) {
        drawOrClean(true, BOTTOM_CORNER_SHARP);
    } else {
        drawOrClean(digit % 3 != 1, BOTTOM_LEFT_SMOOTH);
    }

    lcd.setCursor(offset + 1, 2);
    drawOrClean(digit % 3 != 1, MID_BLOCK);

    lcd.setCursor(offset + 2, 2);
    if (digit == 2) {
        drawOrClean(true, MID_BLOCK);
    } else if (digit % 3 == 1) {
        drawOrClean(true, BOTTOM_CORNER_SHARP);
    } else {
        drawOrClean(true, BOTTOM_RIGHT_SMOOTH);
    }
}

void drawOrClean(bool draw, uint8_t c) {
    if (draw) {
        lcd.write(c);
    } else {
        lcd.print(' ');
    }
}

void drawDot(LiquidCrystal_I2C lcd, uint8_t offset) {
    lcd.setCursor(offset, 0);
    lcd.print(' ');
    lcd.setCursor(offset, 1);
    lcd.print(' ');
    lcd.setCursor(offset, 2);
    lcd.write(DOT);
}

void cleanColumn(LiquidCrystal_I2C lcd, uint8_t offset) {
    cleanRange(lcd, offset, 1);
}

void cleanRange(LiquidCrystal_I2C lcd, uint8_t offset, uint8_t columnsCount) {
    for (uint8_t c=0; c<columnsCount; c++) {
        for (uint8_t r=0; r<3; r++) {
            lcd.setCursor(offset + c, r);
            lcd.print(' ');
        }
    }
}
