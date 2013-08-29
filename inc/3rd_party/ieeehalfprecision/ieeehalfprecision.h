#ifndef __IEEE_HALF_PRECISION_CONVERSION_UTIL_HPP__
#define __IEEE_HALF_PRECISION_CONVERSION_UTIL_HPP__

#ifdef __cplusplus
extern "C" {
#endif

  
  /*!\fn singles2halfp
    Converts from 32bit-floats to 16bit-floats.
    [routine from James Tursa, BSD License,
     see ieeehalfprecision.c for details].
    \param target destination to which to write 16bit-floats
    \param source source from which to read 32bit-floats
    \param numel number of conversions to perform, i.e.
                 source points to numel floats
                 and target points to 2*numel bytes.
   */
  void singles2halfp(uint16_t *target, const uint32_t *source, int numel);

  
  /*!\fn halfp2singles
    Converts from 16bit-floats to 32bit-floats.
    [routine from James Tursa, BSD License,
     see ieeehalfprecision.c for details].
    \param target destination to which to write 32bit-floats
    \param source source from which to read 16bit-floats
    \param numel number of conversions to perform, i.e.
                 source points to 2*numel bytes
                 and target points to numem floats.
   */
  void halfp2singles(uint32_t *target, const uint16_t *source, int numel);

#ifdef __cplusplus
}
#endif

#endif
