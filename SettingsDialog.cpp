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
#include <QSettings>

SettingsDialog::SettingsDialog(QWidget* parent)
        : QDialog(parent)
{
    ui.setupUi(this);

    QSettings settings;

    connect(ui.buttonBox, SIGNAL(accepted()), SLOT(commitChanges()));
    ui.AutoSaveProject->setChecked(settings.value("settings/auto_save_project").toBool());
    connect(ui.AutoSaveProject, SIGNAL(toggled(bool)), this, SLOT(OnCheckAutoSaveProject(bool)));
    ui.DontEqualizeIlluminationPicZones->setChecked(
            settings.value("settings/dont_equalize_illumination_pic_zones").toBool());
    connect(ui.DontEqualizeIlluminationPicZones, SIGNAL(toggled(bool)), this,
            SLOT(OnCheckDontEqualizeIlluminationPicZones(bool)));
}

SettingsDialog::~SettingsDialog()
{
}

void
SettingsDialog::commitChanges()
{
    QSettings settings;
}

void
SettingsDialog::OnCheckAutoSaveProject(bool state)
{
    QSettings settings;

    settings.setValue("settings/auto_save_project", state);

    emit AutoSaveProjectStateSignal(state);
}

void
SettingsDialog::OnCheckDontEqualizeIlluminationPicZones(bool state)
{
    QSettings settings;

    settings.setValue("settings/dont_equalize_illumination_pic_zones", state);

    emit DontEqualizeIlluminationPicZonesSignal(state);
}