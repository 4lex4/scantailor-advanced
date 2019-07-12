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

#include "SettingsDialog.h"
#include <core/ApplicationSettings.h>
#include <tiff.h>
#include <QtCore/QDir>
#include <QtWidgets/QMessageBox>
#include <cmath>
#include "Application.h"
#include "OpenGLSupport.h"

SettingsDialog::SettingsDialog(QWidget* parent) : QDialog(parent) {
  ui.setupUi(this);

  ApplicationSettings& settings = ApplicationSettings::getInstance();

  if (!OpenGLSupport::supported()) {
    ui.enableOpenglCb->setChecked(false);
    ui.enableOpenglCb->setEnabled(false);
    ui.openglDeviceLabel->setEnabled(false);
    ui.openglDeviceLabel->setText(tr("Your hardware / driver don't provide the necessary features"));
  } else {
    ui.enableOpenglCb->setChecked(settings.isOpenGlEnabled());
    const QString openglDevicePattern = ui.openglDeviceLabel->text();
    ui.openglDeviceLabel->setText(openglDevicePattern.arg(OpenGLSupport::deviceName()));
  }

  ui.colorSchemeBox->addItem(tr("Dark"), "dark");
  ui.colorSchemeBox->addItem(tr("Light"), "light");
  ui.colorSchemeBox->addItem(tr("Native"), "native");
  ui.colorSchemeBox->setCurrentIndex(ui.colorSchemeBox->findData(settings.getColorScheme()));
  connect(ui.colorSchemeBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [this](int) {
    QMessageBox::information(this, tr("Information"),
                             tr("ScanTailor need to be restarted to apply the color scheme changes."));
  });

  ui.tiffCompressionBWBox->addItem(tr("None"), COMPRESSION_NONE);
  ui.tiffCompressionBWBox->addItem(tr("LZW"), COMPRESSION_LZW);
  ui.tiffCompressionBWBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
  ui.tiffCompressionBWBox->addItem(tr("CCITT G4"), COMPRESSION_CCITTFAX4);
  ui.tiffCompressionBWBox->setCurrentIndex(ui.tiffCompressionBWBox->findData(settings.getTiffBwCompression()));

  ui.tiffCompressionColorBox->addItem(tr("None"), COMPRESSION_NONE);
  ui.tiffCompressionColorBox->addItem(tr("LZW"), COMPRESSION_LZW);
  ui.tiffCompressionColorBox->addItem(tr("Deflate"), COMPRESSION_DEFLATE);
  ui.tiffCompressionColorBox->addItem(tr("JPEG"), COMPRESSION_JPEG);
  ui.tiffCompressionColorBox->setCurrentIndex(ui.tiffCompressionColorBox->findData(settings.getTiffColorCompression()));

  {
    auto* app = static_cast<Application*>(qApp);
    for (const QString& locale : app->getLanguagesList()) {
      QString languageName = QLocale::languageToString(QLocale(locale).language());
      ui.languageBox->addItem(languageName, locale);
    }
    ui.languageBox->setCurrentIndex(ui.languageBox->findData(app->getCurrentLocale()));
    ui.languageBox->setEnabled(ui.languageBox->count() > 1);
  }

  ui.blackOnWhiteDetectionCB->setChecked(settings.isBlackOnWhiteDetectionEnabled());
  ui.blackOnWhiteDetectionAtOutputCB->setEnabled(ui.blackOnWhiteDetectionCB->isChecked());
  ui.blackOnWhiteDetectionAtOutputCB->setChecked(settings.isBlackOnWhiteDetectionOutputEnabled());
  connect(ui.blackOnWhiteDetectionCB, SIGNAL(clicked(bool)), SLOT(blackOnWhiteDetectionToggled(bool)));

  ui.highlightDeviationCB->setChecked(settings.isHighlightDeviationEnabled());

  ui.deskewDeviationCoefSB->setValue(settings.getDeskewDeviationCoef());
  ui.deskewDeviationThresholdSB->setValue(settings.getDeskewDeviationThreshold());
  ui.selectContentDeviationCoefSB->setValue(settings.getSelectContentDeviationCoef());
  ui.selectContentDeviationThresholdSB->setValue(settings.getSelectContentDeviationThreshold());
  ui.marginsDeviationCoefSB->setValue(settings.getMarginsDeviationCoef());
  ui.marginsDeviationThresholdSB->setValue(settings.getMarginsDeviationThreshold());

  ui.autoSaveProjectCB->setChecked(settings.isAutoSaveProjectEnabled());

  ui.thumbnailQualitySB->setValue(settings.getThumbnailQuality().width());
  ui.thumbnailSizeSB->setValue(settings.getMaxLogicalThumbnailSize().toSize().width());

  ui.singleColumnThumbnailsCB->setChecked(settings.isSingleColumnThumbnailDisplayEnabled());

  connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
}

SettingsDialog::~SettingsDialog() = default;

void SettingsDialog::commitChanges() {
  ApplicationSettings& settings = ApplicationSettings::getInstance();

  settings.setOpenGlEnabled(ui.enableOpenglCb->isChecked());
  settings.setAutoSaveProjectEnabled(ui.autoSaveProjectCB->isChecked());
  settings.setHighlightDeviationEnabled(ui.highlightDeviationCB->isChecked());
  settings.setColorScheme(ui.colorSchemeBox->currentData().toString());

  settings.setTiffBwCompression(ui.tiffCompressionBWBox->currentData().toInt());
  settings.setTiffColorCompression(ui.tiffCompressionColorBox->currentData().toInt());
  settings.setLanguage(ui.languageBox->currentData().toString());

  settings.setDeskewDeviationCoef(ui.deskewDeviationCoefSB->value());
  settings.setDeskewDeviationThreshold(ui.deskewDeviationThresholdSB->value());
  settings.setSelectContentDeviationCoef(ui.selectContentDeviationCoefSB->value());
  settings.setSelectContentDeviationThreshold(ui.selectContentDeviationThresholdSB->value());
  settings.setMarginsDeviationCoef(ui.marginsDeviationCoefSB->value());
  settings.setMarginsDeviationThreshold(ui.marginsDeviationThresholdSB->value());

  settings.setBlackOnWhiteDetectionEnabled(ui.blackOnWhiteDetectionCB->isChecked());
  settings.setBlackOnWhiteDetectionOutputEnabled(ui.blackOnWhiteDetectionAtOutputCB->isChecked());

  {
    const int quality = ui.thumbnailQualitySB->value();
    settings.setThumbnailQuality(QSize(quality, quality));
  }
  {
    const double width = ui.thumbnailSizeSB->value();
    const double height = std::round((width * (16.0 / 25.0)) * 100) / 100;
    settings.setMaxLogicalThumbnailSize(QSizeF(width, height));
  }

  settings.setSingleColumnThumbnailDisplayEnabled(ui.singleColumnThumbnailsCB->isChecked());

  emit settingsChanged();
}

void SettingsDialog::blackOnWhiteDetectionToggled(bool checked) {
  ui.blackOnWhiteDetectionAtOutputCB->setEnabled(checked);
}
