// Copyright (C) 2019  Joseph Artsimovich <joseph.artsimovich@gmail.com>, 4lex4 <4lex49@zoho.com>
// Use of this source code is governed by the GNU GPLv3 license that can be found in the LICENSE file.

#include "ProjectGeneratorFromDir.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QImageReader>
#include <QTextStream>
#include <QtXml>

#include <core/ImageMetadataLoader.h>
#include <core/ImageMetadata.h>

#include "version.h"

namespace {

const int DEFAULT_DPI = 300;
const char* IMAGE_FILTERS[] = {"*.tif", "*.tiff", "*.png", "*.jpg", "*.jpeg", "*.bmp"};

struct ImageEntry {
  QString filePath;
  QString fileName;
  int width = 0;
  int height = 0;
  int dpiX = DEFAULT_DPI;
  int dpiY = DEFAULT_DPI;
};

QList<ImageEntry> discoverImages(const QString& dirPath) {
  QDir dir(dirPath);
  if (!dir.exists()) {
    return QList<ImageEntry>();
  }
  QStringList names;
  for (const char* filter : IMAGE_FILTERS) {
    names.append(dir.entryList(QStringList() << QString::fromUtf8(filter), QDir::Files, QDir::Name));
  }
  names.removeDuplicates();
  names.sort(Qt::CaseInsensitive);

  QList<ImageEntry> entries;
  for (const QString& name : names) {
    const QString path = dir.absoluteFilePath(name);
    QImageReader reader(path);
    QSize size = reader.size();
    QImage img;
    if (reader.read(&img)) {
      if (!size.isValid()) {
        size = img.size();
      }
      const int dpmX = img.dotsPerMeterX();
      const int dpmY = img.dotsPerMeterY();
      if (dpmX > 0 && dpmY > 0) {
        ImageEntry e;
        e.filePath = path;
        e.fileName = name;
        e.width = size.width();
        e.height = size.height();
        e.dpiX = qRound(dpmX / 39.370078740157);
        e.dpiY = qRound(dpmY / 39.370078740157);
        if (e.dpiX < 1) e.dpiX = DEFAULT_DPI;
        if (e.dpiY < 1) e.dpiY = DEFAULT_DPI;
        entries.append(e);
        continue;
      }
    }
    if (!size.isValid() || size.isEmpty()) {
      const QString suffix = QFileInfo(name).suffix().toLower();
      if (suffix == QLatin1String("tif") || suffix == QLatin1String("tiff")) {
        QFile file(path);
        if (file.open(QIODevice::ReadOnly)) {
          ImageMetadata firstMeta;
          bool gotFirst = false;
          const auto status = ImageMetadataLoader::load(file, [&firstMeta, &gotFirst](const ImageMetadata& m) {
            if (!gotFirst) {
              firstMeta = m;
              gotFirst = true;
            }
          });
          if (status == ImageMetadataLoader::LOADED && gotFirst) {
            const QSize ms = firstMeta.size();
            if (ms.isValid() && !ms.isEmpty()) {
              ImageEntry e;
              e.filePath = path;
              e.fileName = name;
              e.width = ms.width();
              e.height = ms.height();
              const Dpi& dpi = firstMeta.dpi();
              if (dpi.isNull()) {
                e.dpiX = DEFAULT_DPI;
                e.dpiY = DEFAULT_DPI;
              } else {
                e.dpiX = dpi.horizontal();
                e.dpiY = dpi.vertical();
                if (e.dpiX < 1) e.dpiX = DEFAULT_DPI;
                if (e.dpiY < 1) e.dpiY = DEFAULT_DPI;
              }
              entries.append(e);
            }
          }
        }
      }
      continue;
    }
    ImageEntry e;
    e.filePath = path;
    e.fileName = name;
    e.width = size.width();
    e.height = size.height();
    e.dpiX = DEFAULT_DPI;
    e.dpiY = DEFAULT_DPI;
    entries.append(e);
  }
  return entries;
}

static QDomElement el(QDomDocument& doc, const QString& tag, QDomElement* parent, const QHash<QString, QString>& attrs) {
  QDomElement e = doc.createElement(tag);
  for (auto it = attrs.begin(); it != attrs.end(); ++it) {
    if (!it.value().isEmpty()) {
      e.setAttribute(it.key(), it.value());
    }
  }
  if (parent) {
    parent->appendChild(e);
  }
  return e;
}

static QString sizeMm(int pixels, int dpi) {
  if (dpi < 1) {
    dpi = DEFAULT_DPI;
  }
  return QString::number(pixels * 25.4 / dpi, 'f', 2);
}

}  // namespace

QString ProjectGeneratorFromDir::generate(const QString& imagesDir, const QString& outputDirName) {
  const QDir dir(imagesDir);
  const QString absDirPath = dir.absolutePath();
  const QList<ImageEntry> entries = discoverImages(absDirPath);
  if (entries.isEmpty()) {
    return QString();
  }

  QDomDocument doc;
  QDomElement root = doc.createElement("project");
  doc.appendChild(root);
  root.setAttribute("version", QString::number(PROJECT_VERSION));
  root.setAttribute("outputDirectory", outputDirName.isEmpty() ? QStringLiteral("out") : outputDirName);
  root.setAttribute("layoutDirection", "LTR");

  QDomElement dirsEl = el(doc, "directories", &root, QHash<QString, QString>());
  el(doc, "directory", &dirsEl, {{"id", "1"}, {"path", "."}});

  QDomElement filesEl = el(doc, "files", &root, QHash<QString, QString>());
  int fileId = 1;
  for (const ImageEntry& e : entries) {
    el(doc, "file", &filesEl, {{"id", QString::number(fileId)}, {"dirId", "1"}, {"name", e.fileName}});
    fileId++;
  }

  QDomElement imagesEl = el(doc, "images", &root, QHash<QString, QString>());
  int imageId = 1;
  for (const ImageEntry& e : entries) {
    QDomElement imgEl = el(doc, "image", &imagesEl,
                           {{"id", QString::number(imageId)},
                            {"subPages", "1"},
                            {"fileId", QString::number(imageId)},
                            {"fileImage", "1"}});
    el(doc, "size", &imgEl, {{"width", QString::number(e.width)}, {"height", QString::number(e.height)}});
    el(doc, "dpi", &imgEl, {{"horizontal", QString::number(e.dpiX)}, {"vertical", QString::number(e.dpiY)}});
    imageId++;
  }

  QDomElement pagesEl = el(doc, "pages", &root, QHash<QString, QString>());
  for (int i = 1; i <= entries.size(); i++) {
    el(doc, "page", &pagesEl,
       {{"id", QString::number(i)},
        {"imageId", QString::number(i)},
        {"subPage", "single"},
        {"selected", i == 1 ? "selected" : ""}});
  }

  QDomElement disambigEl = el(doc, "file-name-disambiguation", &root, QHash<QString, QString>());
  for (int i = 1; i <= entries.size(); i++) {
    el(doc, "mapping", &disambigEl, {{"file", QString::number(i)}, {"label", QString::number(i)}});
  }

  QDomElement filtersEl = el(doc, "filters", &root, QHash<QString, QString>());

  QDomElement fixOrientEl = el(doc, "fix-orientation", &filtersEl, QHash<QString, QString>());
  for (int i = 1; i <= entries.size(); i++) {
    QDomElement imgEl = el(doc, "image", &fixOrientEl, {{"id", QString::number(i)}});
    el(doc, "rotation", &imgEl, {{"degrees", "0"}});
  }
  el(doc, "image-settings", &fixOrientEl, QHash<QString, QString>());

  el(doc, "page-split", &filtersEl, {{"defaultLayoutType", "single-uncut"}});

  QDomElement deskewEl = el(doc, "deskew", &filtersEl, QHash<QString, QString>());
  for (int i = 0; i < entries.size(); i++) {
    const ImageEntry& e = entries.at(i);
    int pid = i + 1;
    QDomElement pageEl = el(doc, "page", &deskewEl, {{"id", QString::number(pid)}});
    QDomElement paramsEl = el(doc, "params", &pageEl, {{"mode", "auto"}, {"angle", "0"}, {"oblique", "0"}});
    QDomElement depsEl = el(doc, "dependencies", &paramsEl, QHash<QString, QString>());
    el(doc, "rotation", &depsEl, {{"degrees", "0"}});
    QDomElement outlineEl = el(doc, "page-outline", &depsEl, QHash<QString, QString>());
    el(doc, "point", &outlineEl, {{"x", "0"}, {"y", "0"}});
    el(doc, "point", &outlineEl, {{"x", QString::number(e.width)}, {"y", "0"}});
    el(doc, "point", &outlineEl, {{"x", QString::number(e.width)}, {"y", QString::number(e.height)}});
    el(doc, "point", &outlineEl, {{"x", "0"}, {"y", QString::number(e.height)}});
  }
  el(doc, "image-settings", &deskewEl, QHash<QString, QString>());

  QDomElement selectEl = el(doc, "select-content", &filtersEl, {{"pageDetectionTolerance", "0.1"}});
  el(doc, "page-detection-box", &selectEl, {{"width", "0"}, {"height", "0"}});
  for (int i = 0; i < entries.size(); i++) {
    const ImageEntry& e = entries.at(i);
    int pid = i + 1;
    QString w = QString::number(e.width);
    QString h = QString::number(e.height);
    QString wMm = sizeMm(e.width, e.dpiX);
    QString hMm = sizeMm(e.height, e.dpiY);
    QDomElement pageEl = el(doc, "page", &selectEl, {{"id", QString::number(pid)}});
    QDomElement paramsEl = el(doc, "params", &pageEl,
                              {{"contentDetectionMode", "auto"},
                               {"pageDetectionMode", "disabled"},
                               {"fineTuneCorners", "0"}});
    el(doc, "content-rect", &paramsEl, {{"x", "0"}, {"y", "0"}, {"width", w}, {"height", h}});
    el(doc, "page-rect", &paramsEl, {{"x", "0"}, {"y", "0"}, {"width", w}, {"height", h}});
    el(doc, "content-size-mm", &paramsEl, {{"width", wMm}, {"height", hMm}});
    QDomElement depsEl = el(doc, "dependencies", &paramsEl, QHash<QString, QString>());
    el(doc, "rotation", &depsEl, {{"degrees", "0"}});
  }

  QDomElement layoutEl = el(doc, "page-layout", &filtersEl, {{"showMiddleRect", "0"}});
  for (int i = 0; i < entries.size(); i++) {
    const ImageEntry& e = entries.at(i);
    int pid = i + 1;
    QString w = QString::number(e.width);
    QString h = QString::number(e.height);
    QString wMm = sizeMm(e.width, e.dpiX);
    QString hMm = sizeMm(e.height, e.dpiY);
    QDomElement pageEl = el(doc, "page", &layoutEl, {{"id", QString::number(pid)}});
    QDomElement paramsEl = el(doc, "params", &pageEl, {{"autoMargins", "1"}});
    el(doc, "hardMarginsMM", &paramsEl, {{"left", "0"}, {"top", "0"}, {"right", "0"}, {"bottom", "0"}});
    el(doc, "contentRect", &paramsEl, {{"x", "0"}, {"y", "0"}, {"width", w}, {"height", h}});
    el(doc, "pageRect", &paramsEl, {{"x", "0"}, {"y", "0"}, {"width", w}, {"height", h}});
    el(doc, "contentSizeMM", &paramsEl, {{"width", wMm}, {"height", hMm}});
    el(doc, "alignment", &paramsEl, {{"vert", "top"}, {"hor", "left"}});
  }

  QDomElement outputEl = el(doc, "output", &filtersEl, QHash<QString, QString>());
  for (int i = 0; i < entries.size(); i++) {
    const ImageEntry& e = entries.at(i);
    int pid = i + 1;
    QString dpiStr = QString::number(qMax(e.dpiX, e.dpiY));
    if (dpiStr.toInt() < 1) {
      dpiStr = QString::number(DEFAULT_DPI);
    }
    QDomElement pageEl = el(doc, "page", &outputEl, {{"id", QString::number(pid)}});
    el(doc, "zones", &pageEl, QHash<QString, QString>());
    el(doc, "fill-zones", &pageEl, QHash<QString, QString>());
    QDomElement paramsEl =
        el(doc, "params", &pageEl, {{"depthPerception", "1"}, {"despeckleLevel", "1"}, {"blackOnWhite", "1"}});
    el(doc, "distortion-model", &paramsEl, QHash<QString, QString>());
    el(doc, "picture-shape-options", &paramsEl,
       {{"pictureShape", "free"}, {"sensitivity", "100"}, {"higherSearchSensitivity", "0"}});
    el(doc, "dewarping-options", &paramsEl, {{"mode", "off"}, {"postDeskew", "1"}, {"postDeskewAngle", "0"}});
    el(doc, "dpi", &paramsEl, {{"horizontal", dpiStr}, {"vertical", dpiStr}});
    QDomElement cpEl = el(doc, "color-params", &paramsEl, {{"colorMode", "bw"}});
    QDomElement cgEl = el(doc, "color-or-grayscale", &cpEl,
                          {{"fillOffcut", "1"},
                           {"fillMargins", "1"},
                           {"normalizeIlluminationColor", "0"},
                           {"fillingColor", "background"},
                           {"wienerCoef", "0"},
                           {"wienerWinSize", "5"}});
    el(doc, "posterization-options", &cgEl,
       {{"enabled", "0"}, {"level", "4"}, {"normalizationEnabled", "0"}, {"forceBlackAndWhite", "1"}});
    QDomElement bwEl = el(doc, "bw", &cpEl,
                          {{"thresholdAdj", "0"},
                           {"savitzkyGolaySmoothing", "1"},
                           {"morphologicalSmoothing", "1"},
                           {"normalizeIlluminationBW", "1"},
                           {"windowSize", "200"},
                           {"binarizationMethod", "otsu"}});
    el(doc, "color-segmenter-options", &bwEl,
       {{"enabled", "0"},
        {"noiseReduction", "7"},
        {"redThresholdAdjustment", "0"},
        {"greenThresholdAdjustment", "0"},
        {"blueThresholdAdjustment", "0"}});
    el(doc, "splitting", &paramsEl, {{"splitOutput", "0"}, {"splittingMode", "bw"}, {"originalBackground", "0"}});
    el(doc, "processing-params", &pageEl, {{"autoZonesFound", "0"}});
  }

  const QString projectPath = dir.absoluteFilePath(QStringLiteral("project.ScanTailor"));
  QFile file(projectPath);
  if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    return QString();
  }
  QTextStream strm(&file);
  doc.save(strm, 2);
  file.close();
  return projectPath;
}
