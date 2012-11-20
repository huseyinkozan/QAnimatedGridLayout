#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}
class QAnimatedGridLayout;

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void buttonClicked();
    
private slots:
    void on_actionDuration_triggered();

private:
    Ui::MainWindow *ui;
    QAnimatedGridLayout * animatedLayout;
};

#endif // MAINWINDOW_H
