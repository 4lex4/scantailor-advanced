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
#include "OpenGLSupport.h"
#include <QSettings>
#include <QtWidgets/QMessageBox>

SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent) {
    ui.setupUi(this);

    QSettings settings;

    if (!OpenGLSupport::supported()) {
        ui.enableOpenglCb->setChecked(false);
        ui.enableOpenglCb->setEnabled(false);
        ui.openglDeviceLabel->setEnabled(false);
        ui.openglDeviceLabel->setText(tr("Your hardware / driver don't provide the necessary features"));
    } else {
        ui.enableOpenglCb->setChecked(
                settings.value("settings/enable_opengl", false).toBool()
        );
        QString const openglDevicePattern = ui.openglDeviceLabel->text();
        ui.openglDeviceLabel->setText(openglDevicePattern.arg(OpenGLSupport::deviceName()));
    }

    ui.colorSchemeBox->addItem(tr("Dark"));
    ui.colorSchemeBox->addItem(tr("Light"));
    QString val = settings.value("settings/color_scheme", "dark").toString();
    if (val == "light") {
        ui.colorSchemeBox->setCurrentIndex(1);
    } else {
        ui.colorSchemeBox->setCurrentIndex(0);
    }

    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
    ui.AutoSaveProject->setChecked(settings.value("settings/auto_save_project").toBool());
    connect(ui.AutoSaveProject, SIGNAL(toggled(bool)), this, SLOT(OnCheckAutoSaveProject(bool)));
    connect(
            ui.colorSchemeBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(onColorSchemeChanged(int))
    );
}

SettingsDialog::~SettingsDialog() {
}

void SettingsDialog::commitChanges() {
    QSettings settings;
    settings.setValue("settings/enable_opengl", ui.enableOpenglCb->isChecked());
}

void SettingsDialog::OnCheckAutoSaveProject(bool state) {
    QSettings settings;

    settings.setValue("settings/auto_save_project", state);

    emit AutoSaveProjectStateSignal(state);
}

void SettingsDialog::onColorSchemeChanged(int idx) {
    QSettings settings;

    if (idx == 0) {
        settings.setValue("settings/color_scheme", "dark");
    } else if (idx == 1) {
        settings.setValue("settings/color_scheme", "light");
    }

    QMessageBox::information(
            this, tr("Information"),
            tr("ScanTailor need to be restarted to apply the color scheme changes.")
    );
}
