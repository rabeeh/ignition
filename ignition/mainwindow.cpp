#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "ui_chooseSubtree.h"
#include "ui_progrss.h"
#include "ui_wifiSetup.h"
#include <unistd.h>
#include <QProcess>
#include <QtDebug>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QTextStream>
#include <QRegExp>
#include <QStringList>
#include <QCheckBox>
#include <QErrorMessage>
#include <QMessageBox>
#include <QProcess>
#include <QProcessEnvironment>
#include <QHBoxLayout>
#include <QApplication>
#include <QTime>
#include <QCloseEvent>

#define REPO_URL_FILE "/tmp/repo.url" /* A bypass to the built-in url that can be set by init scripts before running the installer */

#define GITHUB_REPO_URL "http://www.github.com/SolidRun/ignition-imx6/tarball/master"
#define DFLT_URL "SolidRun GitHub - http://www.github.com/SolidRun/ignition-imx6/"

#define ETH "eth0"
#define WLAN "wlan0"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    int iter;
    reload = true;
    ui->setupUi(this);
    repo = GITHUB_REPO_URL;
    repoNiceName = DFLT_URL;
    /* Setup the repository url */
    if (QFile::exists(REPO_URL_FILE))
    {
        QFile repoUrl (REPO_URL_FILE);
        repoUrl.open(QIODevice::ReadOnly | QIODevice::Text);
        repo = repoUrl.readLine();
        repoNiceName = repoUrl.readLine();
        if (repoNiceName.length() >= 5) /* Min 5 characters (if any) to display the nice name */
            ui->repoURL->setText(repoNiceName);
        else
            ui->repoURL->setText(repo);
    } else ui->repoURL->setText(DFLT_URL);
    custom[0] = ui->custom_0;
    custom[1] = ui->custom_1;
    custom[2] = ui->custom_2;
    custom[3] = ui->custom_3;
    ui->customArea->hide();
    for (iter = 0 ; iter < MAX_CUSTOM; iter ++) {
        custom[iter]->hide();
        custom[iter]->setChecked(false);
    }
    firstTime = true;
    timerId = startTimer(1000);

    /* Setup debug dialog */
    debug = new QDialog;
    debugUi.setupUi(debug);
    ui->debugButton->setCheckable(true);

#ifdef DEBUG
    ui->debugButton->setChecked(true);
    debug->show();
#endif
    QProcess process;
    process.start(QString("ifconfig ") + QString(WLAN));
    process.waitForFinished(-1);
    wifiExist = false;
    if (process.exitCode()) {
        ui->configWifi->hide(); /* Hide Wifi for now */
        ui->wlanIp->hide();
    } else {
        wifiExist = true;
        ui->configWifi->show(); /* Show Wifi option */
        ui->wlanIp->setText("Wireless - Disabled");

        /* Start wpa supplicant */
        QProcess wpa_process;
        wpa_process.start("wpa_supplicant -B -Dnl80211 -iwlan0 -c/etc/wpa_supplicant.conf");
        wpa_process.waitForFinished(-1);
        /* Enable wifi in connman */
        QProcess connman_process;
        connman_process.start("connmanctl enable wifi");
        connman_process.waitForFinished(-1);
        QProcess process;
        process.start("wpa_cli scan");
        process.waitForFinished(-1);

    }
}

MainWindow::~MainWindow()
{
    killTimer (timerId);
    delete ui;
}

void MainWindow::on_repoSync_clicked()
{
    QFile file;
    int idx, iter;
    ui->osList->clear();
    if (reload) {
        QProcess::execute("rm -rf /tmp/repo.tar.gz");
        /* Check if a file called /tmp/repo.url exists. This can be set by init scripts before running the installer */
        if (ui->repoURL->text() != repoNiceName) /* That's in the case where the user edited the URL */
            QProcess::execute("curl -k -L " + ui->repoURL->text() + " -o /tmp/repo.tar.gz");
        else
            QProcess::execute("curl -k -L " + repo + " -o /tmp/repo.tar.gz");
        QProcess::execute("rm -rf /tmp/repo");
        QProcess::execute("mkdir -p /tmp/repo/");
        QProcess::execute("tar zxf /tmp/repo.tar.gz --strip-components=1 -C /tmp/repo/");
    }
    QDir repoDir("/tmp/repo/");//,QDirIterator::Subdirectories);
    repoDir.setFilter(QDir::Dirs);
    QStringList entries = repoDir.entryList();
    reload = true;

    osIcon.clear();
    osDesc.clear();
    osName.clear();
    osDirName.clear();
    osInstall.clear();
    osList.clear();
    osRecommended.clear();
    for (iter = 0; iter < MAX_CUSTOM; iter ++)
        osCustomize[iter].clear();
    idx = 0;
    for (QStringList::ConstIterator entry=entries.begin(); entry!=entries.end(); ++entry) {
        QString dirname="/tmp/repo/"+*entry;
        if ((*entry == tr(".")) || (*entry == tr(".."))) continue;

        if (ui->showAll->isChecked() == false) /* Show only recommended */
            if (QFile::exists(dirname+"/RECOMMENDED") == false) continue;
        /* Save the directory name */
        osDirName.insert(idx, dirname);
        /* Save the icon */
        if (QFile::exists(dirname+"/icon.png"))
            osIcon.insert (idx, QIcon(dirname+"/icon.png"));
        /* Save the name to be shown */
        file.setFileName(dirname+"/NAME"); file.open(QIODevice::ReadOnly | QIODevice::Text); osName.insert (idx, QTextStream (&file).readLine()); file.close();
        if (QFile::exists(dirname+"/RECOMMENDED")) {
            osName[idx] += " [ RECOMMENDED ]";
            osRecommended[idx] = true;
        } else osRecommended[idx] = false;
        /* Save the description */
        file.setFileName(dirname+"/DESCRIPTION"); file.open(QIODevice::ReadOnly | QIODevice::Text); osDesc.insert (idx, QTextStream (&file).readLine()); file.close();
        /* Iterate over different custoimzation files */
        for (iter = 0 ; iter < MAX_CUSTOM ; iter++) {
            QString custom;
            file.setFileName(dirname+"/CUSTOM"+QString::number(iter)); file.open(QIODevice::ReadOnly | QIODevice::Text); osCustomize[iter].insert(idx, QTextStream (&file).readLine()); file.close();
        }
        /* Save the installation script directory. This is a mandatory file */
        osInstall.insert (idx, dirname+"/install.sh");
        /* Save the pre installation user defined listing script */
        osList.insert (idx, dirname+"/list.sh");
        idx ++;
    }
    for (iter = 0 ; iter < idx ; iter ++) {
        QListWidgetItem *item;
        if (osIcon[iter].isNull()) {
            QPixmap dummy = QPixmap(ICON_X, ICON_Y);
            dummy.fill();
            item = new QListWidgetItem(dummy, osName[iter]+"\n"+osDesc[iter]);
            item->setIcon(dummy);
        }
        else
            item = new QListWidgetItem(osIcon[iter], osName[iter]+"\n"+osDesc[iter]);
        QColor back;
        if (iter %2) back.setRgb(200,200,200);
        else back.setRgb(255,255,255);
        item->setBackground(back);
        item->setSizeHint(QSize(item->sizeHint().width(),ICON_Y));
        ui->osList->addItem(item);

    }
    ui->osList->setIconSize(QSize(ICON_X,ICON_Y));
}

void MainWindow::on_osList_currentRowChanged(int currentRow)
{
    int iter;
    ui->customArea->hide();
    for (iter = 0 ; iter < MAX_CUSTOM; iter++) {
        if (osCustomize[iter][currentRow] != "") {
            QRegExp rx("[,]");
            QStringList list = osCustomize[iter][currentRow].split(rx);
            if (list.at(0).toLower() == "true") custom[iter]->setChecked(true);
            else custom[iter]->setChecked(false);
            custom[iter]->setText(list.at(2));
            custom[iter]->show();
            customEnv[iter] = list.at(1);
            ui->customArea->show();
        }
        else {
            custom[iter]->hide();
            custom[iter]->setChecked(false);
        }
    }
}

void MainWindow::closeEvent (QCloseEvent *event)
{
    if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Continue?", "Continuing will reboot.",
                                        QMessageBox::Yes|QMessageBox::No).exec()) {
        QProcess::execute("sync");
        QProcess::execute("reboot");
    }
    event->ignore();
}

/* Start the installation process */
void MainWindow::on_install_clicked()
{
    QFile file;
    int status;
    int iter;
    int choice = ui->osList->currentRow();
    /* Check if this is the micro SD that has ignition that will be over-written */
    QProcess::execute("mount /dev/mmcblk0p1 /mnt -o ro");
    bool ignition_sd = false;
    if (QFile::exists("/mnt/ignition.sig"))
        ignition_sd = true;
    QProcess::execute("umount /mnt");
    if (ignition_sd) {
        while (1) {
            if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Continue?", "Continuing will overwrite Ignition.\nYou can replace the micro SD with another one and then click 'yes'.\nOr you can also click 'yes' but Ignition will be overwritten",
                                                QMessageBox::Yes|QMessageBox::No).exec()) {
                if (QFile::exists("/dev/mmcblk0")) break;
            }
            else return;
        }
    }

    qDebug() << "Running " + osInstall[choice];
    if (osName[choice] == "xterm") {
        QProcess *process = new QProcess(this);
        process->start(osInstall[choice]);
        return;
    }
    status = QProcess::execute("chmod +x "+ osInstall[choice]);
    if (status != 0 ) {
        QErrorMessage msgBox;
        msgBox.showMessage("File "+ osInstall[choice]+" not found. Aborting install");
        msgBox.exec();
    }

    /* First check if a /tmp/list.out can be generated from the destination OS */
    QProcess process;
    /* Prepare environment */
    QProcessEnvironment processEnv = QProcessEnvironment::systemEnvironment();
    for (iter = 0 ; iter < MAX_CUSTOM; iter ++)
    {
        if (custom[iter]->checkState()) {
            processEnv.insert(customEnv[iter],"true");
        }
    }
    /* Try chmod +x; if success means the file is there */
    status = QProcess::execute("chmod +x "+ osList[choice]);
    if (status == 0 ) { /* There is really a file. Check the output /tmp/list.out */
        process.setProcessEnvironment(processEnv);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(osList[choice]);
        qDebug() << "Process started";
        while (process.waitForReadyRead()) {
            debugUi.debugText->append(process.readAll());
            debugUi.debugText->repaint();

        }
        process.waitForFinished(-1); /* Block UI until installation is done */
        file.setFileName("/tmp/list.out");
        file.open(QIODevice::ReadOnly | QIODevice::Text);

        QDialog *dialog = new QDialog;
        Ui::subtreeDialog subtreeUi;
        subtreeUi.setupUi(dialog);
        dialog->show();
        qDebug() << "Adding more lines now";
        while (!file.atEnd()) {
            QString line = file.readLine();
            qDebug() << line;
            subtreeUi.subtreeList->addItem(line);
        }
        file.close();
        subtreeUi.subtreeList->setCurrentRow(0);
        int ret = dialog->exec();
        if (ret == QDialog::Rejected) return; /* User clicked cancel */
        QString line = subtreeUi.subtreeList->currentItem()->text();
        qDebug() << "User choice is :";
        qDebug() << line;
        processEnv.insert("CHOICE", line);

    }
    QDialog *progress = new QDialog;
    Ui::progressDialog progressUi;
    progressUi.setupUi(progress);
    progress->setWindowTitle("Downloading...");
    progress->show();
    progressUi.progress->setValue(0);
    progressUi.progress->repaint();

    QProcess processInstall;
    processInstall.setProcessEnvironment(processEnv);
    processInstall.setProcessChannelMode(QProcess::MergedChannels);
    processInstall.start(osInstall[choice]);
    ui->install->setEnabled(false);
    elapsedTime.restart();
    elapsedTime.start();

    QString forNextTime="";
    int elapsed;
    while (1) {
        bool quit = false;
        int i;
        float x = 0;
        QString tempLine;
        QStringList pieces;
        if (processInstall.state() == QProcess::NotRunning) quit=true;
        usleep(100*1000);
        tempLine = processInstall.readLine();
        tempLine.prepend(forNextTime);
        forNextTime="";
        if (tempLine != "") {
        /* This adds to the general debug. Notice that tempLine might get partial */
           debugUi.debugText->append(tempLine);
           debugUi.debugText->repaint();

            pieces = tempLine.split(0x0d);
            for (i = 0 ; i < pieces.count() - ((quit == true)?0:1); i++)
            {
                QStringList tmp1,tmp2;
                tmp1 = pieces.value(i).split("%");
                /* qDebug() << "SPLITS - " + pieces.value(i); */
                if (tmp1.count() >= 2) {
                    tmp2 = tmp1.value(tmp1.count()-2).split(" ");
                    if (tmp2.count()) {
                        /* If the conversion fails then x is set to 0 */
                        x = tmp2.value(tmp2.count()-1).toFloat();
                    }
                }
            }
            if (x) {
                if (x == 100) progress->setWindowTitle("Installing...");
                progressUi.progress->setValue(x);
                progressUi.progress->repaint();
            }
        }
        forNextTime+=pieces.value(pieces.count()-1);
        elapsed = elapsedTime.elapsed()/1000;
        QString str;
        str.sprintf("%02d:%02d:%02d",elapsed/3600,elapsed/60,elapsed%60);
        progressUi.time->setText(str);            QCoreApplication::processEvents();
        QCoreApplication::processEvents();
        if (quit == true) break;
    }

    processInstall.waitForFinished(-1); /* Block UI until installation is done */
    delete progress;
    ui->install->setEnabled(true);
    elapsedTime.restart();
    qDebug() << "Exit code is " + processInstall.exitCode();
    printf ("exit code %d\n",processInstall.exitCode());
    if (processInstall.exitCode()) {
        /* Installation might have failed */
        QErrorMessage msgBox;
        msgBox.showMessage("Installation failed. Retry.");
        msgBox.exec();
    } else {
        if (QMessageBox::Yes == QMessageBox(QMessageBox::Information, "Restart?", "Installation done. Restart?", QMessageBox::Yes|QMessageBox::No).exec())
        {
            QProcess::execute("sync");
            QProcess::execute("reboot");
            while (1) {} /* Should never reach here */
            {
                qDebug() << "But we do";
            }
        }
    }
}

void MainWindow::timerEvent(QTimerEvent *event)
{
    /* Check if Ethernet has IP address */
    bool internet = false;
    QProcess process;
    QCoreApplication::processEvents();
    process.start(QString("ifconfig ") + QString(ETH));
    process.waitForFinished(-1);
    QString stdout = process.readAllStandardOutput();
    QRegExp rx("[:]");
    QStringList list = stdout.split(rx);
    if (list.count() == 33) { /* using buildroot 2014.08 busybox ifconfig signature */
        QRegExp rxx("[ ]");
        QStringList list2 = (list.at(7)).split(rxx);
        ui->ethIp->setText("Wired - IP "+QString(list2.at(0)));
        internet = true;
    } else {
        ui->ethIp->setText("Wired - Requestion IP...");
    }
    if (wifiExist) {
        /* Now check if wifi has IP address */
        process.start(QString("ifconfig ") + QString(WLAN));
        process.waitForFinished(-1);
        QString stdout = process.readAllStandardOutput();
        QRegExp rx("[:]");
        QStringList list = stdout.split(rx);
        if (list.count() == 33) { /* using buildroot 2014.08 busybox ifconfig signature */
            QRegExp rxx("[ ]");
            QStringList list2 = (list.at(7)).split(rxx);
            ui->wlanIp->setText("Wireless - IP "+QString(list2.at(0)));
            internet = true;
        } else {
            ui->wlanIp->setText("Wireless - No IP");
        }
    }

    if (internet && firstTime) {
        QCoreApplication::processEvents();
        this->on_repoSync_clicked();
        firstTime = false;
    }
}

void MainWindow::on_debugButton_clicked()
{
    if (ui->debugButton->isChecked())
        debug->show();
    else
        debug->hide();
}

void MainWindow::on_showAll_toggled(bool checked)
{
    reload = false;
    on_repoSync_clicked();
}

void MainWindow::updateWifiSSDIList()
{
    QProcess process;
    int i=0, iter=0;
    process.start("wpa_cli scan");
    process.waitForFinished(-1);
    process.start("wpa_cli scan_results");
    process.waitForFinished(-1);
    QString stdout = process.readAllStandardOutput();
//    qDebug() << stdout;

//    QFile file ("/tmp/result.txt");
//    file.open(QIODevice::ReadOnly | QIODevice::Text);
//    QTextStream in(&file);
    QTextStream in(&stdout);
    wifiSetup.wifiList->clear();
    while(!in.atEnd())
    {
        QString line = in.readLine();
        i++;
        iter ++;
        if (i <= 2) continue;
        qDebug() << line;
        QRegExp rx("(\\t)");
        QStringList list = line.split(rx);
        qDebug() << list.count();
        if (list.count() == 5 ) {
            QListWidgetItem *item;
            item = new QListWidgetItem(list[4]);
            QColor back;
            if (iter %2) back.setRgb(200,200,200);
            else back.setRgb(255,255,255);
            item->setBackground(back);
//            item->setSizeHint(QSize(item->sizeHint().width(),ICON_Y));
            wifiSetup.wifiList->addItem(item);
        }
    }
}

void MainWindow::on_configWifi_clicked()
{
    wifiDialog = new QDialog;
    wifiSetup.setupUi(wifiDialog);
    wifiDialog->show();
    wifiDialog->repaint();
    connect(wifiSetup.wifiRefresh, SIGNAL(clicked()), this, SLOT(on_wifiRefresh_clicked()));
    connect(wifiSetup.wifiCancel, SIGNAL(clicked()), this, SLOT(on_wifiCancel_clicked()));
    connect(wifiSetup.wifiConnect, SIGNAL(clicked()), this, SLOT(on_wifiConnect_clicked()));
    updateWifiSSDIList();

}

void MainWindow::on_wifiRefresh_clicked()
{
    qDebug() << "wifi refresh clicked";
    updateWifiSSDIList();
}

void MainWindow::on_wifiCancel_clicked()
{
    delete wifiDialog;
}

void MainWindow::on_wifiConnect_clicked()
{
    QFile file("/tmp/connect_wifi.sh");
    file.open(QIODevice::WriteOnly | QIODevice::Text);
    QTextStream out(&file);
    out << "#!/bin/bash\n";
    out << "wpa_cli select_network 0\n";
    out << "wpa_cli remove_network 1\n";
    out << "wpa_cli add_network\n";
    out << "wpa_cli set_network 1 ssid '\""+ wifiSetup.wifiList->currentItem()->text()+"\"'\n";
    if (wifiSetup.wifiPassword->text() != "Password...")
        out << "wpa_cli set_network 1 psk '\""+ wifiSetup.wifiPassword->text()+"\"'\n";
    out << "wpa_cli select_network 1\n";
    out << "sleep 1\n";
    out << "udhcpc -i wlan0 -b\n";
    file.close();
    QProcess::execute("chmod +x /tmp/connect_wifi.sh");
    QProcess::execute("/tmp/connect_wifi.sh");
    /* Now exit the menu */
    QCoreApplication::processEvents();
    delete wifiDialog;
}
