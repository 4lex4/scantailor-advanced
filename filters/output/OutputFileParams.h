/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

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

  const bool isValid() const;

  /**
   * \brief Returns true if it's likely we have two identical files.
   */
  bool matches(const OutputFileParams& other) const;

 private:
  qint64 m_size;
  time_t m_mtime;
};
}  // namespace output
#endif  // ifndef OUTPUT_OUTPUT_FILE_PARAMS_H_
