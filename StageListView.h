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

#ifndef STAGELISTVIEW_H_
#define STAGELISTVIEW_H_

#include <QPixmap>
#include <QTableView>
#include <vector>
#include "intrusive_ptr.h"

class StageSequence;

class StageListView : public QTableView {
  Q_OBJECT
 public:
  explicit StageListView(QWidget* parent);

  ~StageListView() override;

  void setStages(const intrusive_ptr<StageSequence>& stages);

  QSize sizeHint() const override { return m_sizeHint; }

 signals:

  void launchBatchProcessing();

 public slots:

  void setBatchProcessingPossible(bool possible);

  void setBatchProcessingInProgress(bool in_progress);

 protected slots:

  void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected) override;

 private slots:

  void ensureSelectedRowVisible();

 protected:
  void timerEvent(QTimerEvent* event) override;

 private:
  class Model;
  class LeftColDelegate;

  class RightColDelegate;

  void removeLaunchButton(int row);

  void placeLaunchButton(int row);

  void initiateBatchAnimationFrameRendering();

  void createBatchAnimationSequence(int square_side);

  void updateRowSpans();

  int selectedRow() const;

  QSize m_sizeHint;
  Model* m_pModel;
  LeftColDelegate* m_pFirstColDelegate;
  RightColDelegate* m_pSecondColDelegate;
  QWidget* m_pLaunchBtn;
  std::vector<QPixmap> m_batchAnimationPixmaps;
  int m_curBatchAnimationFrame;
  int m_timerId;
  bool m_batchProcessingPossible;
  bool m_batchProcessingInProgress;
};


#endif  // ifndef STAGELISTVIEW_H_
