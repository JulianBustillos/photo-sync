#pragma once

#include <QMainWindow>
#include <QLineEdit>
#include "ui_PhotoSync.h"
#include "Settings.h"
#include "FileManager.h"

class PhotoSync : public QMainWindow
{
    Q_OBJECT
    friend FileManager;

public:
    PhotoSync(QWidget *parent = Q_NULLPTR);
    ~PhotoSync();

private :
    void askDirectory(QString title, QLineEdit& lineEdit);
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
    QString m_positiveDefaultText;
    Settings *m_settings;
    FileManager *m_fileManager;
};
