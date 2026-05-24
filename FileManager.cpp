#include "FileManager.h"
#include "DateParser.h"
#include <QCryptographicHash>
#include <QDirIterator>

#define MAX_NAME_INDEX 100


FileManager::FileManager(QObject *parent) :
    QThread(parent), m_extensions({ "*.jpg" , "*.jpeg", "*.png", "*.mp4" }), m_cancelled(false), m_status(false), m_removeFiles(false),
    m_runCount(0), m_progress(0), m_copyProgress(0), m_removeProgress(0), m_duplicateCount(0), m_copyCount(0), m_removeCount(0)
{
}

FileManager::~FileManager()
{
    cancel();
    wait();
}

void FileManager::setSettings(const QString & importPath, const QString & exportPath, bool removeFiles)
{
    m_importDir = QDir(importPath);
    m_exportDir = QDir(exportPath);
    m_removeFiles = removeFiles;
}

bool FileManager::getStatus() const
{
    return m_status;
}

void FileManager::warningAnswer(bool answer)
{
    QMutexLocker locker(&m_mutex);
    m_removeFiles = answer;
    m_condition.wakeAll();
}

void FileManager::cancel()
{
    m_cancelled.storeRelaxed(true);
}

void FileManager::run()
{
    m_cancelled.storeRelaxed(false);
    m_progress = 0;
    addToProgress(0);
    emit progressBarMaximum(100);

    m_status = checkDir() && checkRemove();

    if (m_status) {
        std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

        if (m_runCount++)
            emit output(QString());

        emit output("FROM : " + m_importDir.absolutePath());
        emit output("TO       : " + m_exportDir.absolutePath());
        
        m_duplicateCount = 0;
        m_copyCount = 0;
        m_removeCount = 0;

        buildExistingFileData();
        buildImportFileData();
        exportFiles();
        removeFiles();
        printStats();

        m_importErrors.clear();
        m_exportErrors.clear();
        m_existingFiles.clear();
        m_DirectoriesToCreate.clear();
        m_filesToCopy.clear();
        m_filesToRemove.clear();

        std::chrono::steady_clock::time_point endTime = std::chrono::steady_clock::now();
        printElapsedTime(startTime, endTime);
    }

    m_status = m_status && !m_cancelled.loadRelaxed();
}

bool FileManager::checkDir()
{
    if (m_importDir.isEmpty()) {
        emit warning("Path error", "Import path is empty !", false);
        return false;
    }
    if (!m_importDir.exists()) {
        emit warning("Path error", "Import path : \"" + m_importDir.absolutePath() + "\" not found !", false);
        return false;
    }

    if (m_exportDir.isEmpty()) {
        emit warning("Path error", "Export path is empty !", false);
        return false;
    }
    if (!m_exportDir.exists()) {
        emit warning("Path error", "Export path : \"" + m_exportDir.absolutePath() + "\" not found !", false);
        return false;
    }

    return true;
}

bool FileManager::checkRemove()
{
    if (m_removeFiles) {
        QMutexLocker locker(&m_mutex);
        emit warning("Remove warning", "Files are going to be removed after copy. Proceed anyway ?", true);
        m_condition.wait(&m_mutex);
        return m_removeFiles;
    }
    return true;
}

bool FileManager::getDate(const QFileInfo &fileInfo, Date &date)
{
    bool isParsed = false;
    bool tryFileName = false;

    if (fileInfo.suffix().compare("jpg", Qt::CaseInsensitive) == 0 
        || fileInfo.suffix().compare("jpeg", Qt::CaseInsensitive) == 0) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            tryFileName = !DateParser::fromJPGBuffer(file.readAll(), date);
            file.close();
            isParsed = true;
        }
    }
    else if (fileInfo.suffix().compare("png", Qt::CaseInsensitive) == 0) {
        tryFileName = true;
        isParsed = true;
    }
    else if (fileInfo.suffix().compare("mp4", Qt::CaseInsensitive) == 0) {
        QFile file(fileInfo.absoluteFilePath());
        if (file.open(QIODevice::ReadOnly)) {
            tryFileName = !DateParser::fromMP4Buffer(file.readAll(), date);
            file.close();
            isParsed = true;
        }
    }

    if (tryFileName) {
        DateParser::fromFileName(fileInfo.fileName().toStdString(), date);
    }

    return isParsed;
}

void FileManager::buildExistingFileData()
{
    QDirIterator it(m_exportDir.absolutePath(), m_extensions, QDir::Files, QDirIterator::Subdirectories);
    while (it.hasNext()) {
        if (m_cancelled.loadRelaxed())
            return;

        QFileInfo fileInfo(it.next());
        m_existingFiles[fileInfo.size()].emplace_back(fileInfo);
    }
}

void FileManager::buildImportFileData()
{
    QDirIterator it(m_importDir.absolutePath(), m_extensions, QDir::Files, QDirIterator::Subdirectories);
    QVector<QFileInfo> importFilePaths;

    while (it.hasNext())
        importFilePaths.append(QFileInfo(it.next()));

    int progressSize;
    if (m_removeFiles) {
        progressSize = importFilePaths.size() * 10;
        m_copyProgress = 9;
        m_removeProgress = 1;
    }
    else {
        progressSize = importFilePaths.size();
        m_copyProgress = 1;
        m_removeProgress = 0;
    }
    
    if (progressSize == 0)
        addToProgress(100);
    else
        emit progressBarMaximum(progressSize);

    for (QFileInfo &fileInfo : importFilePaths) {
        if (m_cancelled.loadRelaxed())
            return;

        bool copyFile = true;

        auto filesIt = m_existingFiles.find(fileInfo.size());
        if (filesIt != m_existingFiles.end()) {
            QCryptographicHash hash(QCryptographicHash::Md5);
            QFile newFile(fileInfo.absoluteFilePath());
            if (!newFile.open(QIODevice::ReadOnly)) {
                m_importErrors.insert(fileInfo.absoluteFilePath());
                addToProgress(m_copyProgress + m_removeProgress);
                continue;
            }
            hash.addData(newFile.readAll());
            QByteArray newFileChecksum = hash.result();
            newFile.close();

            for (ExistingFile &fileData : filesIt->second) {
                if (fileData.m_checksum.isEmpty()) {
                    QFile existingFile(fileData.m_info.absoluteFilePath());
                    if (!existingFile.open(QIODevice::ReadOnly)) {
                        m_exportErrors.insert(fileData.m_info.absoluteFilePath());
                        continue;
                    }

                    hash.reset();
                    hash.addData(existingFile.readAll());
                    fileData.m_checksum = hash.result();
                    existingFile.close();
                }
                
                if (newFileChecksum == fileData.m_checksum) {
                    copyFile = false;
                    m_duplicateCount++;
                    break;
                }
            }
        }

        if (copyFile) {
            Date date;
            if (!getDate(fileInfo, date)) {
                m_importErrors.insert(fileInfo.absoluteFilePath());
                addToProgress(m_copyProgress + m_removeProgress);
                continue;
            }
            m_DirectoriesToCreate.insert(date);
            m_filesToCopy.emplace_back(fileInfo, date);
        }
        else {
            if (m_removeFiles)
                m_filesToRemove.push_back(fileInfo);
            addToProgress(m_copyProgress);
        }

    }
}

void FileManager::exportFiles()
{
    for (auto &date : m_DirectoriesToCreate) {
        m_exportDir.mkpath(date.toQString());
    }

    for (ExportFile &fileToCopy : m_filesToCopy) {
        if (m_cancelled.loadRelaxed())
            return;

        QString fileName = fileToCopy.m_info.fileName();
        QFileInfo exportFileInfo(m_exportDir.absolutePath() + fileToCopy.m_date.toQString() + "/" + fileName);
        
        int count = 0;
        bool copy = false;

        if (exportFileInfo.exists()) {
            int index = fileName.lastIndexOf(".");
            QString name = fileName.left(index);
            QString extension = fileName.right(fileName.size() - index);
            while (exportFileInfo.exists() && count < MAX_NAME_INDEX) {
                exportFileInfo.setFile(name + "_" + QString::number(++count) + extension);
            }
        }

        if (count < MAX_NAME_INDEX) {
            QFile importFile(fileToCopy.m_info.absoluteFilePath());
            QFile exportFile(exportFileInfo.absoluteFilePath());

            if (importFile.open(QIODevice::ReadOnly) && exportFile.open(QIODevice::WriteOnly))
                copy = (exportFile.write(importFile.readAll()) >= 0);

            importFile.close();
            exportFile.close();
        }

        if (copy) {
            m_copyCount++;
            if (m_removeFiles)
                m_filesToRemove.push_back(fileToCopy.m_info);
            addToProgress(m_copyProgress);
        }
        else {
            m_importErrors.insert(fileToCopy.m_info.absoluteFilePath());
            addToProgress(m_copyProgress + m_removeProgress);
        }

    }

}

void FileManager::removeFiles()
{
    for (QFileInfo& fileToRemove : m_filesToRemove)
    {
        QFile file(fileToRemove.absoluteFilePath());
        if (file.remove())
            m_removeCount++;
        else
            m_importErrors.insert(fileToRemove.absoluteFilePath());

        addToProgress(m_removeProgress);
    }
}

void FileManager::printStats()
{
    if (m_duplicateCount > 0)
        emit output("Found " + QString::number(m_duplicateCount) + (m_duplicateCount > 1 ? " already existing files." : " already existing file."));

    emit output(QString::number(m_copyCount) + (m_copyCount > 1 ? " files copied." : " file copied."));

    if (m_removeCount > 0) {
        emit output(QString::number(m_removeCount) + (m_removeCount > 1 ? " files removed." : " file removed."));
    }

    if (m_exportErrors.size() > 0) {
        emit output("ERROR : " + QString::number(m_exportErrors.size()) + ((m_exportErrors.size() > 1) ? " files in export directory could not be read !" : " file in export directory could not be read !"));
    }

    if (m_importErrors.size() > 0) {
        emit output("ERROR : " + QString::number(m_importErrors.size()) + ((m_importErrors.size() > 1) ? " files in import directory could not be read !" : " file in import directory could not be read !"));
    }

    if (m_cancelled.loadRelaxed()) {
        emit output("SYNC CANCELED !");
    }
}

void FileManager::printElapsedTime(std::chrono::steady_clock::time_point start, std::chrono::steady_clock::time_point end)
{
    int elapsedTime = (int)std::round(std::chrono::duration_cast<std::chrono::seconds>(end - start).count());
    int h = 0, m = 0, s = 0;

    if (elapsedTime >= 3600) {
        h = elapsedTime / 3600;
        elapsedTime -= h * 3600;
    }

    if (elapsedTime >= 60) {
        m = elapsedTime / 60;
        elapsedTime -= m * 60;
    }

    s = elapsedTime;

    QString timeToPrint = QString::number(h).rightJustified(2, '0') + ":" + QString::number(m).rightJustified(2, '0') + ":" + QString::number(s).rightJustified(2, '0');
    emit output("Elapsed time : " + timeToPrint);
}

void FileManager::addToProgress(int val)
{
    emit progressBarValue(m_progress = m_progress + val);
}
