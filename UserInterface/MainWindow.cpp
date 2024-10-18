#include "MainWindow.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QNetworkRequest>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Setup UI
    auto *layout = new QVBoxLayout;
    resize(800, 600);

    QPushButton *btnStartServer = new QPushButton("Neuen Server starten", this);
    QPushButton *btnServerList = new QPushButton("Serverliste neu laden", this);
    tableWidget = new QTableWidget(this);
    tableWidget->setColumnCount(5);
    QStringList headers = {"", "ServerName", "IpAdresse", "Port", "Spieleranzahl"};
    tableWidget->setHorizontalHeaderLabels(headers);

    layout->addWidget(btnStartServer);
    layout->addWidget(btnServerList);
    layout->addWidget(tableWidget);

    QWidget *container = new QWidget;
    container->setLayout(layout);
    setCentralWidget(container);

    // Connect buttons
    connect(btnServerList, &QPushButton::clicked, this, &MainWindow::fetchServerList);

    // Network Manager
    networkManager = new QNetworkAccessManager(this);

    fetchServerList();
}

void MainWindow::fetchServerList() {
    QNetworkRequest request(QUrl("http://127.0.0.1:8080/serverApi/getServers"));
    networkManager->get(request);
    connect(networkManager, &QNetworkAccessManager::finished, this, &MainWindow::onServerListReceived);
}

void MainWindow::onServerListReceived(QNetworkReply* reply) {
    if(reply->error()) {
        qDebug() << "Error in network reply:" << reply->errorString();
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
    QJsonArray serverList = doc.array();

    tableWidget->setRowCount(serverList.size());

    for (int i = 0; i < serverList.size(); ++i) {
        // Add a dummy join button
        QPushButton *btnJoin = new QPushButton("Join");
        tableWidget->setCellWidget(i, 0, btnJoin);

        QJsonObject server = serverList[i].toObject();
        tableWidget->setItem(i, 1, new QTableWidgetItem(server["name"].toString()));
        tableWidget->setItem(i, 2, new QTableWidgetItem(server["ip"].toString()));
        tableWidget->setItem(i, 3, new QTableWidgetItem(QString::number(server["port"].toInt())));
        tableWidget->setItem(i, 4, new QTableWidgetItem(QString::number(server["id"].toInt())));
    }
}
