#ifndef SCREEN_H
#define SCREEN_H

#include <QGuiApplication>
#include <QScreen>
#include <QApplication>
#include <QDebug>

class Screen
{
public:
    Screen();
    static int width;
    static int height;
    static void init();
};

#endif // SCREEN_H
