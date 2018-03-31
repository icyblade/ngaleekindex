#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QCandlestickSeries>
#include <QtCharts/QChartView>
#include <QtCharts/QValueAxis>
#include <QtCore/QDateTime>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMainWindow>

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "post.h"

QT_CHARTS_USE_NAMESPACE

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void load_progress();

private:
    Ui::MainWindow *ui;
    PostReader* postreader;
    QCandlestickSeries *nfliSeries;
    QChart *chart;
    QDateTimeAxis *axisX;
    QValueAxis *axisY;
    QChartView *chartView;

};

#endif // MAINWINDOW_H
