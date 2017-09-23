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

    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
    ui.AutoSaveProject->setChecked(settings.value("settings/auto_save_project").toBool());
    connect(ui.AutoSaveProject, SIGNAL(toggled(bool)), this, SLOT(OnCheckAutoSaveProject(bool)));
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