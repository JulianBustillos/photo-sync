#include "Settings.hpp"
#include <QFile>
#include <QTextStream>

const QString Settings::ConfigFilename = "PhotoSync.cfg";
const QString Settings::ImportVar = "IMPORT_PATH";
const QString Settings::ExportVar = "EXPORT_PATH";
const QString Settings::RemoveVar = "REMOVE";

Settings::Settings(const QString &executablePath)
    : m_configPath(executablePath + "/" + ConfigFilename), m_importPath(""),
      m_exportPath(""), m_remove(false) {}

Settings::~Settings() {}

bool Settings::parseConfigFile() {
  QFile configFile(m_configPath);
  if (configFile.open(QIODevice::ReadOnly)) {
    QTextStream configFileStream(&configFile);
    m_importPath = "";
    m_exportPath = "";
    m_remove = false;

    while (!configFileStream.atEnd()) {
      QString line = configFileStream.readLine();
      QStringList splittedLine = line.split("=");

      if (splittedLine.size() == 2) {
        if (splittedLine[0] == ImportVar) {
          m_importPath = splittedLine[1];
        } else if (splittedLine[0] == ExportVar) {
          m_exportPath = splittedLine[1];
        } else if (splittedLine[0] == RemoveVar) {
          if (splittedLine[1] == "true")
            m_remove = true;
          else if (splittedLine[1] == "false")
            m_remove = false;
        }
      }
    }

    configFile.close();
    return true;
  }
  return false;
}

bool Settings::exportConfigFile() const {
  QFile configFile(m_configPath);
  if (configFile.open(QIODevice::WriteOnly)) {
    QTextStream configStream(&configFile);
    configStream << ImportVar << "=" << m_importPath << "\n";
    configStream << ExportVar << "=" << m_exportPath << "\n";
    configStream << RemoveVar << "=" << (m_remove ? "true" : "false") << "\n";

    configFile.close();
    return true;
  }
  return false;
}

void Settings::setConfig(const QString &importPath, const QString &exportPath,
                         bool remove) {
  m_importPath = importPath;
  m_exportPath = exportPath;
  m_remove = remove;
}

void Settings::getConfig(QString &importPath, QString &exportPath,
                         bool &remove) const {
  importPath = m_importPath;
  exportPath = m_exportPath;
  remove = m_remove;
}
