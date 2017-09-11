/**
 * Miscelaneous algorithms related to computing percentiles
 **/

#pragma once

#include "CartaLib/CartaLib.h"
#include "CartaLib/IImage.h"
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
/// \param quant which percentiles to compute
/// \return the computed percentiles. If all inputs are nans, the result will also be nans.
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
    std::vector < double > quant
    )
{
    qDebug() << "computeClips" << view.dims();

    // basic preconditions
    if ( CARTA_RUNTIME_CHECKS ) {
        for ( auto q : quant ) {
            CARTA_ASSERT( 0.0 <= q && q <= 1.0 );
            Q_UNUSED(q);
        }
    }

    // read in all values from the view into memory so that we can do quickselect on it
    std::vector < Scalar > allValues;
    view.forEach(
        [& allValues] ( const Scalar & val ) {
            // check if the value from raw data is finite
            if ( std::isfinite( val ) ) {
                allValues.push_back( val );
            }
        }
    );

    // indicate bad clip if no finite numbers were found
    if ( allValues.size() == 0 ) {
        qFatal( "The size of raw data is zero !!" );
    }

    std::map < double, Scalar > result;

    // for every input percentile, do quickselect and store the result

    for ( double q : quant ) {
        // we clamp to incremented values and decrement at the end because size_t cannot be negative
        size_t x1 = Carta::Lib::clamp<size_t>(allValues.size() * q , 1, allValues.size()) - 1;
        CARTA_ASSERT( 0 <= x1 && x1 < allValues.size() );
        std::nth_element( allValues.begin(), allValues.begin() + x1, allValues.end() );
        result[q] = allValues[x1];
    }
    CARTA_ASSERT( result.size() == quant.size());

    return result;
} // computeClips

///
///
///
///
template < typename Scalar >
static
std::map<double, std::pair<int,Scalar>>
percentile2pixels_I(
    Carta::Lib::NdArray::TypedView < Scalar > & view,
    int spectralIndex,
    std::vector < double > quant
    )
{
    // basic preconditions
    if ( CARTA_RUNTIME_CHECKS ) {
        for ( auto q : quant ) {
            CARTA_ASSERT( 0.0 <= q && q <= 1.0 );
            Q_UNUSED(q);
        }
    }

    // read in all values from the view into memory so that we can do quickselect on it
    int index = 0;
    std::vector<std::pair<int,Scalar>> allValues;
    std::map<double, std::pair<int,Scalar>> result;

    std::vector<int> dims = view.dims();
    qDebug() << "++++++++ raw data shape for calculating percentile is"<< dims;

    int total_size = std::accumulate(dims.begin(), dims.end(), 1, std::multiplies<int>());
    qDebug() << "++++++++ raw data totalsize=" << total_size;

    int divisor = total_size;
    if (spectralIndex != -1) {
        divisor /= dims[spectralIndex];
    }
    qDebug() << "++++++++ raw data size per channel=" << divisor;

    // Preallocate space to avoid running out of memory unnecessarily through dynamic allocation
    allValues.reserve(total_size);

    // start timer for loading the raw data
    QElapsedTimer timer;
    timer.start();

    view.forEach(
        [&allValues, &index] ( const Scalar &val ) {
            // check if the value from raw data is finite
            if ( std::isfinite( val ) ) {
                allValues.push_back( std::make_pair(index, val) );
            }
            index++;
        }
    );
    qDebug() << "++++++++ raw data index number=" << index;

    // end of timer for loading the raw data
    qCritical() << "<> Time to load the raw data:" << timer.elapsed() << "milliseconds";

    // indicate bad clip if no finite numbers were found
    if ( allValues.size() == 0 ) {
        qFatal( "The size of raw data is zero !!" );
    }

    // for every input percentile, do quickselect and store the result
    // only compare the intensity values and ignore the indices
    auto compareIntensityTuples = [] (const std::pair<int,Scalar>& lhs, const std::pair<int,Scalar>& rhs) { return lhs.second < rhs.second; };

    for ( double q : quant ) {
        // x1 is the locationIndex used for quantile calculation
        // we clamp to incremented values and decrement at the end because size_t cannot be negative
        size_t x1 = Carta::Lib::clamp<size_t>(allValues.size() * q , 1, allValues.size()) - 1;
        CARTA_ASSERT( 0 <= x1 && x1 < allValues.size() );

        // start timer for getting the nth_element from raw data
        QElapsedTimer timer2;
        timer2.start();

        std::nth_element( allValues.begin(), allValues.begin()+x1, allValues.end(), compareIntensityTuples );

        // end of timer for getting the nth_element from the raw data
        qCritical() << "<> Time to get the nth_element from the raw data:" << timer2.elapsed() << "milliseconds";

        result[q] = std::make_pair(allValues[x1].first/divisor, allValues[x1].second);
    }

    return result;
}


}
}
}
