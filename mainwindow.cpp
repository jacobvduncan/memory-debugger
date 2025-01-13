#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "application_controller.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::printToConsole(std::string s){
    ui->console->addItem(QString::fromStdString(s));
}



void MainWindow::on_scanPrograms_clicked()
{
    controller->debug_test();
    std::vector<std::tuple<std::string, DWORD>> processes = controller->getScannedProcesses();
    ui->programList->clear();
    for (const auto& process : processes) {
        std::string processName = std::get<0>(process);
        DWORD processId = std::get<1>(process);
        QString itemText = QString::fromStdString(processName);
        QListWidgetItem* item = new QListWidgetItem(itemText);
        item->setData(Qt::UserRole, static_cast<qulonglong>(processId));
        ui->programList->addItem(item);
    }
    ui->programList->sortItems();
}


void MainWindow::on_scanMemoryButton_clicked()
{
    if (ui->programList->currentItem() == nullptr) {
        return;
    }
    std::string name = ui->programList->currentItem()->text().toStdString();
    DWORD processId = static_cast<DWORD>(ui->programList->currentItem()->data(Qt::UserRole).toULongLong());
    //printToConsole("Selected process: " + name + ", Process ID: " + std::to_string(processId));
    std::string searchText = ui->typeValue->text().toStdString();
    std::string typeText = ui->typeSelect->currentText().toStdString();
    if (searchText.empty()) {
        return;
    }
    if (typeText != "Integer" && typeText != "String" && typeText != "Float") {
        return;
    }
    if (typeText == "Integer") {
        try {
            std::stoi(searchText);
        } catch (std::invalid_argument&) {
            return;
        } catch (std::out_of_range&) {
            return;
        }
    } else if (typeText == "Float") {
        try {
            std::stof(searchText);
        } catch (std::invalid_argument&) {
            return;
        } catch (std::out_of_range&) {
            return;
        }
    }
    ScanResult result = controller->scan(searchText, typeText);
    ui->memoryList->clear();
    for (uintptr_t addr : result.addresses) {
        ui->memoryList->addItem(QString("0x%1").arg(addr, 0, 16));
    }
    std::string s = "RESULTS: " + std::to_string(result.addresses.size());
    ui->resultText->setText(QString::fromStdString(s));
    s = "TOTAL SCANS: " + std::to_string(controller->getTotalScans());
    ui->totalScanText->setText(QString::fromStdString(s));
}

void MainWindow::start(){
    on_scanPrograms_clicked();
}

void MainWindow::on_refreshButton_clicked()
{
    controller->refresh();
    ui->memoryList->clear();
    resetText();
}


void MainWindow::on_writeButton_clicked()
{
    if (ui->programList->currentItem() == nullptr) {
        return;
    }
    if (ui->memoryList->selectedItems().isEmpty()) {
        //printToConsole("Error: No memory address selected.");
        return;
    }
    DWORD processId = static_cast<DWORD>(ui->programList->currentItem()->data(Qt::UserRole).toULongLong());
    std::string writeText = ui->writeBox->text().toStdString();
    std::string typeText = ui->typeSelect->currentText().toStdString();
    QString selectedAddress = ui->memoryList->currentItem()->text();
    uintptr_t address = selectedAddress.toULongLong(nullptr, 16);
    if (controller->writeToMemory(processId, typeText, writeText, address)) {
        printToConsole(("SUCCESSFULLY WROTE VALUE '" + writeText));
    } else {
        printToConsole(("FAILED TO WRITE VALUE '" + writeText));
    }
}


void MainWindow::on_selectProgram_clicked()
{
    if (ui->programList->currentItem() == nullptr) {
        return;
    }
    DWORD processId = static_cast<DWORD>(ui->programList->currentItem()->data(Qt::UserRole).toULongLong());
    std::string name = ui->programList->currentItem()->text().toStdString();
    std::transform(name.begin(), name.end(), name.begin(), ::toupper);
    std::string s = "SELECTED PROGRAM : " + name + " (PID " + std::to_string(processId) + ")";
    controller->setPid(processId);
    ui->programText->setText(QString::fromStdString(s));
    resetText();
}

void MainWindow::resetText(){
    std::string s = "RESULTS: 0";
    ui->resultText->setText(QString::fromStdString(s));

    s = "TOTAL SCANS: " + std::to_string(controller->getTotalScans());
    ui->totalScanText->setText(QString::fromStdString(s));
}


void MainWindow::on_printValueButton_clicked()
{
    if (ui->programList->currentItem() == nullptr) {
        return;
    }
    DWORD processId = static_cast<DWORD>(ui->programList->currentItem()->data(Qt::UserRole).toULongLong());
    std::string typeText = ui->typeSelect->currentText().toStdString();
    QString selectedAddress = ui->memoryList->currentItem()->text();
    uintptr_t address = selectedAddress.toULongLong(nullptr, 16);
    std::string s = controller->readValueFromAddress(processId,typeText,address);
    printToConsole(s);


}

