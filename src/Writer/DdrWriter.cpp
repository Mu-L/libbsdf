// =================================================================== //
// Copyright (C) 2014-2019 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#include <libbsdf/Writer/DdrWriter.h>

#include <fstream>
#include <memory>

#include <libbsdf/Brdf/Processor.h>
#include <libbsdf/Brdf/SpecularCoordinatesBrdf.h>
#include <libbsdf/Brdf/SphericalCoordinatesBrdf.h>
#include <libbsdf/Common/Log.h>
#include <libbsdf/Common/SpectrumUtility.h>
#include <libbsdf/Common/Version.h>

using namespace lb;

bool DdrWriter::write(const std::string&                fileName,
                      const SpecularCoordinatesBrdf&    brdf,
                      const std::string&                comments)
{
    std::ofstream fout(fileName.c_str());
    if (fout.fail()) {
        lbError << "[DdrReader::write] Could not open: " << fileName;
        return false;
    }

    return output(brdf, fout, comments);
}

bool DdrWriter::write(const std::string&    fileName,
                      const Brdf&           brdf,
                      DataType              dataType,
                      const std::string&    comments)
{
    if (!brdf.getSampleSet()->validate()) {
        lbError << "[DdrReader::write] BRDF data is invalid.";
        return false;
    }

    std::unique_ptr<SpecularCoordinatesBrdf> convertedBrdf(convert(brdf));
    std::unique_ptr<SpecularCoordinatesBrdf> extrapolatedBrdf(arrange(*convertedBrdf, dataType));

    return DdrWriter::write(fileName, *extrapolatedBrdf, comments);
}

bool DdrWriter::output(const SpecularCoordinatesBrdf&   brdf,
                       std::ostream&                    stream,
                       const std::string&               comments)
{
    if (!brdf.getSampleSet()->validate()) {
        lbError << "[DdrReader::write] BRDF data is invalid.";
        return false;
    }

    std::ios_base::sync_with_stdio(false);

    stream << ";; This file is generated by libbsdf-" << getVersion() << "." << std::endl;

    if (!comments.empty()) {
        stream << ";; " << comments << std::endl;
    }
    stream << std::endl;

    const SampleSet* ss = brdf.getSampleSet();

    SourceType sourceType = brdf.getSourceType();
    if (sourceType == MEASURED_SOURCE) {
        stream << "Source Measured" << std::endl;
    }
    else if (sourceType == GENERATED_SOURCE) {
        stream << "Source Generated" << std::endl;
    }
    else if (sourceType == EDITED_SOURCE) {
        stream << "Source Edited" << std::endl;
    }
    else {
        stream << "Source Measured" << std::endl;
    }

    if (ss->isIsotropic()) {
        stream << "TypeSym ASymmetrical" << std::endl;
    }
    else {
        stream << "TypeSym ASymmetrical 4D" << std::endl;
    }

    ColorModel colorModel;
    stream << "TypeColorModel ";
    if (ss->getNumWavelengths() == 1) {
        colorModel = MONOCHROMATIC_MODEL;
        stream << "BW" << std::endl;
    }
    else if (ss->getColorModel() == RGB_MODEL ||
             ss->getColorModel() == XYZ_MODEL) {
        colorModel = RGB_MODEL;
        stream << "RGB" << std::endl;
    }
    else {
        colorModel = SPECTRAL_MODEL;
        stream << "spectral " << ss->getNumWavelengths() << std::endl;
    }

    stream << "TypeData Luminance Absolute" << std::endl;

    if (!ss->isIsotropic()) {
        stream << "psi " << brdf.getNumInPhi() << std::endl;
        for (int i = 0; i < brdf.getNumInPhi(); ++i) {
            stream << " " << toDegree(brdf.getInPhi(i));
        }
        stream << std::endl;
    }

    stream << "sigma " << brdf.getNumInTheta() << std::endl;
    for (int i = 0; i < brdf.getNumInTheta(); ++i) {
        stream << " " << toDegree(brdf.getInTheta(i));
    }
    stream << std::endl;

    if (brdf.getNumSpecularOffsets() == brdf.getNumInTheta()) {
        stream << "sigmat" << std::endl;
        for (int i = 0; i < brdf.getNumSpecularOffsets(); ++i) {
            stream << " " << toDegree(brdf.getInTheta(i) + brdf.getSpecularOffset(i));
        }
        stream << std::endl;
    }

    stream << "phi " << brdf.getNumSpecPhi() << std::endl;
    for (int i = 0; i < brdf.getNumSpecPhi(); ++i) {
        stream << " " << toDegree(brdf.getSpecPhi(i));
    }
    stream << std::endl;

    stream << "theta " << brdf.getNumSpecTheta() << std::endl;
    for (int i = 0; i < brdf.getNumSpecTheta(); ++i) {
        stream << " " << toDegree(brdf.getSpecTheta(i));
    }
    stream << std::endl;

    for (int wlIndex = 0; wlIndex < ss->getNumWavelengths(); ++wlIndex) {
        if (colorModel == MONOCHROMATIC_MODEL) {
            stream << "bw" << std::endl;
        }
        else if (colorModel == RGB_MODEL) {
            if (wlIndex == 0) {
                stream << "red" << std::endl;
            }
            else if (wlIndex == 1) {
                stream << "green" << std::endl;
            }
            else {
                stream << "blue" << std::endl;
            }
        }
        else {
            stream << "wl " << ss->getWavelength(wlIndex) << std::endl;
        }

        stream << " kbdf" << std::endl;
        stream << " ";
        for (int i = 0; i < brdf.getNumInTheta() * brdf.getNumInPhi(); ++i) {
            stream << " 1.0";
        }

        stream << "\n def" << std::endl;

        for (int inPhIndex = 0; inPhIndex < brdf.getNumInPhi(); ++inPhIndex) {
            stream << ";; Psi = " << toDegree(brdf.getInPhi(inPhIndex)) << std::endl;

            for (int inThIndex = 0; inThIndex < brdf.getNumInTheta(); ++inThIndex) {
                stream << ";; Sigma = " << toDegree(brdf.getInTheta(inThIndex)) << std::endl;

                for (int spPhIndex = 0; spPhIndex < brdf.getNumSpecPhi();   ++spPhIndex) {
                for (int spThIndex = 0; spThIndex < brdf.getNumSpecTheta(); ++spThIndex) {
                    Spectrum sp = brdf.getSpectrum(inThIndex, inPhIndex, spThIndex, spPhIndex);
                    sp = sp.cwiseMax(0.0);

                    if (ss->getColorModel() == XYZ_MODEL) {
                        Spectrum rgb = xyzToSrgb<Vec3f>(sp);
                        stream << " " << rgb[wlIndex] * PI_F;
                    }
                    else {
                        stream << " " << sp[wlIndex] * PI_F;
                    }
                }

                stream << std::endl;
                }
            }
        }

        stream << " enddef" << std::endl;
    }

    return true;
}

SpecularCoordinatesBrdf* DdrWriter::convert(const Brdf& brdf)
{
    typedef SpecularCoordinatesBrdf SpecBrdf;

    if (const SpecBrdf* specBrdf = dynamic_cast<const SpecBrdf*>(&brdf)) {
        return new SpecBrdf(*specBrdf);
    }
    else if (const SphericalCoordinatesBrdf* spheBrdf = dynamic_cast<const SphericalCoordinatesBrdf*>(&brdf)) {
        using std::max;

        const SampleSet* ss = spheBrdf->getSampleSet();

        int numSpecTheta = max(ss->getNumAngles2(), 181);
        int numSpecPhi   = max(ss->getNumAngles3(), 73);
        return new SpecBrdf(*spheBrdf, numSpecTheta, numSpecPhi);
    }
    else {
        const SampleSet* ss = brdf.getSampleSet();
        int numInPhi = (ss->getNumAngles1() == 1) ? 1 : 37;

        Arrayf inThetaAngles    = Arrayf::LinSpaced(19,         0.0f, SpecularCoordinateSystem::MAX_ANGLE0);
        Arrayf inPhiAngles      = Arrayf::LinSpaced(numInPhi,   0.0f, SpecularCoordinateSystem::MAX_ANGLE1);
        Arrayf specPhiAngles    = Arrayf::LinSpaced(73,         0.0f, SpecularCoordinateSystem::MAX_ANGLE3);

        // Create narrow intervals near specular directions.
        Arrayf specThetaAngles = createExponentialArray<Arrayf>(91,
                                                                SpecularCoordinateSystem::MAX_ANGLE2,
                                                                2.0f);

        return new SpecBrdf(brdf, inThetaAngles, inPhiAngles, specThetaAngles, specPhiAngles);
    }
}

SpecularCoordinatesBrdf* DdrWriter::arrange(const SpecularCoordinatesBrdf&  brdf,
                                            DataType                        dataType)
{
    SpecularCoordinatesBrdf* arrangedBrdf = new SpecularCoordinatesBrdf(brdf);

    // If a BRDF has one incoming polar angle, it is expanded.
    if (arrangedBrdf->getNumInTheta() == 1) {
        const SampleSet* ss = arrangedBrdf->getSampleSet();

        Arrayf inThetaAngles = Arrayf::LinSpaced(10, 0.0, SpecularCoordinateSystem::MAX_ANGLE0);

        SpecularCoordinatesBrdf* filledBrdf = new SpecularCoordinatesBrdf(*arrangedBrdf,
                                                                          inThetaAngles,
                                                                          ss->getAngles1(),
                                                                          ss->getAngles2(),
                                                                          ss->getAngles3());
        delete arrangedBrdf;
        arrangedBrdf = filledBrdf;
    }

    equalizeOverlappingSamples(arrangedBrdf);
    arrangedBrdf->expandAngles();
    copySpectraFromPhiOf0To360(arrangedBrdf->getSampleSet());
    fixEnergyConservation(arrangedBrdf);

    if (dataType == BTDF_DATA) {
        fillSpectraAtInThetaOf90(arrangedBrdf, 0.0f);
    }

    return arrangedBrdf;
}
