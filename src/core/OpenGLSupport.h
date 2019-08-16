// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef OPENGL_SUPPORT_H_
#define OPENGL_SUPPORT_H_

#include <QString>

class OpenGLSupport {
 public:
  /**
   * \brief Returns true if OpenGL support is present and provides the necessary features.
   */
  static bool supported();

  /**
   * \brief Returns the device name (GL_RENDERER) associated with the default OpenGL context.
   *
   * \note This function has to be called from the GUI thread only.
   */
  static QString deviceName();
};


#endif
