#pragma once

#include "FileManager.hpp"
#include "Settings.hpp"
#include "ui_PhotoSync.h"
#include <QLineEdit>
#include <QMainWindow>

class PhotoSync : public QMainWindow
{
    Q_OBJECT
    friend FileManager;

  public:
    PhotoSync(QWidget *parent = Q_NULLPTR);
    ~PhotoSync();

  private:
    void selectDirectory(QString title, QLineEdit &lineEdit);
    void run();

  private slots:
    void createWarning(QString title, QString message, bool emitAnswer);
    void setProgressBarValue(int value);
    void setProgressBarMaximum(int maximum);
    void appendOutput(QString output);
    void finish();

  signals:
    void warningAnswer(bool answer);

  private:
    Ui::PhotoSyncClass m_ui;
    Settings m_settings;
    FileManager m_fileManager;
};
