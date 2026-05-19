#include "ExtendedFileSystem.h"
#include "MTPFileSystem.h"
#include "WPDInstance.h"
#include <QDirIterator>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QString>


/* ExtendedFileSystem::Path */
ExtendedFileSystem::Path::Path(const QString & path) :
    m_path(path), m_type(Type::INVALID)
{
    if (!m_path.isEmpty()) {
        m_path.replace('\\', '/');
        if (m_path.back() == '/')
            m_path = m_path.left(m_path.size() - 1);
        QStringList splittedPath = m_path.split('/');
        if (QDir(splittedPath[0]).exists())
            m_type = Type::SYSTEM;
        else {
            QStringList devices;
            WPDInstance::get().getDevices(devices);
            for (const QString &device : devices) {
                if (device == splittedPath[0]) {
                    m_type = Type::MTP;
                    break;
                }
            }
        }
    }
}

ExtendedFileSystem::Path::~Path()
{
}

const QString & ExtendedFileSystem::Path::toQString() const
{
    return m_path;
}

bool ExtendedFileSystem::Path::isEmpty() const
{
    return m_path.isEmpty();
}

bool ExtendedFileSystem::Path::operator<(const Path & rhs) const
{
    return m_path < rhs.m_path;
}


/* ExtendedFileSystem::DirIterator */
ExtendedFileSystem::DirIterator::DirIterator(const Path & path, const QStringList & extensions) :
    m_path(path), m_iteratorImpl(nullptr)
{
    if (m_path.m_type == Path::Type::SYSTEM) {
        m_iteratorImpl = new QDirIterator(path.toQString(), extensions, QDir::Files, QDirIterator::Subdirectories);
    }
    else if (m_path.m_type == Path::Type::MTP) {
        m_iteratorImpl = new MTPFS::DirIterator(path.toQString(), extensions);
    }
}

ExtendedFileSystem::DirIterator::~DirIterator()
{
    if (m_iteratorImpl)
        delete m_iteratorImpl;
    m_iteratorImpl = nullptr;

}

bool ExtendedFileSystem::DirIterator::hasNext() const
{
    if (m_iteratorImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QDirIterator *>(m_iteratorImpl)->hasNext();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::DirIterator *>(m_iteratorImpl)->hasNext();
        }
    }
    return false;
}

ExtendedFileSystem::Path ExtendedFileSystem::DirIterator::next()
{
    if (m_iteratorImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return Path(static_cast<QDirIterator *>(m_iteratorImpl)->next());
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return Path(static_cast<MTPFS::DirIterator *>(m_iteratorImpl)->next());
        }
    }
    return Path(QString());
}


/* ExtendedFileSystem::Info */
ExtendedFileSystem::FileInfo::FileInfo(const Path & path) :
    m_path(path), m_infoImpl(nullptr)
{
    if (m_path.m_type == Path::Type::SYSTEM) {
        m_infoImpl = new QFileInfo(path.toQString());
    }
    else if (m_path.m_type == Path::Type::MTP) {
        m_infoImpl = new MTPFS::FileInfo(path.toQString());
    }
}

ExtendedFileSystem::FileInfo::~FileInfo()
{
    if (m_infoImpl)
        delete m_infoImpl;
    m_infoImpl = nullptr;
}

ExtendedFileSystem::Path ExtendedFileSystem::FileInfo::path() const
{
    if (m_infoImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return Path(static_cast<QFileInfo *>(m_infoImpl)->absoluteFilePath());
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::FileInfo *>(m_infoImpl)->path();
        }
    }
    return Path(QString());
}

QString ExtendedFileSystem::FileInfo::fileName() const
{
    if (m_infoImpl) {
        if (m_infoImpl) {
            if (m_path.m_type == Path::Type::SYSTEM) {
                return static_cast<QFileInfo *>(m_infoImpl)->fileName();
            }
            else if (m_path.m_type == Path::Type::MTP) {
                return static_cast<MTPFS::FileInfo *>(m_infoImpl)->fileName();
            }
        }
    }
    return QString();
}

void ExtendedFileSystem::FileInfo::setFile(const QString &name)
{
    if (m_infoImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            static_cast<QFileInfo *>(m_infoImpl)->setFile(name);
        }
        else if (m_path.m_type == Path::Type::MTP) {
            static_cast<MTPFS::FileInfo *>(m_infoImpl)->setFile(name);
        }
    }
}

QString ExtendedFileSystem::FileInfo::suffix() const
{
    if (m_infoImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFileInfo *>(m_infoImpl)->suffix();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::FileInfo *>(m_infoImpl)->suffix();
        }
    }
    return QString();
}

int ExtendedFileSystem::FileInfo::size() const
{
    if (m_infoImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFileInfo *>(m_infoImpl)->size();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::FileInfo *>(m_infoImpl)->size();
        }
    }
    return 0;
}

bool ExtendedFileSystem::FileInfo::exists() const
{
    if (m_infoImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFileInfo *>(m_infoImpl)->exists();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::FileInfo *>(m_infoImpl)->exists();
        }
    }
    return false;
}


/* ExtendedFileSystem::File */
ExtendedFileSystem::File::File(const Path & path) :
    m_path(path), m_fileImpl(nullptr)
{
    if (m_path.m_type == Path::Type::SYSTEM) {
        m_fileImpl = new QFile(path.toQString());
    }
    else if (m_path.m_type == Path::Type::MTP) {
        m_fileImpl = new MTPFS::File(path.toQString());
    }
}

ExtendedFileSystem::File::~File()
{
    if (m_fileImpl)
        delete m_fileImpl;
    m_fileImpl = nullptr;
}

bool ExtendedFileSystem::File::open(QIODevice::OpenMode mode)
{
    if (m_fileImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFile *>(m_fileImpl)->open(mode);
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::File *>(m_fileImpl)->open(mode);
        }
    }
    return false;
}

qint64 ExtendedFileSystem::File::write(const QByteArray &byteArray)
{
    if (m_fileImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFile *>(m_fileImpl)->write(byteArray);
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::File *>(m_fileImpl)->write(byteArray);
        }
    }
    return -1;
}

QByteArray ExtendedFileSystem::File::readAll()
{
    if (m_fileImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFile *>(m_fileImpl)->readAll();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::File *>(m_fileImpl)->readAll();
        }
    }
    return QByteArray();
}

void ExtendedFileSystem::File::close()
{
    if (m_fileImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            static_cast<QFile *>(m_fileImpl)->close();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            static_cast<MTPFS::File *>(m_fileImpl)->close();
        }
    }
}

bool ExtendedFileSystem::File::remove()
{
    if (m_fileImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QFile *>(m_fileImpl)->remove();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::File *>(m_fileImpl)->remove();
        }
    }
    return false;
}

int ExtendedFileSystem::File::remove(const QVector<Path> &paths, QVector<bool>& results)
{
    results.resize(paths.size());

    if (paths.isEmpty())
        return 0;

    Path::Type mainType = paths[0].m_type;
    QStringList pathList;
    for (int i = 0; i < paths.size(); i++) {
        pathList.append(paths[i].m_path);
        if (paths[i].m_type != mainType)
            return 0;
    }

    if (mainType == Path::Type::SYSTEM) {
        int removeCount = 0;
        for (int i = 0; i < pathList.size(); i++) {
            if (QFile(pathList[i]).remove()) {
                results[i] = true;
                removeCount++;
            }
        }

        return removeCount;
    }
    else if (mainType == Path::Type::MTP) {
        return MTPFS::File::remove(pathList, results);
    }

    return 0;
}


/* ExtendedFileSystem::Dir */
ExtendedFileSystem::Dir::Dir(const Path & path) :
    m_path(path), m_dirImpl(nullptr)
{
    if (m_path.m_type == Path::Type::SYSTEM) {
        m_dirImpl = new QDir(path.toQString());
    }
    else if (m_path.m_type == Path::Type::MTP) {
        m_dirImpl = new MTPFS::Dir(path.toQString());
    }
}

ExtendedFileSystem::Dir::~Dir()
{
    if (m_dirImpl)
        delete m_dirImpl;
    m_dirImpl = nullptr;
}

ExtendedFileSystem::Path ExtendedFileSystem::Dir::path(const QString &concatenate) const
{
    if (m_dirImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QDir *>(m_dirImpl)->absoluteFilePath(concatenate);
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::Dir *>(m_dirImpl)->path(concatenate);
        }
    }
    return Path();
}

bool ExtendedFileSystem::Dir::exists() const
{
    if (m_dirImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QDir *>(m_dirImpl)->exists();
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::Dir *>(m_dirImpl)->exists();
        }
    }
    return false;
}

bool ExtendedFileSystem::Dir::mkpath(const QString & dirPath) const
{
    if (m_dirImpl) {
        if (m_path.m_type == Path::Type::SYSTEM) {
            return static_cast<QDir *>(m_dirImpl)->mkpath(dirPath);
        }
        else if (m_path.m_type == Path::Type::MTP) {
            return static_cast<MTPFS::Dir *>(m_dirImpl)->mkpath(dirPath);
        }
    }
    return false;
}
