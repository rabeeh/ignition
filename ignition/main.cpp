#include <QtGui/QApplication>
#include <QStyle>
#include <QDesktopWidget>
#include <QSplashScreen>
#include <QString>
#include <QDebug>
#include <QPainter>
#include <QFile>

#ifdef Q_WS_QWS
#include <QWSServer>
#endif

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#ifdef Q_WS_QWS
    QSplashScreen *splash = new QSplashScreen(QPixmap(":/SolidRun_Logo.png"));
    splash->show();
    QApplication::processEvents();
    QWSServer::setCursorVisible(true);
    MainWindow w;
    QWSServer::setBackground(QColor(0xe0, 0xe0, 0xe0));
    w.setGeometry(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter, w.size(), a.desktop()->availableGeometry()));
#else
    MainWindow w;
#endif
    a.setWindowIcon(QIcon(":/SR_icon.png"));
    w.show();
    return a.exec();
}
