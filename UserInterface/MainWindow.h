#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QPushButton>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    private slots:
        void fetchServerList();
    void onServerListReceived(QNetworkReply* reply);

private:
    QTableWidget *tableWidget;
    QNetworkAccessManager *networkManager;
};

#endif // MAINWINDOW_H
