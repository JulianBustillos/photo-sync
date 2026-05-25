#pragma once
#include "FileData.hpp"
#include <QDir>
#include <QFileInfo>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <chrono>
#include <set>
#include <unordered_map>
#include <vector>

class FileManager : public QThread {
  Q_OBJECT

public:
  FileManager(QObject *parent = nullptr);
  ~FileManager();

public:
  void setSettings(const QString &importPath, const QString &exportPath,
                   bool removeFiles);
  bool getStatus() const;

public slots:
  void warningAnswer(bool answer);
  void cancel();

signals:
  void warning(QString title, QString message, bool emitAnswer);
  void progressBarValue(int value);
  void progressBarMaximum(int maximum);
  void output(QString output);

private:
  void run() override;

private:
  bool checkDir();
  bool checkRemove();
  bool getDate(const QFileInfo &fileInfo, Date &date);
  void buildExistingFileData();
  void buildImportFileData();
  void exportFiles();
  void removeFiles();
  void printStats();
  void printElapsedTime(std::chrono::steady_clock::time_point start,
                        std::chrono::steady_clock::time_point end);
  void addToProgress(int val);

private:
  QMutex m_mutex;
  QWaitCondition m_condition;
  QAtomicInt m_cancelled;

private:
  QDir m_importDir;
  QDir m_exportDir;
  bool m_removeFiles;
  const QStringList m_extensions;
  int m_runCount;
  int m_progress;
  int m_copyProgress;
  int m_removeProgress;
  std::unordered_map<qint64, std::vector<ExistingFile>> m_existingFiles;
  std::set<Date> m_DirectoriesToCreate;
  std::vector<ExportFile> m_filesToCopy;
  std::vector<QFileInfo> m_filesToRemove;
  std::set<QString> m_importErrors;
  std::set<QString> m_exportErrors;
  int m_duplicateCount;
  int m_copyCount;
  int m_removeCount;
  bool m_status;
};
