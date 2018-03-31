#include <QCandlestickSet>
#include <QStackedBarSeries>
#include <QBarSet>
#include <QDateTimeAxis>

#include "indexformula.h"

QT_CHARTS_USE_NAMESPACE

#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    postreader = new PostReader();
    connect(postreader, SIGNAL(progress_made()), this, SLOT(load_progress()));
    postreader->refresh();

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::load_progress() {
    qDebug() << postreader->processed() << " of " << postreader->to_process();
    if (!postreader->finished()) return;
    qDebug() << postreader->_posts[0].size() << postreader->_posts[1].size() << postreader->_posts[2].size();

    nfliSeries = new QCandlestickSeries();
    nfliSeries->setName("N坛韭指");
    nfliSeries->setIncreasingColor(QColor(Qt::red));
    nfliSeries->setDecreasingColor(QColor(Qt::green));
    QCandlestickSeries* volumeSeries = new QCandlestickSeries();
    volumeSeries->setIncreasingColor(QColor(Qt::red));
    volumeSeries->setDecreasingColor(QColor(Qt::green));

    double volmax = 0, idxmax = 0, idxmin = 4000;
    IndexFormula indexformula;
    indexformula.calculate(postreader);
    CandleStick tday = indexformula.k_day.back();
    CandleStick yday = indexformula.k_day.at(indexformula.k_day.size() - 2);
    CandleStick yyday = indexformula.k_day.at(indexformula.k_day.size() - 3);
    qDebug() << "今开" << tday.open
             << " 今收" << tday.close
             << " 今高*" << tday.high
             << " 今低*" << tday.low
             << " 日涨" << 100.0 * (tday.close / yday.close - 1.0) << "%";
    qDebug() << "昨开" << yday.open
             << " 昨收" << yday.close
             << " 昨高" << yday.high
             << " 昨低" << yday.low;
    qDebug() << "昨量" << (int)yday.volume << "，较前一日" << (yyday.volume > yday.volume ? "减少" : "增加") << (int)std::abs(yday.volume - yyday.volume) << "篇";
    qDebug() << "昨户" << (int)yday.holder << "，较前一日" << (yyday.holder > yday.holder ? "减少" : "增加") << (int)std::abs(yday.holder - yyday.holder) << "人";
    for(auto&& c : indexformula.k_day) {
        QCandlestickSet *set = new QCandlestickSet(c.timestamp * 1000.0);
        set->setOpen(c.open);
        set->setHigh(c.high);
        set->setLow(c.low);
        set->setClose(c.close);
        QCandlestickSet *vset = new QCandlestickSet(c.timestamp * 1000.0);
        if (c.open > c.close) {
            vset->setOpen(c.volume);
            vset->setClose(0);
            vset->setHigh(c.volume);
            vset->setLow(0);
        } else {
            vset->setOpen(0);
            vset->setClose(c.volume);
            vset->setHigh(c.volume);
            vset->setLow(0);
        }
        volmax = std::max(volmax, c.volume);
        idxmax = std::max(idxmax, c.high);
        idxmin = std::min(idxmin, c.low);
        if (set) {
            nfliSeries->append(set);
            volumeSeries->append(vset);
        }
    }

    chart = new QChart();
    chart->addSeries(nfliSeries);
    chart->setTitle("N坛韭指");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    axisX = new QDateTimeAxis();
    axisX->setFormat("MM.dd");
    chart->addAxis(axisX, Qt::AlignBottom);
    axisY = new QValueAxis();
    axisY->setMax(idxmax * 1.01);
    axisY->setMin(idxmin * 0.99 - (idxmax * 1.01 - idxmin * 0.99) * 0.2);
    chart->addAxis(axisY, Qt::AlignLeft);

    nfliSeries->attachAxis(axisX);
    nfliSeries->attachAxis(axisY);

    chart->addSeries(volumeSeries);
    QValueAxis* axisV = new QValueAxis();
    axisV->setMax(volmax * 5);
    axisV->setMin(0);
    axisV->setVisible(false);
    chart->addAxis(axisV, Qt::AlignLeft);
    volumeSeries->attachAxis(axisX);
    volumeSeries->attachAxis(axisV);

    chart->legend()->setVisible(false);

    chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setRubberBand(QChartView::HorizontalRubberBand);

    QMainWindow* window = new QMainWindow();
    window->setCentralWidget(chartView);
    window->resize(800, 600);
    window->show();

}
