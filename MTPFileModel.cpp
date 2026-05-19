#include "MTPFileModel.h"
#include "WPDInstance.h"
#include <QVector>
#include <QLocale>


MTPFileModel::MTPFileModel(QObject * parent) :
    AggregableItemModel(parent)
{
    m_root = new MTPFileNode();
    m_fetcher = new MTPFileFetcher(this);
    m_observer = new Observer(*this);

    if (m_observer)
        WPDInstance::get().registerForEvents(m_observer);
    if (m_fetcher) {
        QObject::connect(m_fetcher, &MTPFileFetcher::loadedDevices, this, &MTPFileModel::updateDevices);
        QObject::connect(m_fetcher, &MTPFileFetcher::loadedContent, this, &MTPFileModel::populate);
        m_fetcher->start(QThread::LowPriority);
    }
}

MTPFileModel::~MTPFileModel()
{
    cleanNodes();

    if (m_observer) {
        WPDInstance::get().unregisterForEvents(m_observer);
        delete m_observer;
    }
    m_observer = nullptr;

    if (m_fetcher)
        delete m_fetcher;
    m_fetcher = nullptr;

    if (m_root)
        delete m_root;
    m_root = nullptr;
}

void MTPFileModel::setCurrentPath(const QString & newPath)
{
    m_currentStack.clear();
    QList<QString> splittedPath = QString(newPath).replace('\\', '/').split('/');
    for (auto fileName = splittedPath.rbegin(); fileName != splittedPath.rend(); fileName++)
        m_currentStack.push(*fileName);

    MTPFileNode *node = m_root;

    while (node && node->isPopulated() && !m_currentStack.isEmpty()) {
        QString &fileName = m_currentStack.top();
        node = node->getChildren().value(fileName);
        m_currentStack.pop();
    }

    if (!node) {
        m_currentStack.clear();
        return;
    }

    if (m_fetcher && !m_currentStack.isEmpty()) {
        m_currentStack.push(node->getName());
        m_fetcher->addToFetch(node);
        return;
    }

    if (node && node->getParent()) {
        int row = node->getParent()->visibleLocation(node->getName());
        emit currentPathChanged(createIndex(row, 0, node));
    }
}

QString MTPFileModel::filePath(const QModelIndex & index) const
{
    if (index.isValid()) {
        MTPFileNode *node = indexNode(index);
        if (node)
            return node->getPath();
    }
    return QString();
}

QModelIndex MTPFileModel::index(int row, int column, const QModelIndex & parent) const
{
    if (row < 0 || column < 0 || row >= rowCount(parent) || column >= columnCount(parent))
        return QModelIndex();

    MTPFileNode *parentNode = indexNode(parent);
    if (row >= parentNode->getVisibleChildren().size())
        return QModelIndex();

    const QString &childName = parentNode->getVisibleChildren().at(row);
    MTPFileNode *indexNode = parentNode->getChildren().value(childName);
    if (m_fetcher)
        m_fetcher->addToFetch(indexNode);

    return createIndex(row, column, indexNode);
}

QModelIndex MTPFileModel::parent(const QModelIndex & child) const
{
    if (!child.isValid())
        return QModelIndex();

    MTPFileNode *childNode = indexNode(child);
    if (childNode)
        return parent(*childNode);
    return QModelIndex();
}

bool MTPFileModel::hasChildren(const QModelIndex & parent) const
{
    if (parent.column() > 0)
        return false;

    if (!parent.isValid())
        return true;

    return (indexNode(parent)->isDir());
}

bool MTPFileModel::canFetchMore(const QModelIndex & parent) const
{
    return (!indexNode(parent)->isPopulated());
}

void MTPFileModel::fetchMore(const QModelIndex & parent)
{
    MTPFileNode *parentNode = indexNode(parent);
    if (m_fetcher && !parentNode->isPopulated())
        m_fetcher->addToFetch(parentNode);
}

int MTPFileModel::rowCount(const QModelIndex & parent) const
{
    if (parent.column() > 0)
        return 0;

    if (!parent.isValid())
        return m_root->getVisibleChildren().count();

    MTPFileNode *parentNode = indexNode(parent);
    return parentNode->getVisibleChildren().count();
}

int MTPFileModel::columnCount(const QModelIndex & parent) const
{
    return (parent.column() > 0) ? 0 : 4;
}

QVariant MTPFileModel::data(const QModelIndex & index, int role) const
{
    if (!index.isValid())
        return QVariant();

    MTPFileNode *node = indexNode(index);
    if (!node)
        return QVariant();

    switch (role) {
    case Qt::EditRole:
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0: return node->getName();
        case 1: return FormatSize(node->getSize());
        case 2: return node->getType();
        case 3: return node->getLastModified();
        default:
            qWarning("data: invalid display value column %d", index.column());
            break;
        }
        break;
    case Qt::DecorationRole:
        if (index.column() == 0)
            return node->getQIcon();
        return QIcon();
    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return QVariant(Qt::AlignTrailing | Qt::AlignVCenter);
        break;
    case Qt::ForegroundRole:
        return node->isPopulated() ? QColor(Qt::black) : QColor(Qt::gray);
    default:
        return QVariant();
    }
    return QVariant();
}

QVariant MTPFileModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    switch (role) {
    case Qt::DecorationRole:
        if (section == 0) {
            QImage pixmap(16, 1, QImage::Format_ARGB32_Premultiplied);
            pixmap.fill(Qt::transparent);
            return pixmap;
        }
        break;
    case Qt::TextAlignmentRole:
        return Qt::AlignLeft;
    }

    if (orientation != Qt::Horizontal || role != Qt::DisplayRole)
        return QAbstractItemModel::headerData(section, orientation, role);

    QString returnValue;
    switch (section) {
    case 0: returnValue = tr("Name");
        break;
    case 1: returnValue = tr("Size");
        break;
    case 2: returnValue = tr("Type", "All other platforms");
        break;
    case 3: returnValue = tr("Date Modified");
        break;
    default: return QVariant();
    }
    return returnValue;
}

void MTPFileModel::updateDevices(const QStringList &devices)
{
    cleanNodes();

    QModelIndex index = createIndex(-1, -1, nullptr);
    QStringList oldDevices;
    QVector<int> newDevices;

    for (auto &deviceIter : m_root->getChildren()) {
        oldDevices.append(deviceIter->getName());
    }

    for (int i = 0; i < devices.size(); i++) {
        const QString &newDevice = devices[i];
        if (oldDevices.removeAll(newDevice) == 0)
            newDevices.append(i);
    }

    //Remove old devices
    if (!oldDevices.isEmpty()) {
        int idMin = m_root->getVisibleChildren().size();
        int idMax = 0;
        for (const QString &oldDevice : oldDevices) {
            int idVisible = m_root->visibleLocation(oldDevice);
            if (idVisible < idMin)
                idMin = idVisible;
            if (idVisible > idMax)
                idMax = idVisible;
        }

        beginRemoveRows(index, idMin, idMax);
        for (const QString &oldDevice : oldDevices) {
            MTPFileNode *oldNode = m_root->getChildren().take(oldDevice);
            m_nodesToRemove.append(oldNode);
            m_root->getVisibleChildren().removeAll(oldDevice);
        }
        endRemoveRows();
    }

    //Add new nodes
    if (!newDevices.isEmpty()) {
        int firstRow = m_root->getChildren().size();
        beginInsertRows(index, firstRow, firstRow + newDevices.size() - 1);
        for (int id : newDevices) {
            const QString &newDevice = devices[id];
            MTPFileNode *child = new MTPFileNode(newDevice, MTPFileNode::Type::DEVICE, 0, QString(), m_iconProvider.icon(QFileIconProvider::Computer), m_root);
            if (child) {
                m_root->getChildren().insert(MTPFileNodePathKey(newDevice), child);
                m_root->getVisibleChildren().append(newDevice);
            }
        }
        endInsertRows();
    }

    m_root->setPopulated();
    sortChildren(*m_root);
}

void MTPFileModel::populate(const NodeContainer &container)
{
    cleanNodes();

    int row = container.m_node->getParent()->visibleLocation(container.m_node->getName());
    QModelIndex index = createIndex(row, 0, container.m_node);
    QStringList oldChildren;
    QVector<int> newChildren;

    for (auto &childIter : container.m_node->getChildren()) {
        oldChildren.append(childIter->getName());
    }

    for (int i = 0; i < container.m_content->size(); i++) {
        QString &newName = (*container.m_content)[i].m_name;
        if (oldChildren.removeAll(newName) == 0)
            newChildren.append(i);
    }

    //Remove old nodes
    if (!oldChildren.isEmpty()) {
        int idMin = container.m_node->getVisibleChildren().size();
        int idMax = 0;
        for (const QString &oldName : oldChildren) {
            int idVisible = container.m_node->visibleLocation(oldName);
            if (idVisible < idMin)
                idMin = idVisible;
            if (idVisible > idMax)
                idMax = idVisible;
        }

        beginRemoveRows(index, idMin, idMax);
        for (const QString &oldName : oldChildren) {
            MTPFileNode *oldNode = container.m_node->getChildren().take(oldName);
            m_nodesToRemove.append(oldNode);
            container.m_node->getVisibleChildren().removeAll(oldName);
        }
        endRemoveRows();
    }

    //Add new nodes
    if (!newChildren.isEmpty()) {
        int firstRow = container.m_node->getChildren().size();
        beginInsertRows(index, firstRow, firstRow + newChildren.size() - 1);
        for (int itemId : newChildren) {
            WPDManager::Item &item = (*container.m_content)[itemId];
            MTPFileNode *child = new MTPFileNode(item.m_name, TypeConversion(item.m_type), item.m_size, FormatDate(item.m_date), m_iconProvider.icon(IconConversion(item.m_type)), container.m_node);
            if (child) {
                container.m_node->getChildren().insert(MTPFileNodePathKey(item.m_name), child);
                container.m_node->getVisibleChildren().append(item.m_name);
            }
        }
        endInsertRows();
    }

    container.m_node->setPopulated();
    sortChildren(*container.m_node);

    //SetRootPath management
    if (!m_currentStack.isEmpty()) {
        const QString &fileName = m_currentStack.top();
        if (container.m_node->getName() == fileName) {
            m_currentStack.pop();
            if (m_currentStack.isEmpty())
                emit currentPathChanged(index);
            else if (m_fetcher) {
                MTPFileNode *child = container.m_node->getChildren().value(m_currentStack.top());
                if (child)
                    m_fetcher->addToFetch(child);
                else
                    m_currentStack.clear();
            }
        }
    }

    delete container.m_content;
}

void MTPFileModel::sortChildren(MTPFileNode & node)
{
    emit layoutAboutToBeChanged();

    //Optimised bubble sort by folder/file and QString order
    QList<QString> &visibleChildren = node.getVisibleChildren();
    QVector<bool> isFile(visibleChildren.size(), false);
    for (int i = 0; i < visibleChildren.size(); i++) {
        MTPFileNode *childNode = node.getChildren().value(visibleChildren[i]);
        if (childNode && !childNode->isDir())
            isFile[i] = true;
    }

    for (int i = 1; i < visibleChildren.size(); i++) {
        bool sorted = true;
        for (int j = visibleChildren.size() - 1; j >= i; j--) {
            if (isFile[j - 1] != isFile[j] ? isFile[j - 1] > isFile[j] : visibleChildren[j - 1].compare(visibleChildren[j], Qt::CaseInsensitive) > 0) {
                isFile.swapItemsAt(j - 1, j);
                visibleChildren.swapItemsAt(j - 1, j);
                sorted = false;
            }
        }
        if (sorted)
            break;
    }

    emit layoutChanged();
}

QModelIndex MTPFileModel::parent(MTPFileNode & child) const
{
    MTPFileNode *parentNode = child.getParent();
    if (parentNode == nullptr || parentNode == m_root)
        return QModelIndex();

    MTPFileNode *grandParentNode = parentNode->getParent();
    int visualRow = grandParentNode->visibleLocation(grandParentNode->getChildren().value(parentNode->getName())->getName());
    if (visualRow == -1)
        return QModelIndex();

    return createIndex(visualRow, 0, parentNode);
}

MTPFileNode * MTPFileModel::indexNode(const QModelIndex & index) const
{
    if (!index.isValid())
        return m_root;

    MTPFileNode *indexNode = static_cast<MTPFileNode *>(index.internalPointer());
    return indexNode;
}

MTPFileNode * MTPFileModel::pathNode(const QString & path) const
{
    QStringList splittedPath = path.split("/");
    MTPFileNode *node = m_root;
    int index = 0;
    while (node && index < splittedPath.size()) {
        node = node->getChildren().value(splittedPath[index]);
        index++;
    }
    return node;
}

void MTPFileModel::refreshPath(const QString & path) const
{
    int index = path.lastIndexOf("/");
    MTPFileNode *node = pathNode(path.left(index));
    if (node) {
        node->setUpdate();
        if (m_fetcher)
            m_fetcher->addToFetch(node);
    }
}

void MTPFileModel::cleanNodes()
{
    for (MTPFileNode *node : m_nodesToRemove) {
        if (node)
            delete node;
        node = nullptr;
    }
    m_nodesToRemove.clear();
}

MTPFileNode::Type MTPFileModel::TypeConversion(WPDManager::ItemType type)
{
    switch (type) {
    case WPDManager::ItemType::DRIVE:
        return MTPFileNode::Type::DRIVE;
    case WPDManager::ItemType::FOLDER:
        return MTPFileNode::Type::FOLDER;
    case WPDManager::ItemType::FILE:
        return MTPFileNode::Type::FILE;
    default:
        return MTPFileNode::Type::UNKNOWN;
    }
}

QFileIconProvider::IconType MTPFileModel::IconConversion(WPDManager::ItemType type)
{
    switch (type) {
    case WPDManager::ItemType::DRIVE:
        return QFileIconProvider::Drive;
    case WPDManager::ItemType::FOLDER:
        return QFileIconProvider::Folder;
    case WPDManager::ItemType::FILE:
        return QFileIconProvider::File;
    default:
        return QFileIconProvider::File;
    }
}

QString MTPFileModel::FormatDate(QString date)
{
    QString newDate = QString();

    if (date.size() == 23) {
        QStringList YMD = date.first(10).split('/');
        QStringList HM = date.mid(11, 5).split(':');

        if (YMD.size() == 3 && HM.size() == 2) {
            newDate += YMD[2] + "/"; //day
            newDate += YMD[1] + "/"; //month
            newDate += YMD[0] + " "; //year

            newDate += HM[0] + ":";  //hour
            newDate += HM[1];        //minute
        }
    }

    return newDate;
}

QString MTPFileModel::FormatSize(int bytes)
{
    if (bytes <= 0)
        return QString();
    
    return QLocale::system().formattedDataSize(bytes);
}

MTPFileModel::Observer::Observer(MTPFileModel &model) :
    m_model(model)
{
}

MTPFileModel::Observer::~Observer()
{
}

void MTPFileModel::Observer::addDevice(const QString & device)
{
    if (m_model.m_fetcher)
        m_model.m_fetcher->updateDevices();
}

void MTPFileModel::Observer::removeDevice(const QString & device)
{
    if (m_model.m_fetcher)
        m_model.m_fetcher->updateDevices();
}

void MTPFileModel::Observer::addItem(const QString & path)
{
    m_model.refreshPath(path);
}

void MTPFileModel::Observer::updateItem(const QString & path)
{
    m_model.refreshPath(path);
}

void MTPFileModel::Observer::removeItem(const QString & path)
{
    m_model.refreshPath(path);
}
