#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>

#include <QStringListModel>

#include <endpointwidget.h>
#include <datatransmitter.h>

#define EW_MAX_COLS 4

MainWindow::MainWindow(Client* client, QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //save a reference to Client-Object
    this->client = client;
    this->dataTransmitter = DataTransmitter::getInstance();
    this->dataTransmitter->setClient(client);


    //UI connections
    connect(ui->buConnect, SIGNAL(clicked(bool)), this, SLOT(slotConnect(bool)));   
    connect(ui->actionQuit, SIGNAL(triggered(bool)), this, SLOT(slotQuit()));
    connect(ui->actionResetServer, SIGNAL(triggered(bool)), this, SLOT(slotResetServer()));
    connect(ui->actionResetUi, SIGNAL(triggered(bool)), this, SLOT(slotResetUI()));

    ui->lePort->setValidator(new QIntValidator(1, 65535, this));

    this->ui->horizontalLayoutEndpointWidgets_lower->setGeometry(ui->hBoxLowerFrame->geometry());
    this->ui->horizontalLayoutEndpointWidgets_upper->setGeometry(ui->hBoxUpperFrane->geometry());
    qDebug()<<ui->horizontalLayoutEndpointWidgets_upper->geometry();
    qDebug()<<ui->hBoxLowerFrame->geometry();
    qDebug()<<ui->horizontalLayoutEndpointWidgets_lower->geometry();
    //restore previous window settings

    QSettings settings(QDir::currentPath() + "/settings.ini",  QSettings::IniFormat);
    //this->settings.setPath(QSettings::IniFormat, QSettings::UserScope, QApplication::applicationDirPath() );
    settings.beginGroup("MainWindow");
    resize(settings.value("size", QSize(400, 400)).toSize());
    move(settings.value("pos", QPoint(200, 200)).toPoint());
    settings.endGroup();

}

void MainWindow::slotConnect(bool) {
    QString hostString = ui->leHost->text();
    QHostAddress host;
    if(hostString!= "") {
        if(this->client->isConnected()) {
            emit signalDisconnect();
        } else {
            if(hostString == "localhost") {
                host = QHostAddress::LocalHost;
            } else {
                host = QHostAddress(ui->leHost->text());
            }
            quint16 port = ui->lePort->text().toInt();
            emit signalConnect(host, port);
        }
    }else {
        QMessageBox msgBox;
        msgBox.setText("Fehler: Bitte gültigen Hostnamen oder IP eingeben.");
        msgBox.setStandardButtons(QMessageBox::Ok);
        msgBox.exec();
    }
}

void MainWindow::slotConnected() {
    this->ui->buConnect->setText("trennen");
    this->ui->buConnect->setStyleSheet("background-color:green");
    this->dataTransmitter->sendIdent();
    clearEndpointsGrid();
}

void MainWindow::slotDisconnected() {
    this->ui->buConnect->setText("verbinden");
    this->ui->buConnect->setStyleSheet("");
}

//void MainWindow::slotReceivedData(QString message) {
void MainWindow::slotReceivedEndpointList(QList<Endpoint *> endpointsUpdate) {
    qDebug()<<__FUNCTION__;
    this->endpoints = endpointsUpdate;
    updateTable(endpointsUpdate);
}

void MainWindow::parseBasicEndpointInfo(QString message) {
}

void MainWindow::addEndpoint(QString alias, QString type, QString MAC) {
    Endpoint* newEndpoint = new Endpoint(NULL, alias, type, MAC);
    this->endpoints.append(newEndpoint);
}

void MainWindow::updateTable(QList<Endpoint *> endpointsUpdate) {
    //update existing
    foreach(EndpointWidget* ew, this->endpointWidgets) {
        bool endpointPresent = false;
        foreach(Endpoint* endpoint, endpointsUpdate) {
            if (endpoint->getMAC() == ew->getMac()) {
                //one of the endpoints from the list has the same MAC
                endpointPresent = true;
            }
        }
        if (!endpointPresent){
            //delete this endpoint Widget
            ew->hide();
            this->ui->horizontalLayoutEndpointWidgets_upper->removeWidget(ew);
            this->ui->horizontalLayoutEndpointWidgets_lower->removeWidget(ew);
            this->endpointWidgets.removeOne(ew);
            this->mapMacToEndpointWidget.remove(ew->getMac());
        }
    }


    foreach(Endpoint* endpoint, endpointsUpdate) {
        if(this->mapMacToEndpointWidget.contains(endpoint->getMAC())) {
            //a widget for this endpoint already exists-->get Widget
            EndpointWidget* ew = this->mapMacToEndpointWidget.value(endpoint->getMAC());
            //update its content
            ew->setEndpoint(endpoint);
            ew->updateWidget();
            ew->repaint();
            this->ui->horizontalLayoutEndpointWidgets_upper->update();
        } else {
            //new endpoint; create a new Widget and add
            EndpointWidget* ew = new EndpointWidget(endpoint);
            //this->ui->grid_endpointWidgets->addWidget(ew);
            this->endpointWidgets.append(ew);
            this->mapMacToEndpointWidget.insert(endpoint->getMAC(), ew);
            connect(ew, SIGNAL(signalRequestStateChange(QString,bool)),
                    this, SLOT(slotRequestStateChange(QString,bool)));
            int row=0;
            if (this->endpointWidgets.length()>4) {
                row=1;
            }
            if(row==0) {
                int col = this->endpointWidgets.length()-1;
                this->ui->horizontalLayoutEndpointWidgets_upper->insertWidget(col, ew);
            }else if(row==1){
                int col = this->endpointWidgets.length() -EW_MAX_COLS - 1;
                this->ui->horizontalLayoutEndpointWidgets_lower->insertWidget(col, ew);
            }
        }
    }
}

void MainWindow::clearEndpointsGrid() {
    //clean up first
    QList<QHBoxLayout*> hBoxLayouts;
    hBoxLayouts.append(this->ui->horizontalLayoutEndpointWidgets_upper);
    hBoxLayouts.append(this->ui->horizontalLayoutEndpointWidgets_lower);
    QLayoutItem* item;
    int colCount = ui->horizontalLayoutEndpointWidgets_upper->count();
    for(int row=0; row<2; row++) {
        colCount = hBoxLayouts.at(row)->count();
        for(int col=0; col<colCount; col++) {
            item = hBoxLayouts.at(row)->itemAt(hBoxLayouts.at(row)->count()-1);
            if(item) {
                //If item exists...
                EndpointWidget* ew = (EndpointWidget*)item->widget();
                ew->hide();
                hBoxLayouts.at(row)->removeWidget(ew);
                this->endpointWidgets.removeOne(ew);
            }
        }
    }
    this->mapMacToEndpointWidget.clear();
}
void MainWindow::slotRequestStateChange(QString MAC, bool state) {
    this->dataTransmitter->sendStateRequestDigital(MAC, state);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    QSettings settings(QDir::currentPath() + "/settings.ini",  QSettings::IniFormat);
    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    settings.endGroup();
}

void MainWindow::slotQuit() {
    qDebug()<<__FUNCTION__;
    //save current window settings
    emit signalQuit();
    this->close();
}

void MainWindow::slotResetServer() {
    this->dataTransmitter->sendServerResetRequest();
}

void MainWindow::slotResetUI() {
    clearEndpointsGrid();
}

MainWindow::~MainWindow()
{
    delete ui;
}