#define WRATH_FONT_IMPLEMENT_SIGNED_DISTANCE

mediump float 
wrath_font_distance_compute_signed_distance(in mediump float rr)
{
  return wrath_font_page_data(2)*(2.0*rr - 1.0);
}
