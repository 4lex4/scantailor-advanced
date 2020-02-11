// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_
#define SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_

#include <boost/function.hpp>
#include <memory>
#include <set>

#include "NonCopyable.h"

class AbstractRelinker;
class QString;
class QDomElement;
class QDomDocument;

/**
 * \brief Associates an integer label with each file path so that
 *        files with the same name but from different directories
 *        have distinctive labels.
 *
 * \note This class is thread-safe.
 */
class FileNameDisambiguator {
  DECLARE_NON_COPYABLE(FileNameDisambiguator)

 public:
  FileNameDisambiguator();

  /**
   * \brief Load disambiguation information from XML.
   */
  explicit FileNameDisambiguator(const QDomElement& disambiguatorEl);

  /**
   * \brief Load disambiguation information from XML with file path unpacking.
   *
   * Supplying a file path unpacker allows storing shorthands rather than
   * full paths in XML.  Unpacker is a functor taking a shorthand and
   * returning the full path.  If unpacker returns an empty string,
   * the record will be skipped.
   */
  FileNameDisambiguator(const QDomElement& disambiguatorEl,
                        const boost::function<QString(const QString&)>& filePathUnpacker);

  virtual ~FileNameDisambiguator();

  /**
   * \brief Serialize disambiguation information to XML.
   */
  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  /**
   * \brief Serialize disambiguation information to XML with file path packing.
   *
   * Supplying a file path packer allows storing shorthands rather than
   * full paths in XML.  Packer is a functor taking a full file path and
   * returning the corresponding shorthand.  If packer returns an empty string,
   * the record will be skipped.
   */
  QDomElement toXml(QDomDocument& doc,
                    const QString& name,
                    const boost::function<QString(const QString&)>& filePathPacker) const;

  int getLabel(const QString& filePath) const;

  int registerFile(const QString& filePath);

  void performRelinking(const AbstractRelinker& relinker);

 private:
  class Impl;

  std::unique_ptr<Impl> m_impl;
};


#endif  // ifndef SCANTAILOR_CORE_FILENAMEDISAMBIGUATOR_H_
