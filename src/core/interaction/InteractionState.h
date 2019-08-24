// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_INTERACTION_INTERACTIONSTATE_H_
#define SCANTAILOR_INTERACTION_INTERACTIONSTATE_H_

#include <QCursor>
#include <QString>
#include <boost/intrusive/list.hpp>
#include "NonCopyable.h"
#include "Proximity.h"

class Proximity;

class InteractionState {
  DECLARE_NON_COPYABLE(InteractionState)

 public:
  class Captor : public boost::intrusive::list_base_hook<boost::intrusive::link_mode<boost::intrusive::auto_unlink>> {
    friend class InteractionState;

   private:
    struct CopyHelper {
      Captor* captor;

      explicit CopyHelper(Captor* cap) : captor(cap) {}
    };

   public:
    Captor() = default;

    Captor(Captor& other) { swap_nodes(other); }

    explicit Captor(CopyHelper other) { swap_nodes(*other.captor); }

    Captor& operator=(Captor& other);

    Captor& operator=(CopyHelper other);

    explicit operator CopyHelper() { return CopyHelper(this); }

    void release() { unlink(); }

    const QCursor& proximityCursor() const { return m_proximityCursor; }

    void setProximityCursor(const QCursor& cursor) { m_proximityCursor = cursor; }

    const QCursor& interactionCursor() const { return m_interactionCursor; }

    void setInteractionCursor(const QCursor& cursor) { m_interactionCursor = cursor; }

    const QString& proximityStatusTip() const { return m_proximityStatusTip; }

    void setProximityStatusTip(const QString& tip) { m_proximityStatusTip = tip; }

    const QString& interactionStatusTip() const { return m_interactionStatusTip; }

    void setInteractionStatusTip(const QString& tip) { m_interactionStatusTip = tip; }

    const QString& interactionOrProximityStatusTip() const {
      return m_interactionStatusTip.isNull() ? m_proximityStatusTip : m_interactionStatusTip;
    }

   private:
    QCursor m_proximityCursor;
    QCursor m_interactionCursor;
    QString m_proximityStatusTip;
    QString m_interactionStatusTip;
  };


  InteractionState();

  void capture(Captor& captor);

  bool captured() const { return !m_captorList.empty(); }

  bool capturedBy(const Captor& captor) const;

  void resetProximity();

  void updateProximity(Captor& captor,
                       const Proximity& proximity,
                       int priority = 0,
                       Proximity proximityThreshold = Proximity());

  bool proximityLeader(const Captor& captor) const;

  const Proximity& proximityThreshold() const { return m_proximityThreshold; }

  QCursor cursor() const;

  QString statusTip() const;

  const QString& defaultStatusTip() const { return m_defaultStatusTip; }

  void setDefaultStatusTip(const QString& statusTip) { m_defaultStatusTip = statusTip; }

  bool redrawRequested() const { return m_redrawRequested; }

  void setRedrawRequested(bool requested) { m_redrawRequested = requested; }

 private:
  using CaptorList = boost::intrusive::list<Captor, boost::intrusive::constant_time_size<false>>;

  /**
   * Returns true if the provided proximity is better than the stored one.
   */
  bool betterProximity(const Proximity& proximity, int priority) const;

  QString m_defaultStatusTip;
  CaptorList m_captorList;
  CaptorList m_proximityLeader;
  Proximity m_bestProximity;
  Proximity m_proximityThreshold;
  int m_bestProximityPriority;
  bool m_redrawRequested;
};


#endif  // ifndef SCANTAILOR_INTERACTION_INTERACTIONSTATE_H_
