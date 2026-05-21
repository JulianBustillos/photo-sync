#include "PhotoSync.h"
#include "WPDInstance.h"
#include <QMessageBox>


PhotoSync::PhotoSync(QWidget *parent)
    : QMainWindow(parent), m_settings(nullptr), m_fileDialog(nullptr), m_fileManager(nullptr)
{
    m_ui.setupUi(this);
    m_ui.progressBar->setValue(0);

    m_positiveDefaultText = m_ui.positivePushButton->text();

    m_settings = new Settings(QCoreApplication::applicationDirPath());
    m_fileDialog = new FileExplorerDialog(this);
    m_fileManager = new FileManager(this);

    if (m_settings && m_settings->parseConfigFile()) {
        QString importPath, exportPath;
        bool remove = false;

        m_settings->getConfig(importPath, exportPath, remove);
        m_ui.importEdit->setText(importPath);
        m_ui.exportEdit->setText(exportPath);
        m_ui.deleteCheckBox->setChecked(remove);
    }

    QObject::connect(m_ui.importToolButton, &QToolButton::clicked, this, &PhotoSync::askImportFolder);
    QObject::connect(m_ui.exportToolButton, &QToolButton::clicked, this, &PhotoSync::askExportFolder);
    QObject::connect(m_ui.positivePushButton, &QToolButton::clicked, this, &PhotoSync::run);
    QObject::connect(m_ui.negativePushButton, &QToolButton::clicked, this, &PhotoSync::close);

    if (m_fileManager) {
        QObject::connect(m_fileManager, &FileManager::warning, this, &PhotoSync::createWarning);
        QObject::connect(m_fileManager, &FileManager::progressBarValue, this, &PhotoSync::setProgressBarValue);
        QObject::connect(m_fileManager, &FileManager::progressBarMaximum, this, &PhotoSync::setProgressBarMaximum);
        QObject::connect(m_fileManager, &FileManager::output, this, &PhotoSync::appendOutput);
        QObject::connect(m_fileManager, &FileManager::finished, this, &PhotoSync::finish);
        QObject::connect(this, &PhotoSync::warningAnswer, m_fileManager, &FileManager::warningAnswer);
    }
}

PhotoSync::~PhotoSync()
{
    if (m_fileManager)
        delete m_fileManager;
    m_fileManager = nullptr;

    if (m_fileDialog)
        delete m_fileDialog;
    m_fileDialog = nullptr;

    if (m_settings)
        delete m_settings;
    m_settings = nullptr;

    WPDInstance::free();
}

void PhotoSync::askImportFolder()
{
    if (m_fileDialog) {
        m_fileDialog->setWindowTitle("Import directory path");
        m_fileDialog->setDirectory(m_ui.importEdit->text());
        int code = m_fileDialog->exec();
        if (code == QDialog::Accepted)
            m_ui.importEdit->setText(m_fileDialog->getDirectory());
    }
}

void PhotoSync::askExportFolder()
{
    if (m_fileDialog) {
        m_fileDialog->setWindowTitle("Export directory path");
        m_fileDialog->setDirectory(m_ui.exportEdit->text());
        int code = m_fileDialog->exec();
        if (code == QDialog::Accepted)
            m_ui.exportEdit->setText(m_fileDialog->getDirectory());
    }
}

void PhotoSync::run()
{
    if (m_settings)
        m_settings->setConfig(m_ui.importEdit->text(), m_ui.exportEdit->text(), m_ui.deleteCheckBox->isChecked());
    
    if (m_fileManager) {
        QObject::disconnect(m_ui.positivePushButton, nullptr, nullptr, nullptr);
        QObject::connect(m_ui.positivePushButton, &QToolButton::clicked, m_fileManager, &FileManager::cancel);
        m_ui.positivePushButton->setText("Cancel");

        m_fileManager->setSettings(m_ui.importEdit->text(), m_ui.exportEdit->text(), m_ui.deleteCheckBox->isChecked());
        m_fileManager->start(QThread::NormalPriority);
    }
}

void PhotoSync::createWarning(QString title, QString message, bool emitAnswer)
{
    QMessageBox::StandardButton button = QMessageBox::warning(this, title, message, emitAnswer ? QMessageBox::Ok | QMessageBox::Cancel : QMessageBox::Ok);
    if (emitAnswer)
        emit warningAnswer(button == QMessageBox::Ok);
}

void PhotoSync::setProgressBarValue(int value)
{
    m_ui.progressBar->setValue(value);
}

void PhotoSync::setProgressBarMaximum(int maximum)
{
    m_ui.progressBar->setMaximum(maximum);
}

void PhotoSync::appendOutput(QString output)
{
    m_ui.textEditOutput->append(output);
}

void PhotoSync::finish()
{
    QObject::disconnect(m_ui.positivePushButton, nullptr, nullptr, nullptr);
    QObject::connect(m_ui.positivePushButton, &QToolButton::clicked, this, &PhotoSync::run);
    m_ui.positivePushButton->setText(m_positiveDefaultText);

    if (m_settings && m_fileManager && m_fileManager->getStatus())
        m_settings->exportConfigFile();
}
