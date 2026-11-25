/*
    Copyright 2016-2026 melonDS team

    This file is part of melonDS.

    melonDS is free software: you can redistribute it and/or modify it under
    the terms of the GNU General Public License as published by the Free
    Software Foundation, either version 3 of the License, or (at your option)
    any later version.

    melonDS is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
    FOR A PARTICULAR PURPOSE. See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with melonDS. If not, see http://www.gnu.org/licenses/.
*/

#include <QFileDialog>
#include <QtGlobal>

#include "types.h"
#include "Platform.h"
#include "Config.h"
#include "GPU.h"
#include "main.h"

#include "VideoSettingsDialog.h"
#include "ui_VideoSettingsDialog.h"


inline bool VideoSettingsDialog::UsesGL()
{
    auto& cfg = emuInstance->getGlobalConfig();
    return cfg.GetBool("Screen.UseGL") || (cfg.GetInt("3D.Renderer") != renderer3D_Software);
}

VideoSettingsDialog* VideoSettingsDialog::currentDlg = nullptr;

void VideoSettingsDialog::setEnabled()
{
    auto& cfg = emuInstance->getGlobalConfig();
    int renderer = cfg.GetInt("3D.Renderer");

    bool softwareRenderer = renderer == renderer3D_Software;
    ui->cbGLDisplay->setEnabled(softwareRenderer);
    ui->cbSoftwareThreaded->setEnabled(softwareRenderer);
    ui->cbxGLResolution->setEnabled(!softwareRenderer);
    ui->cbBetterPolygons->setEnabled(renderer == renderer3D_OpenGL);
    ui->cbStereo->setEnabled(!softwareRenderer);
    ui->cbStereoTop->setEnabled(!softwareRenderer);
    bool stereoEnabled = !softwareRenderer && (ui->cbStereo->isChecked() || ui->cbStereoTop->isChecked());
    ui->cbStereoHalfSBS->setEnabled(stereoEnabled);
    ui->sbStereoSeparation->setEnabled(stereoEnabled);
    ui->cbStereoAdaptive->setEnabled(stereoEnabled);
    ui->sbStereoAdaptiveTarget->setEnabled(stereoEnabled && ui->cbStereoAdaptive->isChecked());
    ui->sbStereoEdgeCrop->setEnabled(stereoEnabled);
    ui->cbStereoAutoCrop->setEnabled(stereoEnabled);
    ui->cbStereoInset2D->setEnabled(stereoEnabled);
    ui->cbStereoKeep2DInFront->setEnabled(stereoEnabled);
    ui->cbxComputeHiResCoords->setEnabled(renderer == renderer3D_OpenGLCompute);
    bool upscaleAvailable = renderer == renderer3D_OpenGLCompute;
    ui->cbxUpscaleFilter->setEnabled(upscaleAvailable);
    ui->cbHQ4x2DOnly->setEnabled(upscaleAvailable && ui->cbxUpscaleFilter->currentIndex() != 0);
}

VideoSettingsDialog::VideoSettingsDialog(QWidget* parent) : QDialog(parent), ui(new Ui::VideoSettingsDialog)
{
    ui->setupUi(this);
    setAttribute(Qt::WA_DeleteOnClose);

    emuInstance = ((MainWindow*)parent)->getEmuInstance();

    auto& cfg = emuInstance->getGlobalConfig();
    oldRenderer = cfg.GetInt("3D.Renderer");
    oldGLDisplay = cfg.GetBool("Screen.UseGL");
    oldVSync = cfg.GetBool("Screen.VSync");
    oldVSyncInterval = cfg.GetInt("Screen.VSyncInterval");
    oldSoftThreaded = cfg.GetBool("3D.Soft.Threaded");
    oldGLScale = cfg.GetInt("3D.GL.ScaleFactor");
    oldGLBetterPolygons = cfg.GetBool("3D.GL.BetterPolygons");
    oldStereo = cfg.GetBool("3D.GL.Stereo");
    oldStereoTop = cfg.GetBool("3D.GL.StereoTop");
    oldStereoHalfSBS = cfg.GetBool("3D.GL.StereoHalfSBS");
    oldStereoSeparation = cfg.GetInt("3D.GL.StereoSeparation");
    oldStereoAdaptive = cfg.GetBool("3D.GL.StereoAdaptive");
    oldStereoAdaptiveTarget = cfg.GetInt("3D.GL.StereoAdaptiveTarget");
    oldStereoEdgeCrop = cfg.GetInt("3D.GL.StereoEdgeCrop");
    oldStereoAutoCrop = cfg.GetBool("3D.GL.StereoAutoCrop");
    oldStereoInset2D = cfg.GetBool("3D.GL.StereoInset2D");
    oldStereoKeep2DInFront = cfg.GetBool("3D.GL.StereoKeep2DInFront");
    oldHiresCoordinates = cfg.GetBool("3D.GL.HiresCoordinates");
    oldHQ4x = cfg.GetBool("3D.GL.HQ4x");
    oldUpscaleFilter = cfg.GetInt("3D.GL.UpscaleFilter");
    oldHQ4x2DOnly = cfg.GetBool("3D.GL.HQ4x2DOnly");

    grp3DRenderer = new QButtonGroup(this);
    grp3DRenderer->addButton(ui->rb3DSoftware, renderer3D_Software);
    grp3DRenderer->addButton(ui->rb3DOpenGL,   renderer3D_OpenGL);
    grp3DRenderer->addButton(ui->rb3DCompute,  renderer3D_OpenGLCompute);
#if QT_VERSION < QT_VERSION_CHECK(5, 15, 0)
    connect(grp3DRenderer, SIGNAL(buttonClicked(int)), this, SLOT(onChange3DRenderer(int)));
#else
    connect(grp3DRenderer, SIGNAL(idClicked(int)), this, SLOT(onChange3DRenderer(int)));
#endif
    grp3DRenderer->button(oldRenderer)->setChecked(true);

#ifndef OGLRENDERER_ENABLED
    ui->rb3DOpenGL->setEnabled(false);
#endif

#ifdef __APPLE__
    ui->rb3DCompute->setEnabled(false);
#endif

    ui->cbGLDisplay->setChecked(oldGLDisplay != 0);

    ui->cbVSync->setChecked(oldVSync != 0);
    ui->sbVSyncInterval->setValue(oldVSyncInterval);

    ui->cbSoftwareThreaded->setChecked(oldSoftThreaded);

    for (int i = 1; i <= 16; i++)
        ui->cbxGLResolution->addItem(QString("%1x native (%2x%3)").arg(i).arg(256*i).arg(192*i));
    ui->cbxGLResolution->setCurrentIndex(oldGLScale-1);

    ui->cbBetterPolygons->setChecked(oldGLBetterPolygons != 0);
    ui->cbStereo->setChecked(oldStereo);
    ui->cbStereoTop->setChecked(oldStereoTop);
    ui->cbStereoHalfSBS->setChecked(oldStereoHalfSBS);
    ui->sbStereoSeparation->setValue(oldStereoSeparation);
    ui->cbStereoAdaptive->setChecked(oldStereoAdaptive);
    ui->sbStereoAdaptiveTarget->setValue(oldStereoAdaptiveTarget);
    ui->sbStereoEdgeCrop->setValue(oldStereoEdgeCrop);
    ui->cbStereoAutoCrop->setChecked(oldStereoAutoCrop);
    ui->cbStereoInset2D->setChecked(oldStereoInset2D);
    ui->cbStereoKeep2DInFront->setChecked(oldStereoKeep2DInFront);
    ui->cbxComputeHiResCoords->setChecked(oldHiresCoordinates != 0);
    // the legacy HQ4x checkbox setting maps to the HQ4x filter
    ui->cbxUpscaleFilter->setCurrentIndex((oldUpscaleFilter == 0 && oldHQ4x) ? 1 : oldUpscaleFilter);
    ui->cbHQ4x2DOnly->setChecked(oldHQ4x2DOnly);

    if (!oldVSync)
        ui->sbVSyncInterval->setEnabled(false);
    setVsyncControlEnable(UsesGL());

    setEnabled();
}

VideoSettingsDialog::~VideoSettingsDialog()
{
    delete ui;
}

void VideoSettingsDialog::on_VideoSettingsDialog_accepted()
{
    Config::Save();

    closeDlg();
}

void VideoSettingsDialog::on_VideoSettingsDialog_rejected()
{
    if (!((MainWindow*)parent())->getEmuInstance())
    {
        closeDlg();
        return;
    }

    bool old_gl = UsesGL();

    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.Renderer", oldRenderer);
    cfg.SetBool("Screen.UseGL", oldGLDisplay);
    cfg.SetBool("Screen.VSync", oldVSync);
    cfg.SetInt("Screen.VSyncInterval", oldVSyncInterval);
    cfg.SetBool("3D.Soft.Threaded", oldSoftThreaded);
    cfg.SetInt("3D.GL.ScaleFactor", oldGLScale);
    cfg.SetBool("3D.GL.BetterPolygons", oldGLBetterPolygons);
    cfg.SetBool("3D.GL.Stereo", oldStereo);
    cfg.SetBool("3D.GL.StereoTop", oldStereoTop);
    cfg.SetBool("3D.GL.StereoHalfSBS", oldStereoHalfSBS);
    cfg.SetInt("3D.GL.StereoSeparation", oldStereoSeparation);
    cfg.SetBool("3D.GL.StereoAdaptive", oldStereoAdaptive);
    cfg.SetInt("3D.GL.StereoAdaptiveTarget", oldStereoAdaptiveTarget);
    cfg.SetInt("3D.GL.StereoEdgeCrop", oldStereoEdgeCrop);
    cfg.SetBool("3D.GL.StereoAutoCrop", oldStereoAutoCrop);
    cfg.SetBool("3D.GL.StereoInset2D", oldStereoInset2D);
    cfg.SetBool("3D.GL.StereoKeep2DInFront", oldStereoKeep2DInFront);
    cfg.SetBool("3D.GL.HiresCoordinates", oldHiresCoordinates);
    cfg.SetBool("3D.GL.HQ4x", oldHQ4x);
    cfg.SetInt("3D.GL.UpscaleFilter", oldUpscaleFilter);
    cfg.SetBool("3D.GL.HQ4x2DOnly", oldHQ4x2DOnly);

    emit updateVideoSettings(old_gl != UsesGL());

    closeDlg();
}

void VideoSettingsDialog::setVsyncControlEnable(bool hasOGL)
{
    ui->cbVSync->setEnabled(hasOGL);
    ui->sbVSyncInterval->setEnabled(hasOGL);
}

void VideoSettingsDialog::onChange3DRenderer(int renderer)
{
    bool old_gl = UsesGL();

    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.Renderer", renderer);

    setEnabled();

    emit updateVideoSettings(old_gl != UsesGL());
}

void VideoSettingsDialog::on_cbGLDisplay_stateChanged(int state)
{
    bool old_gl = UsesGL();

    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("Screen.UseGL", (state != 0));

    setVsyncControlEnable(UsesGL());

    emit updateVideoSettings(old_gl != UsesGL());
}

void VideoSettingsDialog::on_cbVSync_stateChanged(int state)
{
    bool vsync = (state != 0);
    ui->sbVSyncInterval->setEnabled(vsync);

    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("Screen.VSync", vsync);

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_sbVSyncInterval_valueChanged(int val)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("Screen.VSyncInterval", val);

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbSoftwareThreaded_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.Soft.Threaded", (state != 0));

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbxGLResolution_currentIndexChanged(int idx)
{
    // prevent a spurious change
    if (ui->cbxGLResolution->count() < 16) return;

    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.GL.ScaleFactor", idx+1);

    setVsyncControlEnable(UsesGL());

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbBetterPolygons_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.BetterPolygons", (state != 0));

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereo_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.Stereo", state != 0);
    setEnabled();
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_sbStereoSeparation_valueChanged(int value)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.GL.StereoSeparation", value);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoAdaptive_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoAdaptive", state != 0);
    setEnabled();
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_sbStereoAdaptiveTarget_valueChanged(int value)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.GL.StereoAdaptiveTarget", value);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoKeep2DInFront_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoKeep2DInFront", state != 0);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_sbStereoEdgeCrop_valueChanged(int value)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.GL.StereoEdgeCrop", value);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoAutoCrop_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoAutoCrop", state != 0);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoInset2D_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoInset2D", state != 0);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoTop_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoTop", state != 0);
    setEnabled();
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbStereoHalfSBS_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.StereoHalfSBS", state != 0);
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbxComputeHiResCoords_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.HiresCoordinates", (state != 0));

    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbxUpscaleFilter_currentIndexChanged(int index)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetInt("3D.GL.UpscaleFilter", index);
    cfg.SetBool("3D.GL.HQ4x", false);
    setEnabled();
    emit updateVideoSettings(false);
}

void VideoSettingsDialog::on_cbHQ4x2DOnly_stateChanged(int state)
{
    auto& cfg = emuInstance->getGlobalConfig();
    cfg.SetBool("3D.GL.HQ4x2DOnly", state != 0);
    emit updateVideoSettings(false);
}
