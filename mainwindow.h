#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <string>
#include <vector>
#include <algorithm>

class application_controller;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    inline void setController(application_controller* c){controller = c;};

    void printToConsole(std::string);

    void start();

    void resetText();


private slots:

    void on_scanPrograms_clicked();

    void on_scanMemoryButton_clicked();

    void on_refreshButton_clicked();

    void on_writeButton_clicked();

    void on_selectProgram_clicked();


    void on_printValueButton_clicked();

private:
    Ui::MainWindow *ui;
    application_controller* controller;
};
#endif // MAINWINDOW_H
