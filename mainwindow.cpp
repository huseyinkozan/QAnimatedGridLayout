#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QInputDialog>
#include <QPushButton>
#include <QEasingCurve>
#include "qanimatedgridlayout.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    statusBar()->hide();

    // Setup animated grid
    animatedLayout = new QAnimatedGridLayout(centralWidget());
    animatedLayout->setAnimationDuration(350);
    animatedLayout->setContentsMargins(10,20,10,20);
    ui->actionDuration->setText(
                QString("Duration:%1").arg(animatedLayout->animationDuration()));

    // See QEasingCurve doc for detailed explanation
    QEasingCurve easing(QEasingCurve::OutElastic);
    easing.setAmplitude(0.7);
    easing.setPeriod(1.0);
    animatedLayout->setEasingCurve(easing);

    QPushButton * button = NULL;

    int rowMax = 3;
    int colMax = 4;

    for (int row = 0; row < rowMax; ++row)
    {
        for (int col = 0; col < colMax; ++col)
        {
            // Add a button to each cell in the matrix
            button = new QPushButton(
                        QString("(%1,%2)").arg(row).arg(col),
                        centralWidget());
            // Set size policy to fill the cell properly
            button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
            animatedLayout->addWidget(button,row, col);
            connect(button, SIGNAL(clicked()), SLOT(buttonClicked()));
        }
    }
    // Lets add a button to a row
    button = new QPushButton(
                QString("(%1,%2) to (%3,%4)")
                    .arg(rowMax)
                    .arg(0)
                    .arg(rowMax)
                    .arg(colMax-1)
                ,
                centralWidget());
    button->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Expanding);
    animatedLayout->addWidget(button,rowMax, 0, 1, colMax);
    connect(button, SIGNAL(clicked()), SLOT(buttonClicked()));

    centralWidget()->setLayout(animatedLayout);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::buttonClicked()
{
    // On each click, switch the view
    if (animatedLayout->isZoomed())
        animatedLayout->showAll();
    else
        animatedLayout->zoomTo(qobject_cast<QPushButton*>(sender()));
}

void MainWindow::on_actionDuration_triggered()
{
    bool ok = false;
    int i = QInputDialog::getInt(this,
                                 "Duration",
                                 "Please enter a duration as miliseconds (1000ms = 1s):",
                                 animatedLayout->animationDuration(),
                                 50, 10000, 1, &ok);
    if (ok) {
        animatedLayout->setAnimationDuration(i);
        ui->actionDuration->setText(
                    QString("Duration:%1").arg(animatedLayout->animationDuration()));
    }
}
