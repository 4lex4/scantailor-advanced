
#ifndef SCANTAILOR_GUIDE_H
#define SCANTAILOR_GUIDE_H

#include <QDomElement>
#include <QtCore>

namespace page_layout {
class Guide {
 public:
  Guide();

  Guide(Qt::Orientation orientation, double position);

  Guide(const QLineF& line);

  explicit Guide(const QDomElement& el);

  QDomElement toXml(QDomDocument& doc, const QString& name) const;

  operator QLineF() const;

  Qt::Orientation getOrientation() const;

  double getPosition() const;

  void setPosition(double position);

 private:
  static Qt::Orientation lineOrientation(const QLineF& line);

  static QString orientationToString(Qt::Orientation orientation);

  static Qt::Orientation orientationFromString(const QString& str);

  Qt::Orientation m_orientation;
  double m_position;
};
}  // namespace page_layout

#endif  // ifndef SCANTAILOR_GUIDE_H