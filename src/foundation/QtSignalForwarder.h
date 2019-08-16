// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef QT_SIGNAL_FORWARDER_H_
#define QT_SIGNAL_FORWARDER_H_

#include <QObject>
#include <boost/function.hpp>
#include "NonCopyable.h"

/**
 * \brief Connects to a Qt signal and forwards it to a boost::function.
 *
 * Useful when you need to bind additional parameters to a slot
 * at connection time.
 */
class QtSignalForwarder : public QObject {
  Q_OBJECT
  DECLARE_NON_COPYABLE(QtSignalForwarder)

 public:
  /**
   * \brief Constructor.
   *
   * \param emitter The object that will emit a signal.  The forwarder
   *        will become its child.
   * \param signal The signal specification in the form of SIGNAL(name()).
   *        Signals with arguments may be specified, but the arguments
   *        won't be forwarded.
   * \param slot A boost::function to forward the signal to.
   */
  QtSignalForwarder(QObject* emitter, const char* signal, const boost::function<void()>& slot);

 private slots:

  void handleSignal();

 private:
  boost::function<void()> m_slot;
};


#endif
