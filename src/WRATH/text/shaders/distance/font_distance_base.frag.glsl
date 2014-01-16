#define WRATH_FONT_IMPLEMENT_SIGNED_DISTANCE

float wrath_font_distance_compute_signed_distance(float rr)
{
  return wrath_font_page_data(3)*(2.0*rr - 1.0);
}
