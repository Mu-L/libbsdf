// =================================================================== //
// Copyright (C) 2014-2024 Kimura Ryo                                  //
//                                                                     //
// This Source Code Form is subject to the terms of the Mozilla Public //
// License, v. 2.0. If a copy of the MPL was not distributed with this //
// file, You can obtain one at http://mozilla.org/MPL/2.0/.            //
// =================================================================== //

/*!
 * \file    Array.h
 * \brief   The Array.h header file includes the array declarations and functions.
 */

#ifndef LIBBSDF_ARRAY_H
#define LIBBSDF_ARRAY_H

#include <iterator>
#include <vector>

#include <libbsdf/Common/Utility.h>

namespace lb {

using Arrayf = Eigen::ArrayXf;
using Arrayd = Eigen::ArrayXd;

namespace array_util {

/*! \brief Copies an array. */
template <typename SrcT, typename DestT>
void copy(const SrcT& srcArray, DestT* destArray);

/*! \brief Appends an element to the end of an array. */
template <typename ArrayT>
void appendElement(ArrayT* arrayf, typename ArrayT::Scalar value);

/*! \brief Creates a non-equal interval array from zero to \a maxValue with \a exponent. */
template <typename ArrayT>
ArrayT createExponential(int                     numElements,
                         typename ArrayT::Scalar maxValue,
                         typename ArrayT::Scalar exponent);

/*! \brief Returns true if the elements of an array are equally-spaced intervals. */
template <typename T>
bool isEqualInterval(const T& array);

/*!
 * Finds neighbor indices and values.
 * If \a value is out of bounds, two nearest bounds are returned for extrapolation.
 *
 * \param lowerIndex Found index of the sample point at the lower bound.
 * \param upperIndex Found index of the sample point at the upper bound.
 * \param lowerValue Found value of the sample point at the lower bound.
 * \param upperValue Found value of the sample point at the upper bound.
 */
void findBounds(const Arrayd& values,
                double        value,
                bool          equalIntervalValues,
                int*          lowerIndex,
                int*          upperIndex,
                double*       lowerValue,
                double*       upperValue);

} // namespace array_util

/*
 * Implementation
 */

template <typename SrcT, typename DestT>
void array_util::copy(const SrcT& srcArray, DestT* destArray)
{
    int i = 0;
    for (auto it = srcArray.begin(); it != srcArray.end(); ++it, ++i) {
        (*destArray)[i] = *it;
    }
}

template <typename ArrayT>
void array_util::appendElement(ArrayT* arrayf, typename ArrayT::Scalar value)
{
    using ScalarType = typename ArrayT::Scalar;

    ArrayT& a = *arrayf;
    std::vector<ScalarType> orig(a.data(), a.data() + a.size());
    orig.push_back(value);
    a.resize(a.size() + 1);

#if (_MSC_VER >= 1600) && (_MSC_VER < 1930) // Visual Studio 2010 to 2019
    std::copy(orig.begin(), orig.end(),
              stdext::checked_array_iterator<ScalarType*>(a.data(), a.size()));
#else
    std::copy(orig.begin(), orig.end(), a.data());
#endif
}

template <typename ArrayT>
ArrayT array_util::createExponential(int                     numElements,
                                     typename ArrayT::Scalar maxValue,
                                     typename ArrayT::Scalar exponent)
{
    ArrayT arr = ArrayT::LinSpaced(numElements, 0.0, maxValue);

    for (int i = 1; i < arr.size() - 1; ++i) {
        typename ArrayT::Scalar ratio = arr[i] / maxValue;
        ratio = std::pow(ratio, exponent);
        arr[i] = ratio * maxValue;
    }

    return arr;
}

template <typename T>
bool array_util::isEqualInterval(const T& array)
{
    if (array.size() <= 2) return false;

    double interval = array[array.size() - 1] / (array.size() - 1);
    for (int i = 0; i < array.size(); ++i) {
        if (!isEqual(array[i], interval * i)) {
            return false;
        }
    }

    return true;
}

} // namespace lb

#endif // LIBBSDF_ARRAY_H
