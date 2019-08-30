// =================================================================== //
// Copyright (C) 2014-2019 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#include <libbsdf/Brdf/SampleSet.h>

#include <algorithm>

#include <libbsdf/Common/Log.h>
#include <libbsdf/Common/Utility.h>
#include <libbsdf/Common/SpectrumUtility.h>

using namespace lb;

SampleSet::SampleSet(int        numAngles0,
                     int        numAngles1,
                     int        numAngles2,
                     int        numAngles3,
                     ColorModel colorModel,
                     int        numWavelengths)
                     : equalIntervalAngles0_(false),
                       equalIntervalAngles1_(false),
                       equalIntervalAngles2_(false),
                       equalIntervalAngles3_(false),
                       oneSide_(false)
{
    assert(numAngles0 > 0 && numAngles1 > 0 && numAngles2 > 0 && numAngles3 > 0);

    resizeAngles(numAngles0, numAngles1, numAngles2, numAngles3);

    colorModel_ = colorModel;

    if (colorModel == SPECTRAL_MODEL) {
        resizeWavelengths(numWavelengths);
    }
    else if (colorModel == MONOCHROMATIC_MODEL) {
        resizeWavelengths(1);
        wavelengths_ = Arrayf::Zero(1);
    }
    else {
        resizeWavelengths(3);
        wavelengths_ = Arrayf::Zero(3);
    }
}

bool SampleSet::validate() const
{
    bool valid = true;

    // Spectra
    bool spectraValid = true;
    for (int i0 = 0; i0 < angles0_.size(); ++i0) {
    for (int i1 = 0; i1 < angles1_.size(); ++i1) {
    for (int i2 = 0; i2 < angles2_.size(); ++i2) {
    for (int i3 = 0; i3 < angles3_.size(); ++i3) {
        const Spectrum& sp = getSpectrum(i0, i1, i2, i3);
        
        if (!sp.allFinite()) {
            spectraValid = false;
            if (sp.hasNaN()) {
                lbWarn
                    << "[SampleSet::validate] The spectrum contains NaN values at ("
                    << i0 << ", " << i1 << ", " << i2 << ", " << i3 << ").";
            }
            else {
                lbWarn
                    << "[SampleSet::validate] The spectrum contains +/-INF values at ("
                    << i0 << ", " << i1 << ", " << i2 << ", " << i3 << ").";
            }
        }
    }}}}

    if (spectraValid) {
        lbInfo << "[SampleSet::validate] Spectra are valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] Invalid spectra are found.";
    }
    
    // Angles
    if (angles0_.allFinite()) {
        lbInfo << "[SampleSet::validate] The array of angle0 is valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] The invalid angle0(s) is found.";
    }

    if (angles1_.allFinite()) {
        lbInfo << "[SampleSet::validate] The array of angle1 is valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] The invalid angle1(s) is found.";
    }

    if (angles2_.allFinite()) {
        lbInfo << "[SampleSet::validate] The array of angle2 is valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] The invalid angle2(s) is found.";
    }

    if (angles3_.allFinite()) {
        lbInfo << "[SampleSet::validate] The array of angle3 is valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] The invalid angle3(s) is found.";
    }

    // Wavelengths
    if (wavelengths_.allFinite()) {
        lbInfo << "[SampleSet::validate] Wavelengths are valid.";
    }
    else {
        valid = false;
        lbWarn << "[SampleSet::validate] The invalid wavelength(s) is found.";
    }

    return valid;
}

void SampleSet::updateAngleAttributes()
{
    updateEqualIntervalAngles();
    updateOneSide();
}

void SampleSet::resizeAngles(int numAngles0,
                             int numAngles1,
                             int numAngles2,
                             int numAngles3)
{
    assert(numAngles0 > 0 && numAngles1 > 0 && numAngles2 > 0 && numAngles3 > 0);

    angles0_.resize(numAngles0);
    angles1_.resize(numAngles1);
    angles2_.resize(numAngles2);
    angles3_.resize(numAngles3);

    size_t numSamples = numAngles0 * numAngles1 * numAngles2 * numAngles3;
    spectra_.resize(numSamples);
}

void SampleSet::resizeWavelengths(int numWavelengths)
{
    assert(numWavelengths > 0);

    size_t numSamples = angles0_.size() * angles1_.size() * angles2_.size() * angles3_.size();

    for (size_t i = 0; i < numSamples; ++i) {
        spectra_.at(i) = Spectrum::Zero(numWavelengths);
    }

    wavelengths_.resize(numWavelengths);
}

void SampleSet::updateEqualIntervalAngles()
{
    equalIntervalAngles0_ = isEqualInterval(angles0_);
    equalIntervalAngles1_ = isEqualInterval(angles1_);
    equalIntervalAngles2_ = isEqualInterval(angles2_);
    equalIntervalAngles3_ = isEqualInterval(angles3_);

    lbInfo << "[SampleSet::updateEqualIntervalAngles] Angle0: " << equalIntervalAngles0_;
    lbInfo << "[SampleSet::updateEqualIntervalAngles] Angle1: " << equalIntervalAngles1_;
    lbInfo << "[SampleSet::updateEqualIntervalAngles] Angle2: " << equalIntervalAngles2_;
    lbInfo << "[SampleSet::updateEqualIntervalAngles] Angle3: " << equalIntervalAngles3_;
}

void SampleSet::updateOneSide()
{
    bool containing_0_PI = false;
    bool containing_PI_2PI = false;

    const float offset = EPSILON_F * 2.0f;

    for (int i = 0; i < angles3_.size(); ++i) {
        float angle = angles3_[i];

        if (angle > offset && angle < PI_F - offset * PI_F) {
            containing_0_PI = true;
        }

        if (angle > PI_F + offset * PI_F && angle < 2.0f * PI_F - offset * 2.0f * PI_F) {
            containing_PI_2PI = true;
        }
    }

    oneSide_ = (!containing_0_PI || !containing_PI_2PI);

    lbInfo << "[SampleSet::updateOneSide] " << oneSide_;
}
