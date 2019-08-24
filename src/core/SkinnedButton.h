// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#ifndef SCANTAILOR_CORE_SKINNEDBUTTON_H_
#define SCANTAILOR_CORE_SKINNEDBUTTON_H_

#include <QIcon>
#include <QToolButton>

/**
 * \brief A button represented by a set of images.
 *
 * The button won't have any borders or effects other than
 * those represented by set of 3 images:
 * \li The normal state image.
 * \li The hover state image.
 * \li The pressed state image.
 */
class SkinnedButton : public QToolButton {
 public:
  /**
   * \brief Construct a skinned button from a single image.
   *
   * \param file The path to a file or a Qt resource to the
   *        image representing the normal state of the button.
   * \param parent An optional parent widget.
   */
  explicit SkinnedButton(const QIcon& icon, QWidget* parent = nullptr);

  /**
   * \brief Construct a skinned button from a set of 3 images.
   *
   * \param normal_state_file The path to a file or a Qt resource
   *        to the image representing the normal state of the button.
   * \param hover_state_file The path to a file or a Qt resource
   *        to the image representing the hover state of the button.
   * \param pressed_state_file The path to a file or a Qt resource
   *        to the image representing the pressed state of the button.
   * \param parent An optional parent widget.
   *
   * Note that the sizes of all 3 images should be the same.
   */
  SkinnedButton(const QIcon& normalStateIcon,
                const QIcon& hoverStateIcon,
                const QIcon& pressedStateIcon,
                QWidget* parent = nullptr);

  /**
   * \brief Set the hover state image.
   *
   * \param file The path to a file or a Qt resource to the
   *        image representing the hover state of the button.
   *        This image should have the same size as the normal
   *        state image.
   */
  void setHoverImage(const QIcon& icon);

  /**
   * \brief Set the pressed state image.
   *
   * \param file The path to a file or a Qt resource to the
   *        image representing the pressed state of the button.
   *        This image should have the same size as the normal
   *        state image.
   */
  void setPressedImage(const QIcon& icon);

 protected:
  bool event(QEvent* e) override;

 private:
  void initialize();

  void initStyleSheet();

  void updateAppearance();

 private:
  QIcon m_normalStateIcon;
  QIcon m_hoverStateIcon;
  QIcon m_pressedStateIcon;
  bool m_hovered = false;
  bool m_pressed = false;
};


#endif  // ifndef SCANTAILOR_CORE_SKINNEDBUTTON_H_
