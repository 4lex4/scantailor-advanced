// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "FileNameDisambiguator.h"
#include <QDomDocument>
#include <QFileInfo>
#include <QMutex>
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/multi_index/composite_key.hpp>
#include <boost/multi_index/member.hpp>
#include <boost/multi_index/ordered_index.hpp>
#include <boost/multi_index/sequenced_index.hpp>
#include <boost/multi_index_container.hpp>
#include "AbstractRelinker.h"
#include "RelinkablePath.h"

using namespace boost::multi_index;

class FileNameDisambiguator::Impl {
 public:
  Impl();

  Impl(const QDomElement& disambiguatorEl, const boost::function<QString(const QString&)>& filePathUnpacker);

  QDomElement toXml(QDomDocument& doc,
                    const QString& name,
                    const boost::function<QString(const QString&)>& filePathPacker) const;

  int getLabel(const QString& filePath) const;

  int registerFile(const QString& filePath);

  void performRelinking(const AbstractRelinker& relinker);

 private:
  class ItemsByFilePathTag;
  class ItemsByFileNameLabelTag;

  class UnorderedItemsTag;

  struct Item {
    QString filePath;
    QString fileName;
    int label;

    Item(const QString& filePath, int lbl);

    Item(const QString& filePath, const QString& fileName, int lbl);
  };

  typedef multi_index_container<
      Item,
      indexed_by<
          ordered_unique<tag<ItemsByFilePathTag>, member<Item, QString, &Item::filePath>>,
          ordered_unique<tag<ItemsByFileNameLabelTag>,
                         composite_key<Item, member<Item, QString, &Item::fileName>, member<Item, int, &Item::label>>>,
          sequenced<tag<UnorderedItemsTag>>>>
      Container;

  typedef Container::index<ItemsByFilePathTag>::type ItemsByFilePath;
  typedef Container::index<ItemsByFileNameLabelTag>::type ItemsByFileNameLabel;
  typedef Container::index<UnorderedItemsTag>::type UnorderedItems;

  mutable QMutex m_mutex;
  Container m_items;
  ItemsByFilePath& m_itemsByFilePath;
  ItemsByFileNameLabel& m_itemsByFileNameLabel;
  UnorderedItems& m_unorderedItems;
};


/*====================== FileNameDisambiguator =========================*/

FileNameDisambiguator::FileNameDisambiguator() : m_impl(new Impl) {}

FileNameDisambiguator::FileNameDisambiguator(const QDomElement& disambiguatorEl)
    : m_impl(new Impl(disambiguatorEl, boost::lambda::_1)) {}

FileNameDisambiguator::FileNameDisambiguator(const QDomElement& disambiguatorEl,
                                             const boost::function<QString(const QString&)>& filePathUnpacker)
    : m_impl(new Impl(disambiguatorEl, filePathUnpacker)) {}

QDomElement FileNameDisambiguator::toXml(QDomDocument& doc, const QString& name) const {
  return m_impl->toXml(doc, name, boost::lambda::_1);
}

QDomElement FileNameDisambiguator::toXml(QDomDocument& doc,
                                         const QString& name,
                                         const boost::function<QString(const QString&)>& filePathPacker) const {
  return m_impl->toXml(doc, name, filePathPacker);
}

int FileNameDisambiguator::getLabel(const QString& filePath) const {
  return m_impl->getLabel(filePath);
}

int FileNameDisambiguator::registerFile(const QString& filePath) {
  return m_impl->registerFile(filePath);
}

void FileNameDisambiguator::performRelinking(const AbstractRelinker& relinker) {
  m_impl->performRelinking(relinker);
}

/*==================== FileNameDisambiguator::Impl ====================*/

FileNameDisambiguator::Impl::Impl()
    : m_items(),
      m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
      m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
      m_unorderedItems(m_items.get<UnorderedItemsTag>()) {}

FileNameDisambiguator::Impl::Impl(const QDomElement& disambiguatorEl,
                                  const boost::function<QString(const QString&)>& filePathUnpacker)
    : m_items(),
      m_itemsByFilePath(m_items.get<ItemsByFilePathTag>()),
      m_itemsByFileNameLabel(m_items.get<ItemsByFileNameLabelTag>()),
      m_unorderedItems(m_items.get<UnorderedItemsTag>()) {
  QDomNode node(disambiguatorEl.firstChild());
  for (; !node.isNull(); node = node.nextSibling()) {
    if (!node.isElement()) {
      continue;
    }
    if (node.nodeName() != "mapping") {
      continue;
    }
    const QDomElement fileEl(node.toElement());

    const QString filePathShorthand(fileEl.attribute("file"));
    const QString filePath = filePathUnpacker(filePathShorthand);
    if (filePath.isEmpty()) {
      // Unresolved shorthand - skipping this record.
      continue;
    }

    const int label = fileEl.attribute("label").toInt();
    m_items.insert(Item(filePath, label));
  }
}

QDomElement FileNameDisambiguator::Impl::toXml(QDomDocument& doc,
                                               const QString& name,
                                               const boost::function<QString(const QString&)>& filePathPacker) const {
  const QMutexLocker locker(&m_mutex);

  QDomElement el(doc.createElement(name));

  for (const Item& item : m_unorderedItems) {
    const QString filePathShorthand = filePathPacker(item.filePath);
    if (filePathShorthand.isEmpty()) {
      // Unrepresentable file path - skipping this record.
      continue;
    }

    QDomElement fileEl(doc.createElement("mapping"));
    fileEl.setAttribute("file", filePathShorthand);
    fileEl.setAttribute("label", item.label);
    el.appendChild(fileEl);
  }

  return el;
}

int FileNameDisambiguator::Impl::getLabel(const QString& filePath) const {
  const QMutexLocker locker(&m_mutex);

  const ItemsByFilePath::iterator fpIt(m_itemsByFilePath.find(filePath));
  if (fpIt != m_itemsByFilePath.end()) {
    return fpIt->label;
  }

  return 0;
}

int FileNameDisambiguator::Impl::registerFile(const QString& filePath) {
  const QMutexLocker locker(&m_mutex);

  const ItemsByFilePath::iterator fpIt(m_itemsByFilePath.find(filePath));
  if (fpIt != m_itemsByFilePath.end()) {
    return fpIt->label;
  }

  int label = 0;

  const QString fileName(QFileInfo(filePath).fileName());
  const ItemsByFileNameLabel::iterator fnIt(m_itemsByFileNameLabel.upper_bound(boost::make_tuple(fileName)));
  // If the item preceeding fnIt has the same file name,
  // the new file belongs to the same disambiguation group.
  if (fnIt != m_itemsByFileNameLabel.begin()) {
    ItemsByFileNameLabel::iterator prev(fnIt);
    --prev;
    if (prev->fileName == fileName) {
      label = prev->label + 1;
    }
  }  // Otherwise, label remains 0.
  const Item newItem(filePath, fileName, label);
  m_itemsByFileNameLabel.insert(fnIt, newItem);

  return label;
}

void FileNameDisambiguator::Impl::performRelinking(const AbstractRelinker& relinker) {
  const QMutexLocker locker(&m_mutex);
  Container newItems;

  for (const Item& item : m_unorderedItems) {
    const RelinkablePath oldPath(item.filePath, RelinkablePath::File);
    Item newItem(relinker.substitutionPathFor(oldPath), item.label);
    newItems.insert(newItem);
  }

  m_items.swap(newItems);
}

/*============================ Impl::Item =============================*/

FileNameDisambiguator::Impl::Item::Item(const QString& filePath, int lbl)
    : filePath(filePath), fileName(QFileInfo(filePath).fileName()), label(lbl) {}

FileNameDisambiguator::Impl::Item::Item(const QString& filePath, const QString& fileName, int lbl)
    : filePath(filePath), fileName(fileName), label(lbl) {}
