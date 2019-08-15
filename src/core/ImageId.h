/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#ifndef IMAGEID_H_
#define IMAGEID_H_

#include <foundation/Hashes.h>
#include <QString>

class QFileInfo;

class ImageId {
  // Member-wise copying is OK.
 public:
  ImageId() : m_filePath(), m_page(0) {}

  explicit ImageId(const QString& file_path, int page = 0);

  explicit ImageId(const QFileInfo& file_info, int page = 0);

  bool isNull() const { return m_filePath.isNull(); }

  const QString& filePath() const { return m_filePath; }

  void setFilePath(const QString& path) { m_filePath = path; }

  int page() const { return m_page; }

  void setPage(int page) { m_page = page; }

  int zeroBasedPage() const { return m_page > 0 ? m_page - 1 : 0; }

  bool isMultiPageFile() const { return m_page > 0; }

 private:
  QString m_filePath;

  /**
   * If zero, indicates the file is not multipage.
   * If above zero, indicates Nth page in a multipage file.
   */
  int m_page;
};


bool operator==(const ImageId& lhs, const ImageId& rhs);

bool operator!=(const ImageId& lhs, const ImageId& rhs);

bool operator<(const ImageId& lhs, const ImageId& rhs);

namespace std {
template <>
struct hash<ImageId> {
  size_t operator()(const ImageId& imageId) const noexcept {
    return (hashes::hash<QString>()(imageId.filePath()) ^ hash<int>()(imageId.page()) << 1);
  }
};
}  // namespace std

#endif  // ifndef IMAGEID_H_
