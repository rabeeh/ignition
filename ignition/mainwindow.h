#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QTextStream>
#include <QIcon>
#include <QCheckBox>
#include <QTimer>
#include <QTime>
#include "ui_chooseSubtree.h"
#include "ui_debug.h"
#include "ui_wifiSetup.h"

#define MAX_CUSTOM 4
#define ICON_X  50
#define ICON_Y  50

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    QMap < int, QString > osDirName;
    QMap < int, QIcon > osIcon;
    QMap < int, QString > osName;
    QMap < int, QString > osDesc;
    QMap < int, QString > osCustomize[MAX_CUSTOM];
    QMap < int, QString > osInstall;
    QMap < int, QString > osList;
    QMap < int, bool > osRecommended;
    QString subtreeLine;
    bool subtreeOK;
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    QTime elapsedTime;
    QString repo, repoNiceName;
    bool reload;
    bool wifiExist;
    QDialog *wifiDialog;
    
private slots:
    void on_repoSync_clicked();

    void on_osList_currentRowChanged(int currentRow);

    void on_install_clicked();

    void on_debugButton_clicked();

    void on_showAll_toggled(bool checked);

    void on_configWifi_clicked();

    void updateWifiSSDIList();

    void on_wifiRefresh_clicked();

    void on_wifiCancel_clicked();

    void on_wifiConnect_clicked();

private:
    Ui::MainWindow *ui;
    int timerId;
    QCheckBox* custom[MAX_CUSTOM];
    QString customEnv[MAX_CUSTOM];
    bool firstTime;
    QDialog *debug;
    Ui::debug debugUi;
    Ui::wifiSetup wifiSetup;

protected:
    void timerEvent(QTimerEvent *event);
    void closeEvent (QCloseEvent *event);
};

#endif // MAINWINDOW_H
