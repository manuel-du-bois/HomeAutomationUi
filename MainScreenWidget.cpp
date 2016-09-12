#include "MainScreenWidget.h"
#include "ui_MainScreenWidget.h"
#include <QDebug>
#include <EndpointOverviewScreen.h>
#include <StartScreenController.h>
#include <client.h>
#include <StartScreenController.h>

MainScreenWidget* MainScreenWidget::_instance = NULL;

MainScreenWidget::MainScreenWidget(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainScreenWidget),
    currentScreen(NULL)
{
    ui->setupUi(this);
    setFixedSize(800,600);
    qmlWidget = this->ui->quickWidget;
    qmlWidget->rootContext()->setContextProperty("mainContentSource", "");
    qmlWidget->setSource(QUrl(QStringLiteral("qrc:/MainQmlScreen.qml")));
    rootObject = qmlWidget->rootObject();
    connect((QObject*)qmlWidget->rootObject(), SIGNAL(signalQmlLoaded()), this, SLOT(slotQmlLoaded()));
    prepareScreens();
}

MainScreenWidget::~MainScreenWidget()
{
    delete ui;
}

bool MainScreenWidget::navigate(QString screenName)
{
    QList<QString> liste = mapScreenNamesToScreenController.keys();
    if (liste.contains(screenName)) {
        AbstractScreenController* nextScreen = mapScreenNamesToScreenController.value(screenName);
        if(nextScreen != NULL && nextScreen != this->currentScreen) {
            nextScreen->setQmlContext(qmlWidget->rootContext());
            this->currentScreen = nextScreen;
            setMainContentQmlPath(nextScreen->getQmlPath());
            return true;
        }
    }
    return false;
}

AbstractScreenController* MainScreenWidget::getControllerInstance(QString screenName)
{
    QList<QString> liste = mapScreenNamesToScreenController.keys();
    if (liste.contains(screenName)) {
        return mapScreenNamesToScreenController.value(screenName);
    }
    else {
        return NULL;
    }
}

MainScreenWidget *MainScreenWidget::getInstance()
{
    if(_instance == NULL) {
        _instance = new MainScreenWidget();
    }
    return _instance;
}

void MainScreenWidget::slotQmlLoaded()
{
    QString screenName = this->currentScreen->getScreenName();
    QQuickItem* loadedContentRootObject;
    loadedContentRootObject = rootObject->findChild<QQuickItem*>(screenName);
    if(loadedContentRootObject != NULL) {
        this->currentScreen->setQmlRootObject(loadedContentRootObject);
    }
    if (screenName == "StartScreen") {
        connect((StartScreenController*)this->currentScreen, SIGNAL(signalConnect(QHostAddress,quint16)), Client::getInstance(), SLOT(slotConnectToHost(QHostAddress,quint16)));
    }

}

void MainScreenWidget::prepareScreens()
{
    //create instances or get pointer to them and save it inside map
    mapScreenNamesToScreenController.insert("EndpointOverviewScreen", new EndpointOverviewScreen());
    mapScreenNamesToScreenController.insert("StartScreen", new StartScreenController());

}

void MainScreenWidget::setMainContentQmlPath(QString path)
{
  qmlWidget->rootContext()->setContextProperty("mainContentSource", path);
}

