// =================================================================== //
// Copyright (C) 2016-2019 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#include <libbsdf/ReflectanceModel/ReflectanceModelUtility.h>

#include <libbsdf/Brdf/Processor.h>
#include <libbsdf/Brdf/SpecularCoordinatesBrdf.h>

using namespace lb;

bool reflectance_model_utility::setupTabularBrdf(const ReflectanceModel&    model,
                                                 Brdf*                      brdf,
                                                 DataType                   dataType,
                                                 float                      maxValue)
{
    using std::abs;
    using std::max;
    using std::min;

    SampleSet* ss = brdf->getSampleSet();

    ColorModel cm = ss->getColorModel();
    if (cm != RGB_MODEL &&
        cm != MONOCHROMATIC_MODEL) {
        std::cerr << "[setupTabularBrdf] Unsupported color model: " << cm << std::endl;
        return false;
    }

    SpecularCoordinatesBrdf* spBrdf = dynamic_cast<SpecularCoordinatesBrdf*>(brdf);
    bool backSideFillable = (spBrdf != 0);

    Vec3 inDir, outDir;
    Vec3 values;
    Spectrum sp;
    int i0, i1, i3;
    #pragma omp parallel for private(inDir, outDir, values, sp, i0, i1, i3) schedule(dynamic)
    for (int i2 = 0; i2 < ss->getNumAngles2(); ++i2) {
        for (i0 = 0; i0 < ss->getNumAngles0(); ++i0) {
        for (i1 = 0; i1 < ss->getNumAngles1(); ++i1) {
        for (i3 = 0; i3 < ss->getNumAngles3(); ++i3) {
            brdf->getInOutDirection(i0, i1, i2, i3, &inDir, &outDir);

            if (backSideFillable && isDownwardDir(outDir)) {
                continue;
            }

            const float minZ = 0.001f;
            inDir.z() = max(inDir.z(), minZ);
            outDir.z() = max(outDir.z(), minZ);

            if (abs(outDir.x()) <= minZ &&
                abs(outDir.y()) <= minZ &&
                outDir.z() <= minZ) {
                outDir.x() = 1.0f;
            }

            inDir.normalize();
            outDir.normalize();

            if (dataType == BTDF_DATA) {
                outDir.z() = -outDir.z();
            }

            values = model.getBrdfValue(inDir, outDir);
            assert(values.allFinite());

            if (cm == RGB_MODEL) {
                sp = values.asVector3f();
                sp = sp.cwiseMin(maxValue);
            }
            else { // MONOCHROMATIC_MODEL
                sp.resize(1);
                sp[0] = min(values.sum() / 3.0f, maxValue);
            }
            ss->setSpectrum(i0, i1, i2, i3, sp);
        }}}
    }

    if (backSideFillable) {
        fillBackSide(spBrdf);
    }

    return true;
}
