/**
 * Miscelaneous algorithms related to computing percentiles
 **/

#pragma once

#include "CartaLib/CartaLib.h"
#include "CartaLib/IImage.h"
#include "CartaLib/IntensityUnitConverter.h"
#include <QDebug>
#include <limits>
#include <algorithm>
#include <vector>
#include <cmath>
#include <numeric>
#include <QElapsedTimer>

namespace Carta
{
namespace Core
{
namespace Algorithms
{
/// compute requested percentiles
/// \param view the input dataset
/// \param percentiles which percentiles to compute
/// \return the computed intensities. If all inputs are nans, the result will also be nans.
///
/// Example: [0.1] will compute a value such that 10% of all values are smaller than the returned
/// value.
///
/// \note this is a dumb algorithm using quickselect. It really only works on datasets that
/// are small enough to store in memory. For really big datasets we need a lot more sophisticated
/// algorithm.
///
/// \note NANs are treated as if they did not exist
///
/// \note for best performance, the supplied list of percentiles should be sorted small->large
template < typename Scalar >
static
typename std::map < double, Scalar >
percentile2pixels(
    Carta::Lib::NdArray::TypedView < Scalar > & view,
    std::vector < double > percentiles,
    int spectralIndex=-1,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter=nullptr,
    std::vector<double> hertzValues={}
    )
{
    // basic preconditions
    if ( CARTA_RUNTIME_CHECKS ) {
        for ( auto q : percentiles ) {
            CARTA_ASSERT( 0.0 <= q && q <= 1.0 );
            Q_UNUSED(q);
        }
    }

    // read in all values from the view into memory so that we can do quickselect on it
    std::vector < Scalar > allValues;

    if (converter && converter->frameDependent && spectralIndex >= 0) {
        // we need to apply the frame-dependent conversion to each intensity value before copying it
        // to avoid calculating the frame index repeatedly we use slices to iterate over the image one frame at a time

        for (size_t f = 0; f < hertzValues.size(); f++) {
            // Create a slice for retrieving a single frame
            SliceND frame;
            for (size_t d = 0; d < view.dims().size(); d++) {
                if ((int)d == spectralIndex) {
                    frame.index(f);
                } else {
                    frame.next();
                }
            }

            // Create a new view for a single frame using this slice
            Carta::Lib::NdArray::Double viewSlice( view.rawView()->getView(frame), false ); // create a new view for this slice

            double hertzVal = hertzValues[f];

            // iterate over the frame
            viewSlice.forEach(
                [& allValues, &converter, &hertzVal](const Scalar & val) {
                    // check if the value from raw data is finite
                    if ( std::isfinite( val ) ) {
                        allValues.push_back( converter->_frameDependentConvert(val, hertzVal) );
                    }
                }
            );
        }
        
    } else {
        // we can loop over the flat image
        view.forEach(
            [& allValues] ( const Scalar & val ) {
                // check if the value from raw data is finite
                if ( std::isfinite( val ) ) {
                    allValues.push_back( val );
                }
            }
        );

    }

    // indicate bad clip if no finite numbers were found
    if ( allValues.size() == 0 ) {
        qFatal( "The size of raw data is zero !!" );
    }

    std::map < double, Scalar > result;

    // for every input percentile, do quickselect and store the result

    for ( double q : percentiles ) {
        // we clamp to incremented values and decrement at the end because size_t cannot be negative
        size_t x1 = Carta::Lib::clamp<size_t>(allValues.size() * q , 1, allValues.size()) - 1;
        CARTA_ASSERT( 0 <= x1 && x1 < allValues.size() );
        std::nth_element( allValues.begin(), allValues.begin() + x1, allValues.end() );
        result[q] = allValues[x1];
    }

    // if we have a frame-ependent converter and image with a single frame,
    // apply the frame-dependent conversion at the end to the final intensities
    if (converter && converter->frameDependent && spectralIndex < 0) {
        for (size_t i = 0; i < result.size(); i++) {
            result[i] = converter->_frameDependentConvert(result[i], hertzValues[0]);
        }
    }   

    CARTA_ASSERT( result.size() == percentiles.size());

    return result;
} // computeClips


template < typename Scalar >
static
typename std::vector<double>
intensities2percentiles(
    Carta::Lib::NdArray::TypedView < Scalar > & view,
    std::vector < Scalar > intensities,
    int spectralIndex=-1,
    Carta::Lib::IntensityUnitConverter::SharedPtr converter=nullptr,
    std::vector<double> hertzValues={}
    )
{
    std::vector<Scalar> divided_intensities;
    std::vector<Scalar> target_intensities;

    u_int64_t totalCount = 0;
    std::vector<u_int64_t> countBelow(intensities.size());
    std::vector<double> percentiles(intensities.size());

    if (converter) {
        // Divide the target intensities by the multiplier
        for (auto& intensity : intensities) {
            divided_intensities.push_back(intensity / converter->multiplier);
        }

        if (converter->frameDependent) { // more complicated loop for frame-dependent conversions
            target_intensities.resize(intensities.size());
            
            if (spectralIndex >= 0) { // need to recalculate target intensities for every frame
                for (size_t f = 0; f < hertzValues.size(); f++) {
                    for (size_t i = 0; i < intensities.size(); i++) {
                        target_intensities[i] = converter->_frameDependentConvertInverse(divided_intensities[i], hertzValues[f]);
                    }

                    // Create a slice for retrieving a single frame
                    SliceND frame;
                    for (size_t d = 0; d < view.dims().size(); d++) {
                        if ((int)d == spectralIndex) {
                            frame.index(f);
                        } else {
                            frame.next();
                        }
                    }

                    // Create a new view for a single frame using this slice
                    Carta::Lib::NdArray::Double viewSlice( view.rawView()->getView(frame), false ); // create a new view for this slice
                    
                    viewSlice.forEach([&](const double& val) { // iterate over the frame
                        if( Q_UNLIKELY( std::isnan(val))){
                            return;
                        }
                        
                        totalCount++;

                        for (size_t i = 0; i < intensities.size(); i++) {
                            if( val <= target_intensities[i]){
                                countBelow[i]++;
                            }
                        }
                        return;
                    });
                }
            } else { // no spectral axis; only one frame; calculate the target intensities once and wait for generic counting loop at the end
                for (size_t i = 0; i < intensities.size(); i++) {
                    target_intensities[i] = converter->_frameDependentConvertInverse(divided_intensities[i], hertzValues[0]);
                }
            }
        } else { // not frame-dependent; calculate the target intensities once and wait for generic counting loop at the end
            target_intensities = divided_intensities;
        }
    } else { // no conversion
        target_intensities = intensities;
    }  

    if (!converter || !converter->frameDependent || spectralIndex < 0) { // generic counting loop
        view.forEach([&](const double& val) {
            if( Q_UNLIKELY( std::isnan(val))){
                return;
            }
            
            totalCount++;

            for (size_t i = 0; i < intensities.size(); i++) {
                if( val <= target_intensities[i]){
                    countBelow[i]++;
                }
            }
            return;
        });
    }

    for (size_t i = 0; i < intensities.size(); i++) { // calculate the percentages
        if ( totalCount > 0 ){
            percentiles[i] = double(countBelow[i]) / totalCount;
        }
    }
    
    return percentiles;
}


}
}
}