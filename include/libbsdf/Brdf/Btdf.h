// =================================================================== //
// Copyright (C) 2014-2019 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

#ifndef LIBBSDF_BTDF_H
#define LIBBSDF_BTDF_H

#include <memory>

#include <libbsdf/Brdf/Brdf.h>

namespace lb {

/*!
 * \class   Btdf
 * \brief   The Btdf class provides the BTDF data and sampling functions.
 *
 * The BTDF data consists of angles, wavelengths, spectra, and coordinate system.
 * The data structure is defined in lb::Brdf and lb::SampleSet.
 */
class Btdf
{
public:
    /*! Constructs a BTDF. */
    explicit Btdf(std::shared_ptr<Brdf> brdf);

    virtual ~Btdf();

    /*! Gets the spectrum of the BTDF at incoming and outgoing directions. */
    Spectrum getSpectrum(const Vec3& inDir, const Vec3& outDir) const;

    /*!
     * Computes incoming and outgoing directions of a Cartesian coordinate system
     * using a set of angle indices.
     */
    void getInOutDirection(int      index0,
                           int      index1,
                           int      index2,
                           int      index3,
                           Vec3*    inDir,
                           Vec3*    outDir) const;

    std::shared_ptr<Brdf>       getBrdf();       /*!< Gets the BRDF data. */
    std::shared_ptr<const Brdf> getBrdf() const; /*!< Gets the BRDF data. */

    SampleSet*       getSampleSet();       /*!< Gets sample points. */
    const SampleSet* getSampleSet() const; /*!< Gets sample points. */

protected:
    /*! This attribute holds the BRDF data including angles, wavelengths, and spectra. */
    std::shared_ptr<Brdf> brdf_;

private:
    /*! Copy operator is disabled. */
    Btdf& operator=(const Btdf&);
};

inline Spectrum Btdf::getSpectrum(const Vec3& inDir, const Vec3& outDir) const
{
    using std::abs;

    Spectrum sp = brdf_->getSpectrum(Vec3(inDir[0],  inDir[1],  abs(inDir[2])),
                                     Vec3(outDir[0], outDir[1], abs(outDir[2])));
    return sp;
}

inline void Btdf::getInOutDirection(int index0, int index1, int index2, int index3,
                                    Vec3* inDir, Vec3* outDir) const
{
    brdf_->getInOutDirection(index0, index1, index2, index3,
                             inDir, outDir);
    outDir->z() = -outDir->z();
}

inline std::shared_ptr<      Brdf> Btdf::getBrdf()       { return brdf_; }
inline std::shared_ptr<const Brdf> Btdf::getBrdf() const { return brdf_; }

inline       SampleSet* Btdf::getSampleSet()       { return brdf_->getSampleSet(); }
inline const SampleSet* Btdf::getSampleSet() const { return brdf_->getSampleSet(); }

} // namespace lb

#endif // LIBBSDF_BTDF_H
