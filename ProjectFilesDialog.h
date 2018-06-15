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

#ifndef PROJECTFILESDIALOG_H_
#define PROJECTFILESDIALOG_H_

#include <QDialog>
#include <QSet>
#include <QString>
#include <memory>
#include <vector>
#include "ImageFileInfo.h"
#include "ui_ProjectFilesDialog.h"

class ProjectFilesDialog : public QDialog, private Ui::ProjectFilesDialog {
  Q_OBJECT
 public:
  explicit ProjectFilesDialog(QWidget* parent = nullptr);

  ~ProjectFilesDialog() override;

  QString inputDirectory() const;

  QString outputDirectory() const;

  std::vector<ImageFileInfo> inProjectFiles() const;

  bool isRtlLayout() const;

  bool isDpiFixingForced() const;

 private slots:

  static QString sanitizePath(const QString& path);

  void inpDirBrowse();

  void outDirBrowse();

  void inpDirEdited(const QString& text);

  void outDirEdited(const QString& text);

  void addToProject();

  void removeFromProject();

  void onOK();

 private:
  class Item;
  class FileList;
  class SortedFileList;
  class ItemVisualOrdering;

  void setInputDir(const QString& dir, bool auto_add_files = true);

  void setOutputDir(const QString& dir);

  void startLoadingMetadata();

  void timerEvent(QTimerEvent* event) override;

  void finishLoadingMetadata();

  QSet<QString> m_supportedExtensions;
  std::unique_ptr<FileList> m_ptrOffProjectFiles;
  std::unique_ptr<SortedFileList> m_ptrOffProjectFilesSorted;
  std::unique_ptr<FileList> m_ptrInProjectFiles;
  std::unique_ptr<SortedFileList> m_ptrInProjectFilesSorted;
  int m_loadTimerId;
  bool m_metadataLoadFailed;
  bool m_autoOutDir;
};


#endif  // ifndef PROJECTFILESDIALOG_H_
