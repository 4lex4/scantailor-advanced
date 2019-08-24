// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_DEBUGIMAGESIMPL_H_
#define SCANTAILOR_CORE_DEBUGIMAGESIMPL_H_

#include <imageproc/DebugImages.h>
#include <QString>
#include <boost/function.hpp>
#include <deque>
#include "AutoRemovingFile.h"
#include "intrusive_ptr.h"
#include "ref_countable.h"

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImagesImpl : public DebugImages {
 public:
  void add(const QImage& image,
           const QString& label,
           const boost::function<QWidget*(const QImage&)>& imageViewFactory
           = boost::function<QWidget*(const QImage&)>()) override;

  void add(const imageproc::BinaryImage& image,
           const QString& label,
           const boost::function<QWidget*(const QImage&)>& imageViewFactory
           = boost::function<QWidget*(const QImage&)>()) override;

  bool empty() const override { return m_sequence.empty(); }

  /**
   * \brief Removes and returns the first item in the sequence.
   *
   * The label and viewer widget factory (that may not be bound)
   * are returned by taking pointers to them as arguments.
   * Returns a null AutoRemovingFile if image sequence is empty.
   */
  AutoRemovingFile retrieveNext(QString* label = nullptr,
                                boost::function<QWidget*(const QImage&)>* imageViewFactory = nullptr) override;

 private:
  struct Item : public ref_countable {
    AutoRemovingFile file;
    QString label;
    boost::function<QWidget*(const QImage&)> imageViewFactory;

    Item(AutoRemovingFile f, const QString& l, const boost::function<QWidget*(const QImage&)>& imf)
        : file(f), label(l), imageViewFactory(imf) {}
  };

  std::deque<intrusive_ptr<Item>> m_sequence;
};


#endif  // ifndef SCANTAILOR_CORE_DEBUGIMAGESIMPL_H_
