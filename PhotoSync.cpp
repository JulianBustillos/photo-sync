#include "PhotoSync.h"
#include <QMessageBox>
#include <QFileDialog>


PhotoSync::PhotoSync(QWidget *parent)
    : QMainWindow(parent), m_settings(QCoreApplication::applicationDirPath()), m_fileManager(this)
{
    m_ui.setupUi(this);
    m_ui.progressBar->setValue(0);

    m_positiveDefaultText = m_ui.positivePushButton->text();

    if (m_settings.parseConfigFile()) {
        QString importPath, exportPath;
        bool remove = false;

        m_settings.getConfig(importPath, exportPath, remove);
        m_ui.importEdit->setText(importPath);
        m_ui.exportEdit->setText(exportPath);
        m_ui.deleteCheckBox->setChecked(remove);
    }

    QObject::connect(m_ui.importToolButton, &QToolButton::clicked, this, [&]() { selectDirectory("Import directory path", *m_ui.importEdit); });
    QObject::connect(m_ui.exportToolButton, &QToolButton::clicked, this, [&]() { selectDirectory("Export directory path", *m_ui.exportEdit); });
    QObject::connect(m_ui.positivePushButton, &QToolButton::clicked, this, &PhotoSync::run);
    QObject::connect(m_ui.negativePushButton, &QToolButton::clicked, this, &PhotoSync::close);

    QObject::connect(&m_fileManager, &FileManager::warning, this, &PhotoSync::createWarning);
    QObject::connect(&m_fileManager, &FileManager::progressBarValue, this, &PhotoSync::setProgressBarValue);
    QObject::connect(&m_fileManager, &FileManager::progressBarMaximum, this, &PhotoSync::setProgressBarMaximum);
    QObject::connect(&m_fileManager, &FileManager::output, this, &PhotoSync::appendOutput);
    QObject::connect(&m_fileManager, &FileManager::finished, this, &PhotoSync::finish);
    QObject::connect(this, &PhotoSync::warningAnswer, &m_fileManager, &FileManager::warningAnswer);
}

PhotoSync::~PhotoSync()
{
}

void PhotoSync::selectDirectory(QString title, QLineEdit& lineEdit)
{
    QString startDir = !lineEdit.text().isEmpty() ? lineEdit.text() : QDir::homePath();
    QString selectedDir = QFileDialog::getExistingDirectory(this, title, startDir);
    if (!selectedDir.isEmpty())
        lineEdit.setText(selectedDir);
}

void PhotoSync::run()
{
    m_settings.setConfig(m_ui.importEdit->text(), m_ui.exportEdit->text(), m_ui.deleteCheckBox->isChecked());
    
    QObject::disconnect(m_ui.positivePushButton, nullptr, nullptr, nullptr);
    QObject::connect(m_ui.positivePushButton, &QToolButton::clicked, &m_fileManager, &FileManager::cancel);
    m_ui.positivePushButton->setText("Cancel");

    m_fileManager.setSettings(m_ui.importEdit->text(), m_ui.exportEdit->text(), m_ui.deleteCheckBox->isChecked());
    m_fileManager.start(QThread::NormalPriority);
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

    if (m_fileManager.getStatus())
        m_settings.exportConfigFile();
}
