// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OUTPUT_OUTPUT_FILE_PARAMS_H_
#define OUTPUT_OUTPUT_FILE_PARAMS_H_

#include <QtGlobal>
#include <ctime>

class QDomDocument;
class QDomElement;
class QFileInfo;

namespace output {
/**
 * \brief Parameters of the output file used to determine if it has changed.
 */
class OutputFileParams {
 public:
  OutputFileParams();

  explicit OutputFileParams(const QFileInfo& file_info);

  explicit OutputFileParams(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  bool isValid() const;

  /**
   * \brief Returns true if it's likely we have two identical files.
   */
  bool matches(const OutputFileParams& other) const;

 private:
  qint64 m_size;
  time_t m_modifiedTime;
};
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUT_FILE_PARAMS_H_
