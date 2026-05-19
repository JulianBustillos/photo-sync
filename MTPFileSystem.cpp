#include "MTPFileSystem.h"
#include "WPDInstance.h"


/* MTPFileSystem::DirIterator */
MTPFileSystem::DirIterator::DirIterator(const QString & rootPath, const QStringList & extensions) :
    m_rootLevel(rootPath.count('/') + 1), m_extensions(extensions)
{
    findNext(rootPath + "/");
}

bool MTPFileSystem::DirIterator::hasNext() const
{
    return !m_next.isEmpty();
}

QString MTPFileSystem::DirIterator::next()
{
    if (hasNext()) {
        QString current = m_next;
        findNext(current);
        return current;
    }
    return QString();
}

void MTPFileSystem::DirIterator::findNext(const QString & current)
{
    m_next = QString();
    QStringList splittedPath = current.split('/');
    int level = splittedPath.size();
    bool searching = true;

    while (searching && level > m_rootLevel) {
        QString path = splittedPath[0];
        for (int i = 1; i < level - 1; i++)
            path += "/" + splittedPath[i];
        QVector<WPDManager::Item> content;
        WPDInstance::get().getContent(path, content);

        int id = -1;
        if (!splittedPath[level - 1].isEmpty()) {
            for (id = 0; id < content.size(); id++) {
                if (content[id].m_name == splittedPath[level - 1])
                    break;
            }
        }
        for (++id; id < content.size() && searching; id++) {
            if (content[id].m_type == WPDManager::ItemType::FILE) {
                int pointId = content[id].m_name.lastIndexOf('.') + 1;
                QString currentExt = content[id].m_name.mid(pointId, content[id].m_name.size() - pointId);

                for (const QString &extension : m_extensions) {
                    QString filterExt = extension.mid(2, extension.size() - 2);
                    if (currentExt.compare(filterExt, Qt::CaseInsensitive) == 0) {
                        splittedPath[level - 1] = content[id].m_name;
                        searching = false;
                        break;
                    }
                }
                continue;
            }
            else {
                if (splittedPath.size() == level)
                    splittedPath.append("");
                splittedPath[level - 1] = content[id].m_name;
                splittedPath[level++] = "";
                break;
            }
        }

        if (searching && id == content.size())
            level--;
    }

    if (!searching) {
        m_next = splittedPath[0];
        for (int i = 1; i < level; i++)
            m_next += "/" + splittedPath[i];
    }
}


/* MTPFileSystem::FileInfo */
MTPFileSystem::FileInfo::FileInfo(const QString & path) :
    m_path(path)
{
}

MTPFileSystem::FileInfo::~FileInfo()
{
}

QString MTPFileSystem::FileInfo::path() const
{
    return m_path;
}

QString MTPFileSystem::FileInfo::fileName() const
{
    int index = m_path.lastIndexOf("/");
    return m_path.right(m_path.size() - index - 1);
}

QString MTPFileSystem::FileInfo::suffix() const
{
    int lastSeparator = m_path.lastIndexOf('/');
    int lastPoint = m_path.lastIndexOf('.');
    if (lastSeparator >= lastPoint)
        return QString();
    return m_path.right(m_path.size() - lastPoint - 1);
}

void MTPFileSystem::FileInfo::setFile(const QString &name)
{
    int index = m_path.lastIndexOf("/");
    m_path = m_path.left(index + 1) + name;
}

int MTPFileSystem::FileInfo::size() const
{
    WPDManager::Item item;
    WPDInstance::get().getItem(m_path, item);
    return item.m_size;
}

bool MTPFileSystem::FileInfo::exists() const
{
    WPDManager::Item item;
    return WPDInstance::get().getItem(m_path, item);
}


/* MTPFileSystem::File */
MTPFileSystem::File::File(const QString & path) :
    m_mode(QIODevice::NotOpen), m_path(path), m_size(0)
{
}

MTPFileSystem::File::~File()
{
    close();
}

bool MTPFileSystem::File::open(QIODevice::OpenMode mode)
{
    m_mode = mode;
    m_size = 0;
    WPDManager::Item item;

    switch (m_mode) {
    case QIODevice::ReadOnly:
        return WPDInstance::get().getItem(m_path, item) && (item.m_type == WPDManager::ItemType::FILE) && (m_size = item.m_size);
    case QIODevice::WriteOnly:
        int index = m_path.lastIndexOf("/");
        if (index > 0)
            return WPDInstance::get().getItem(m_path.left(index), item) && (item.m_type != WPDManager::ItemType::FILE) &&!WPDInstance::get().getItem(m_path, item);
    }

    m_mode = QIODevice::NotOpen;
    return false;
}

qint64 MTPFileSystem::File::write(const QByteArray & fileData)
{
    if (m_mode == QIODevice::WriteOnly) {
        int index = m_path.lastIndexOf("/");
        if (WPDInstance::get().createFile(m_path.left(index), m_path.right(m_path.size() - index - 1), fileData.data(), fileData.size()))
            return fileData.size();
    }
    return -1;
}

QByteArray MTPFileSystem::File::readAll()
{
    if (m_mode == QIODevice::ReadOnly && m_size > 0) {
        QByteArray fileData(m_size, '0');
        if (WPDInstance::get().readData(m_path, fileData.data()))
            return fileData;
    }
    return QByteArray();
}

void MTPFileSystem::File::close()
{
    m_mode = QIODevice::NotOpen;
    m_size = 0;
}

bool MTPFileSystem::File::remove()
{
    if (m_mode == QIODevice::NotOpen) {
        QStringList path;
        QVector<bool> result;
        path.append(m_path);
        return WPDInstance::get().deleteObjects(path, result);
    }
    return false;
}

int MTPFileSystem::File::remove(const QStringList& pathList, QVector<bool>& results)
{
    return WPDInstance::get().deleteObjects(pathList, results);
}

/* MTPFileSystem::Dir */
MTPFileSystem::Dir::Dir(const QString & path) :
    m_path(path)
{
}

MTPFileSystem::Dir::~Dir()
{
}

QString MTPFileSystem::Dir::path(const QString &concatenate) const
{
    return m_path + "/" + QString(concatenate).replace("\\", "/");
}

bool MTPFileSystem::Dir::exists() const
{
    WPDManager::Item item;
    return WPDInstance::get().getItem(m_path, item);
}

bool MTPFileSystem::Dir::mkpath(const QString & dirPath) const
{
    QString tempPath = dirPath;
    QStringList folders = tempPath.replace("\\", "/").split("/");
    
    bool created = true;
    int index = 0;
    QString existingPath = m_path;
    WPDManager::Item item;
    while (index < folders.size() && created) {
        QString pathToCreate = existingPath + "/" + folders[index];
        if (!WPDInstance::get().getItem(pathToCreate, item))
            created = WPDInstance::get().createFolder(existingPath, folders[index]);
        existingPath = pathToCreate;
        index++;
    }

    return created;
}